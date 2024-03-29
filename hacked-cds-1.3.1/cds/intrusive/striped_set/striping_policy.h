/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_INTRUSIVE_STRIPED_SET_STRIPING_POLICY_H
#define __CDS_INTRUSIVE_STRIPED_SET_STRIPING_POLICY_H

#include <cds/lock/array.h>
#include <cds/os/thread.h>
#include <cds/details/std/memory.h>
#include <cds/lock/spinlock.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>


namespace cds { namespace intrusive { namespace striped_set {

    /// Lock striping concurrent access policy
    /**
        This is one of available opt::mutex_policy option type for StripedSet

        Lock striping is very simple technique.
        The set consists of the bucket table and the array of locks.
        Initially, the capacity of lock array and bucket table is the same.
        When set is resized, bucket table capacity will be doubled but lock array will not.
        The lock \p i protects each bucket \p j, where <tt> j = i mod L </tt>,
        where \p L - the size of lock array.

        The policy contains an internal array of \p Lock locks.

        Template arguments:
        - \p Lock - the type of mutex. The default is \p boost::mutex. The mutex type should be default-constructible.
            Note that a spin-lock is not so good suitable for lock striping for performance reason.
        - \p Alloc - allocator type used for lock array memory allocation. Default is \p CDS_DEFAULT_ALLOCATOR.
    */
    template <class Lock = boost::mutex, class Alloc = CDS_DEFAULT_ALLOCATOR >
    class striping
    {
    public:
        typedef Lock    lock_type       ;   ///< lock type
        typedef Alloc   allocator_type  ;   ///< allocator type

        typedef cds::lock::array< lock_type, cds::lock::pow2_select_policy, allocator_type >    lock_array_type ;   ///< lock array type

    protected:
        //@cond
        lock_array_type m_Locks ;
        //@endcond

    public:
        //@cond
        class scoped_cell_lock {
            cds::lock::scoped_lock< lock_array_type >   m_guard ;

        public:
            scoped_cell_lock( striping& policy, size_t nHash )
                : m_guard( policy.m_Locks, nHash )
            {}
        };

        class scoped_full_lock {
            cds::lock::scoped_lock< lock_array_type >   m_guard ;
        public:
            scoped_full_lock( striping& policy )
                : m_guard( policy.m_Locks )
            {}
        };

        class scoped_resize_lock: public scoped_full_lock {
        public:
            scoped_resize_lock( striping& policy )
                : scoped_full_lock( policy )
            {}

            bool success() const
            {
                return true ;
            }
        };
        //@endcond

    public:
        /// Constructor
        striping(
            size_t nLockCount   ///< The size of lock array. Must be power of two.
        )
            : m_Locks( nLockCount, cds::lock::pow2_select_policy( nLockCount ))
        {}

        /// Returns lock array size
        /**
            Lock array size is unchanged during \p striped object lifetime
        */
        size_t lock_count() const
        {
            return m_Locks.size()   ;
        }

        //@cond
        void resize( size_t /*nNewCapacity*/ )
        {}
        //@endcond
    };


    /// Refinable concurrent access policy
    /**
        This is one of available opt::mutex_policy option type for StripedSet

        Refining is like a striping technique (see striped_set::striping)
        but it allows growing the size of lock array when resizing the hash table.
        So, the sizes of hash table and lock array are equal.

        Template arguments:
        - \p RecursiveLock - the type of mutex. Reentrant (recursive) mutex is required.
            The default is \p boost::recursive_mutex. The mutex type should be default-constructible.
        - \p BackOff - back-off strategy. Default is cds::backoff::yield
        - \p Alloc - allocator type used for lock array memory allocation. Default is \p CDS_DEFAULT_ALLOCATOR.
    */
    template <
        class RecursiveLock = boost::recursive_mutex,
        typename BackOff = cds::backoff::yield,
        class Alloc = CDS_DEFAULT_ALLOCATOR>
    class refinable
    {
    public:
        typedef RecursiveLock   lock_type   ;   ///< lock type
        typedef BackOff         back_off    ;   ///< back-off strategy used
        typedef Alloc           allocator_type; ///< allocator type

    protected:
        //@cond
        typedef cds::lock::trivial_select_policy  lock_selection_policy   ;

        class lock_array_type
            : public cds::lock::array< lock_type, lock_selection_policy, allocator_type >
            , public std::enable_shared_from_this< lock_array_type >
        {
            typedef cds::lock::array< lock_type, lock_selection_policy, allocator_type >    lock_array_base ;
        public:
            lock_array_type( size_t nCapacity )
                : lock_array_base( nCapacity )
            {}
        };
        typedef std::shared_ptr< lock_array_type >  lock_array_ptr ;
        typedef cds::details::Allocator< lock_array_type, allocator_type >  lock_array_allocator    ;

        typedef unsigned long long  owner_t     ;
        typedef cds::OS::ThreadId   threadId_t  ;

        typedef cds::lock::Spin     spinlock_type ;
        typedef cds::lock::scoped_lock< spinlock_type > scoped_spinlock ;
        //@endcond

    protected:
        //@cond
        static owner_t const c_nOwnerMask = (((owner_t) 1) << (sizeof(owner_t) * 8 - 1)) - 1 ;

        lock_array_ptr                  m_arrLocks  ;   ///< Lock array. The capacity of array is specified in constructor.
        CDS_ATOMIC::atomic< owner_t >   m_Owner     ;   ///< owner mark (thread id + boolean flag)
        CDS_ATOMIC::atomic<size_t>      m_nCapacity ;   ///< Lock array capacity
        spinlock_type                   m_access    ;   ///< access to m_arrLocks
        //@endcond

    protected:
        //@cond
        struct lock_array_disposer {
            void operator()( lock_array_type * pArr )
            {
                lock_array_allocator().Delete( pArr )   ;
            }
        };

        lock_array_ptr create_lock_array( size_t nCapacity )
        {
            m_nCapacity.store( nCapacity, CDS_ATOMIC::memory_order_relaxed ) ;
            return lock_array_ptr( lock_array_allocator().New( nCapacity ), lock_array_disposer() ) ;
        }

        lock_type& acquire( size_t nHash )
        {
            owner_t me = (owner_t) cds::OS::getCurrentThreadId() ;
            owner_t who ;

            back_off bkoff  ;
            while ( true ) {
                // wait while resizing
                while ( true ) {
                    who = m_Owner.load( CDS_ATOMIC::memory_order_acquire )  ;
                    if ( !( who & 1 ) || (who >> 1) == (me & c_nOwnerMask) )
                        break   ;
                    bkoff()     ;
                }

                lock_array_ptr pLocks   ;
                {
                    scoped_spinlock sl(m_access)    ;
                    pLocks = m_arrLocks  ;
                }

                lock_type& lock = pLocks->at( nHash & (pLocks->size() - 1)) ;
                lock.lock() ;

                who = m_Owner.load( CDS_ATOMIC::memory_order_acquire )  ;
                if ( ( !(who & 1) || (who >> 1) == (me & c_nOwnerMask) ) && m_arrLocks == pLocks )
                    return lock ;
                lock.unlock() ;
            }
        }

        lock_array_ptr acquire_all()
        {
            owner_t me = (owner_t) cds::OS::getCurrentThreadId() ;
            owner_t who ;

            back_off bkoff  ;
            while ( true ) {
                // wait while resizing
                while ( true ) {
                    who = m_Owner.load( CDS_ATOMIC::memory_order_acquire )  ;
                    if ( !( who & 1 ) || (who >> 1) == (me & c_nOwnerMask) )
                        break   ;
                    bkoff()     ;
                }

                lock_array_ptr pLocks ;
                {
                    scoped_spinlock sl(m_access)    ;
                    pLocks = m_arrLocks ;
                }

                pLocks->lock_all()   ;

                who = m_Owner.load( CDS_ATOMIC::memory_order_acquire )  ;
                if ( ( !(who & 1) || (who >> 1) == (me & c_nOwnerMask) ) && m_arrLocks == pLocks )
                    return pLocks ;

                pLocks->unlock_all() ;
            }
        }

        void release_all( lock_array_ptr p )
        {
            p->unlock_all() ;
        }

        bool acquire_resize()
        {
            owner_t me = (owner_t) cds::OS::getCurrentThreadId() ;

            back_off bkoff ;
            for (unsigned int nAttempts = 0; nAttempts < 32; ++nAttempts ) {
                owner_t ownNull = 0 ;
                if ( m_Owner.compare_exchange_strong( ownNull, (me << 1) | 1, CDS_ATOMIC::memory_order_acquire, CDS_ATOMIC::memory_order_relaxed )) {
                    lock_array_ptr pOldLocks = m_arrLocks   ;
                    size_t const nLockCount = pOldLocks->size() ;
                    for ( size_t i = 0; i < nLockCount; ++i ) {
                        typename lock_array_type::lock_type& lock = pOldLocks->at(i) ;
                        bkoff.reset() ;
                        while ( !lock.try_lock() )
                            bkoff() ;
                        lock.unlock() ;
                    }
                    return true ;
                }
                else
                    bkoff() ;
            }
            return false ;
        }

        void release_resize()
        {
            m_Owner.store( 0, CDS_ATOMIC::memory_order_release )  ;
        }
        //@endcond
    public:
        //@cond
        class scoped_cell_lock {
            cds::lock::scoped_lock< lock_type >   m_guard ;

        public:
            scoped_cell_lock( refinable& policy, size_t nHash )
                : m_guard( policy.acquire( nHash ), true )
            {}
        };

        class scoped_full_lock {
            refinable&      m_Policy    ;
            lock_array_ptr  m_Locks     ;
        public:
            scoped_full_lock( refinable& policy )
                : m_Policy( policy )
            {
                m_Locks = policy.acquire_all()  ;
            }
            ~scoped_full_lock()
            {
                m_Policy.release_all( m_Locks )  ;
            }
        };

        class scoped_resize_lock {
            refinable&      m_Policy    ;
            bool            m_bSucceess ;

        public:
            scoped_resize_lock( refinable& policy )
                : m_Policy( policy )
            {
                m_bSucceess = policy.acquire_resize()  ;
            }

            ~scoped_resize_lock()
            {
                if ( m_bSucceess )
                    m_Policy.release_resize() ;
            }

            bool success() const
            {
                return m_bSucceess ;
            }
        };
        //@endcond

    public:
        /// Constructor
        refinable(
            size_t nLockCount   ///< Initial size of lock array. Must be power of two.
        )
        : m_Owner(0)
        , m_nCapacity( nLockCount )
        {
            assert( cds::beans::is_power2( nLockCount )) ;
            m_arrLocks = create_lock_array( nLockCount ) ;
        }

        /// Returns lock array size
        /**
            Lock array size is not a constant for \p refinable policy and can be changed when the set is resized.
        */
        size_t lock_count() const
        {
            return m_nCapacity.load( CDS_ATOMIC::memory_order_relaxed ) ;
        }

        /// Resize for new capacity
        void resize( size_t nNewCapacity )
        {
            // Expect the access is locked by scoped_resize_lock!!!
            lock_array_ptr pNewArr = create_lock_array( nNewCapacity ) ;
            scoped_spinlock sl(m_access)    ;
            m_arrLocks.swap( pNewArr )      ;
        }
    };

}}} // namespace cds::intrusive::striped_set

#endif

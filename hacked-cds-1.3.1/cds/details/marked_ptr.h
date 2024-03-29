/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_DETAILS_MARKUP_PTR_H
#define __CDS_DETAILS_MARKUP_PTR_H

#include <cds/cxx11_atomic.h>

namespace cds {
    namespace details {

        /// Marked pointer
        /**
            On the modern architectures, the default data alignment is 4 (for 32bit) or 8 byte for 64bit.
            Therefore, the least 2 or 3 bits of the pointer is always zero and can
            be used as a bitfield to store logical flags. This trick is widely used in
            lock-free programming to operate with the pointer and its flags atomically.

            Template parameters:
            - T - type of pointer
            - Bitmask - bitmask of least unused bits
        */
        template <typename T, int Bitmask>
        class marked_ptr
        {
            T *     m_ptr   ;   ///< pointer and its mark bits

        public:
            typedef T       value_type      ;       ///< type of value the class points to
            typedef T *     pointer_type    ;       ///< type of pointer
            static const uintptr_t bitmask = Bitmask  ;   ///< bitfield bitmask
            static const uintptr_t pointer_bitmask = ~bitmask ; ///< pointer bitmask

        public:
            /// Constructs null marked pointer. The flag is cleared.
            CDS_CONSTEXPR marked_ptr() CDS_NOEXCEPT
                : m_ptr( null_ptr<pointer_type>() )
            {}

            /// Constructs marked pointer with \p ptr value. The least bit(s) of \p ptr is the flag.
            CDS_CONSTEXPR explicit marked_ptr( value_type * ptr ) CDS_NOEXCEPT
                : m_ptr( ptr )
            {}

            /// Constructs marked pointer with \p ptr value and \p nMask flag.
            /**
                The \p nMask argument defines the OR-bits
            */
            marked_ptr( value_type * ptr, int nMask ) CDS_NOEXCEPT
                : m_ptr( ptr )
            {
                assert( bits() == 0 )   ;
                *this |= nMask ;
            }

#   ifdef CDS_CXX11_EXPLICITLY_DEFAULTED_FUNCTION_SUPPORT
            /// Copy constructor
            marked_ptr( marked_ptr const& src ) CDS_NOEXCEPT_DEFAULTED = default ;
            /// Copy-assignment operator
            marked_ptr& operator =( marked_ptr const& p ) CDS_NOEXCEPT_DEFAULTED = default ;
#       if defined(CDS_MOVE_SEMANTICS_SUPPORT) && !defined(CDS_DISABLE_DEFAULT_MOVE_CTOR)
            //@cond
            marked_ptr( marked_ptr&& src ) CDS_NOEXCEPT_DEFAULTED = default ;
            marked_ptr& operator =( marked_ptr&& p ) CDS_NOEXCEPT_DEFAULTED = default ;
            //@endcond
#       endif
#   else
            /// Copy constructor
            marked_ptr( marked_ptr const& src ) CDS_NOEXCEPT
                : m_ptr( src.m_ptr )
            {}

            /// Copy-assignment operator
            marked_ptr& operator =( marked_ptr const& p ) CDS_NOEXCEPT
            {
                m_ptr = p.m_ptr ;
                return *this    ;
            }
#   endif

            //TODO: make move ctor

        private:
            //@cond
            static uintptr_t   to_int( value_type * p ) CDS_NOEXCEPT
            {
                return reinterpret_cast<uintptr_t>( p ) ;
            }

            static value_type * to_ptr( uintptr_t n ) CDS_NOEXCEPT
            {
                return reinterpret_cast< value_type *>( n )    ;
            }

            uintptr_t   to_int() const CDS_NOEXCEPT
            {
                return to_int( m_ptr )  ;
            }
            //@endcond

        public:
            /// Returns the pointer without mark bits (real pointer) const version
            value_type *        ptr() const CDS_NOEXCEPT
            {
                return to_ptr( to_int() & ~bitmask );
            }

            /// Returns the pointer and bits together
            value_type *        all() const CDS_NOEXCEPT
            {
                return m_ptr    ;
            }

            /// Returns the least bits of pointer according to \p Bitmask template argument of the class
            uintptr_t bits() const CDS_NOEXCEPT
            {
                return to_int() & bitmask ;
            }

            /// Analogue for \ref ptr
            value_type * operator ->() const CDS_NOEXCEPT
            {
                return ptr()    ;
            }

            /// Assignment operator sets markup bits to zero
            marked_ptr operator =( T * p ) CDS_NOEXCEPT
            {
                m_ptr = p       ;
                return *this    ;
            }

            /// Set LSB bits as <tt>bits() | nBits</tt>
            marked_ptr& operator |=( int nBits ) CDS_NOEXCEPT
            {
                assert( (nBits & pointer_bitmask) == 0 )    ;
                m_ptr = to_ptr( to_int() | nBits ) ;
                return *this    ;
            }

            /// Set LSB bits as <tt>bits() & nBits</tt>
            marked_ptr& operator &=( int nBits ) CDS_NOEXCEPT
            {
                assert( (nBits & pointer_bitmask) == 0 )    ;
                m_ptr = to_ptr( to_int() & (pointer_bitmask | nBits) ) ;
                return *this    ;
            }

            /// Set LSB bits as <tt>bits() ^ nBits</tt>
            marked_ptr& operator ^=( int nBits ) CDS_NOEXCEPT
            {
                assert( (nBits & pointer_bitmask) == 0 )    ;
                m_ptr = to_ptr( to_int() ^ nBits )   ;
                return *this    ;
            }

            /// Returns <tt>p |= nBits</tt>
            friend marked_ptr operator |( marked_ptr p, int nBits) CDS_NOEXCEPT
            {
                p |= nBits       ;
                return p         ;
            }

            /// Returns <tt>p |= nBits</tt>
            friend marked_ptr operator |( int nBits, marked_ptr p ) CDS_NOEXCEPT
            {
                p |= nBits       ;
                return p         ;
            }

            /// Returns <tt>p &= nBits</tt>
            friend marked_ptr operator &( marked_ptr p, int nBits) CDS_NOEXCEPT
            {
                p &= nBits       ;
                return p         ;
            }

            /// Returns <tt>p &= nBits</tt>
            friend marked_ptr operator &( int nBits, marked_ptr p ) CDS_NOEXCEPT
            {
                p &= nBits       ;
                return p         ;
            }

            /// Returns <tt>p ^= nBits</tt>
            friend marked_ptr operator ^( marked_ptr p, int nBits) CDS_NOEXCEPT
            {
                p ^= nBits       ;
                return p         ;
            }
            /// Returns <tt>p ^= nBits</tt>
            friend marked_ptr operator ^( int nBits, marked_ptr p ) CDS_NOEXCEPT
            {
                p ^= nBits       ;
                return p         ;
            }

            /// Inverts LSBs of pointer \p p
            friend marked_ptr operator ~( marked_ptr p ) CDS_NOEXCEPT
            {
                return p ^ marked_ptr::bitmask ;
            }


            /// Comparing two marked pointer including its mark bits
            friend bool operator ==( marked_ptr p1, marked_ptr p2 ) CDS_NOEXCEPT
            {
                return p1.all() == p2.all() ;
            }

            /// Comparing marked pointer and raw pointer, mark bits of \p p1 is ignored
            friend bool operator ==( marked_ptr p1, value_type const * p2 ) CDS_NOEXCEPT
            {
                return p1.ptr() == p2 ;
            }

            /// Comparing marked pointer and raw pointer, mark bits of \p p2 is ignored
            friend bool operator ==( value_type const * p1, marked_ptr p2 ) CDS_NOEXCEPT
            {
                return p1 == p2.ptr() ;
            }

            /// Comparing two marked pointer including its mark bits
            friend bool operator !=( marked_ptr p1, marked_ptr p2 ) CDS_NOEXCEPT
            {
                return p1.all() != p2.all() ;
            }

            /// Comparing marked pointer and raw pointer, mark bits of \p p1 is ignored
            friend bool operator !=( marked_ptr p1, value_type const * p2 ) CDS_NOEXCEPT
            {
                return p1.ptr() != p2 ;
            }

            /// Comparing marked pointer and raw pointer, mark bits of \p p2 is ignored
            friend bool operator !=( value_type const * p1, marked_ptr p2 ) CDS_NOEXCEPT
            {
                return p1 != p2.ptr() ;
            }

            //@cond
            /// atomic< marked_ptr< T, Bitmask > > support
            T *& impl_ref() CDS_NOEXCEPT
            {
                return m_ptr ;
            }
            //@endcond
        };
    }   // namespace details

    //@cond
    /// atomic specialization for marked pointer
    /*
    template <typename T, int Bitmask >
    class atomic< details::marked_ptr< T, Bitmask > >
    {
    public:
        typedef details::marked_ptr< T, Bitmask >    marked_ptr_type ;  ///< Marked pointer type

    protected:
        marked_ptr_type volatile            m_marked_ptr    ;

    public:
        typedef typename marked_ptr_type::value_type         value_type      ;  ///< value type
        typedef typename marked_ptr_type::pointer_type       pointer_type    ;  ///< pointer type
    public:
        atomic()
            : m_marked_ptr()
        {}

        explicit atomic( value_type * p )
            : m_marked_ptr( p )
        {}

        atomic( value_type * ptr, int nMask )
            : m_marked_ptr( ptr, nMask )
        {}

        explicit atomic( const marked_ptr_type&  ptr )
            : m_marked_ptr( ptr )
        {}

    public:
        /// Read marked pointer
        marked_ptr_type load( memory_order order ) volatile const
        {
            return atomics::load( m_marked_ptr, order )  ;
        }

        /// Read marked pointer
        template <typename Order>
        marked_ptr_type load() volatile const
        {
            return atomics::load<Order>( &m_marked_ptr )  ;
        }

        /// Store marked pointer
        void store( value_type * p, memory_order order ) volatile
        {
            atomics::store( &m_marked_ptr, marked_ptr_type(p), order )  ;
        }

        /// Store marked pointer
        void store( marked_ptr_type p, memory_order order ) volatile
        {
            atomics::store( &m_marked_ptr, p, order )  ;
        }

        /// Store marked pointer value
        template <typename Order>
        void store( value_type * p ) volatile
        {
            return atomics::store<Order>( &m_marked_ptr, marked_ptr_type(p) ) ;
        }

        /// Store marked pointer value
        template <typename Order>
        void store( marked_ptr_type p ) volatile
        {
            return atomics::store<Order>( &m_marked_ptr, p ) ;
        }

        /// CAS
        bool cas( value_type * expected, value_type * desired, memory_order success_order, memory_order failure_order ) volatile
        {
            return atomics::cas( &m_marked_ptr, marked_ptr_type(expected), marked_ptr_type(desired), success_order, failure_order ) ;
        }

        /// CAS
        template <typename SuccessOrder>
        bool cas( value_type * expected, value_type * desired ) volatile
        {
            return atomics::cas<SuccessOrder>( &m_marked_ptr, marked_ptr_type(expected), marked_ptr_type(desired) ) ;
        }

        /// CAS
        template <typename SuccessOrder>
        bool cas( marked_ptr_type expected, marked_ptr_type desired ) volatile
        {
            return atomics::cas<SuccessOrder>( &m_marked_ptr, expected, desired ) ;
        }

        /// Valued CAS
        marked_ptr_type vcas( value_type * expected, value_type * desired, memory_order success_order, memory_order failure_order ) volatile
        {
            return atomics::vcas( &m_marked_ptr, marked_ptr_type(expected), marked_ptr_type(desired), success_order, failure_order ) ;
        }

        /// Valued CAS
        template <typename SuccessOrder>
        marked_ptr_type vcas( value_type * expected, value_type * desired ) volatile
        {
            return atomics::vcas<SuccessOrder>( &m_marked_ptr, marked_ptr_type(expected), marked_ptr_type(desired) ) ;
        }

        /// Valued CAS
        template <typename SuccessOrder>
        marked_ptr_type vcas( marked_ptr_type expected, marked_ptr_type desired ) volatile
        {
            return atomics::vcas<SuccessOrder>( &m_marked_ptr, expected, desired ) ;
        }

        /// Pointer exchange
        marked_ptr_type xchg( value_type * p, memory_order order ) volatile
        {
            return atomics::exchange( &m_marked_ptr, marked_ptr_type(p), order ) ;
        }

        /// Pointer exchange
        template <typename Order>
        marked_ptr_type xchg( value_type * p ) volatile
        {
            return atomics::exchange<Order>( &m_marked_ptr, marked_ptr_type(p) ) ;
        }

        /// Pointer exchange
        template <typename Order>
        marked_ptr_type xchg( marked_ptr_type p ) volatile
        {
            return atomics::exchange<Order>( &m_marked_ptr, p ) ;
        }
    };
    */
    //@endcond
}   // namespace cds

//@cond
CDS_CXX11_ATOMIC_BEGIN_NAMESPACE

    template <typename T, int Bitmask >
    class atomic< cds::details::marked_ptr<T, Bitmask> >
    {
    private:
        typedef cds::details::marked_ptr<T, Bitmask> marked_ptr ;
        typedef CDS_ATOMIC::atomic<T *>  atomic_impl ;

        atomic<T *> m_atomic ;
    public:
        bool is_lock_free() const volatile CDS_NOEXCEPT
        {
            return m_atomic.is_lock_free() ;
        }
        bool is_lock_free() const CDS_NOEXCEPT
        {
            return m_atomic.is_lock_free() ;
        }

        void store(marked_ptr val, memory_order order = memory_order_seq_cst) volatile CDS_NOEXCEPT
        {
            m_atomic.store( val.all(), order ) ;
        }
        void store(marked_ptr val, memory_order order = memory_order_seq_cst) CDS_NOEXCEPT
        {
            m_atomic.store( val.all(), order ) ;
        }

        marked_ptr load(memory_order order = memory_order_seq_cst) const volatile CDS_NOEXCEPT
        {
            return marked_ptr( m_atomic.load( order )) ;
        }
        marked_ptr load(memory_order order = memory_order_seq_cst) const CDS_NOEXCEPT
        {
            return marked_ptr( m_atomic.load( order )) ;
        }

        operator marked_ptr() const volatile CDS_NOEXCEPT
        {
            return load()   ;
        }
        operator marked_ptr() const CDS_NOEXCEPT
        {
            return load()   ;
        }

        marked_ptr exchange(marked_ptr val, memory_order order = memory_order_seq_cst) volatile CDS_NOEXCEPT
        {
            return marked_ptr( m_atomic.exchange( val.all(), order )) ;
        }
        marked_ptr exchange(marked_ptr val, memory_order order = memory_order_seq_cst) CDS_NOEXCEPT
        {
            return marked_ptr( m_atomic.exchange( val.all(), order )) ;
        }

        bool compare_exchange_weak(marked_ptr& expected, marked_ptr desired, memory_order success_order, memory_order failure_order) volatile CDS_NOEXCEPT
        {
            return m_atomic.compare_exchange_weak( expected.impl_ref(), desired.all(), success_order, failure_order ) ;
        }
        bool compare_exchange_weak(marked_ptr& expected, marked_ptr desired, memory_order success_order, memory_order failure_order) CDS_NOEXCEPT
        {
            return m_atomic.compare_exchange_weak( expected.impl_ref(), desired.all(), success_order, failure_order ) ;
        }
        bool compare_exchange_strong(marked_ptr& expected, marked_ptr desired, memory_order success_order, memory_order failure_order) volatile CDS_NOEXCEPT
        {
            return m_atomic.compare_exchange_strong( expected.impl_ref(), desired.all(), success_order, failure_order ) ;
        }
        bool compare_exchange_strong(marked_ptr& expected, marked_ptr desired, memory_order success_order, memory_order failure_order) CDS_NOEXCEPT
        {
            return m_atomic.compare_exchange_strong( expected.impl_ref(), desired.all(), success_order, failure_order ) ;
        }
        bool compare_exchange_weak(marked_ptr& expected, marked_ptr desired, memory_order success_order = memory_order_seq_cst) volatile CDS_NOEXCEPT
        {
            return m_atomic.compare_exchange_weak( expected.impl_ref(), desired.all(), success_order ) ;
        }
        bool compare_exchange_weak(marked_ptr& expected, marked_ptr desired, memory_order success_order = memory_order_seq_cst) CDS_NOEXCEPT
        {
            return m_atomic.compare_exchange_weak( expected.impl_ref(), desired.all(), success_order ) ;
        }
        bool compare_exchange_strong(marked_ptr& expected, marked_ptr desired, memory_order success_order = memory_order_seq_cst) volatile CDS_NOEXCEPT
        {
            return m_atomic.compare_exchange_strong( expected.impl_ref(), desired.all(), success_order ) ;
        }
        bool compare_exchange_strong(marked_ptr& expected, marked_ptr desired, memory_order success_order = memory_order_seq_cst) CDS_NOEXCEPT
        {
            return m_atomic.compare_exchange_strong( expected.impl_ref(), desired.all(), success_order ) ;
        }

#   ifdef CDS_CXX11_EXPLICITLY_DEFAULTED_FUNCTION_SUPPORT
        atomic() = default;
#   else
        atomic()
        {}
#   endif
        CDS_CONSTEXPR explicit atomic(marked_ptr val) CDS_NOEXCEPT
            : m_atomic( val.all() )
        {}
        CDS_CONSTEXPR explicit atomic(T * p) CDS_NOEXCEPT
            : m_atomic( p )
        {}

#   ifdef CDS_CXX11_DELETE_DEFINITION_SUPPORT
        atomic(const atomic&) = delete;
        atomic& operator=(const atomic&) = delete;
        atomic& operator=(const atomic&) volatile = delete;
#   endif

        marked_ptr operator=(marked_ptr val) volatile CDS_NOEXCEPT
        {
            store( val )    ;
            return val      ;
        }
        marked_ptr operator=(marked_ptr val) CDS_NOEXCEPT
        {
            store( val )    ;
            return val      ;
        }
    };

CDS_CXX11_ATOMIC_END_NAMESPACE
//@endcond

#endif  // #ifndef __CDS_DETAILS_MARKUP_PTR_H

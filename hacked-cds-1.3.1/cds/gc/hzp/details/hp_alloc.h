/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_GC_HZP_DETAILS_HP_ALLOC_H
#define __CDS_GC_HZP_DETAILS_HP_ALLOC_H

#include <cds/cxx11_atomic.h>
#include <cds/details/allocator.h>
#include <cds/gc/hzp/details/hp_fwd.h>
#include <cds/gc/hzp/details/hp_type.h>

//@cond
namespace cds {
    namespace gc { namespace hzp {
    /// Hazard Pointer schema implementation details
    namespace details {

        /// Hazard pointer guard
        /**
            It is unsafe to use this class directly.
            Instead, the AutoHPGuard class should be used.

            Template parameter:
                \li HazardPointer - type of hazard pointer. It is \ref hazard_pointer for Michael's Hazard Pointer reclamation schema
        */
        template <typename HazardPointer>
        class HPGuardT: protected CDS_ATOMIC::atomic<HazardPointer>
        {
        public:
            typedef HazardPointer   hazard_ptr ;    ///< Hazard pointer type
        private:
            //@cond
            typedef CDS_ATOMIC::atomic<hazard_ptr>  base_class  ;
            //@endcond

        protected:
            //@cond
            template <typename OtherHazardPointer, class Allocator> friend class HPAllocator    ;
            //@endcond

        public:
            HPGuardT()
                : base_class( null_ptr<hazard_ptr>() )
            {}
            ~HPGuardT()
            {}

            /// Sets HP value. Guards pointer \p p from reclamation.
            /**
                Storing has release semantics.
            */
            template <typename T>
            T * operator =( T * p )
            {
                // We use atomic store with explicit memory order because other threads may read this hazard pointer concurrently
                base_class::store( reinterpret_cast<hazard_ptr>(p), CDS_ATOMIC::memory_order_release )  ;
                return p    ;
            }

            /// Returns current value of hazard pointer
            /**
                Loading has acquire semantics
            */
            operator hazard_ptr() const CDS_NOEXCEPT
            {
                return get() ;
            }

            /// Returns current value of hazard pointer
            /**
                Loading has acquire semantics
            */
            hazard_ptr get() const CDS_NOEXCEPT
            {
                return base_class::load( CDS_ATOMIC::memory_order_acquire ) ;
            }

            /// Clears HP
            /**
                Clearing has relaxed semantics.
            */
            void clear()
            {
                // memory order is not necessary here
                base_class::store( null_ptr<hazard_ptr>(), CDS_ATOMIC::memory_order_relaxed )  ;
                //CDS_COMPILER_RW_BARRIER ;
            }
        };

        /// Specialization of HPGuardT for hazard_pointer type
        typedef HPGuardT<hazard_pointer> HPGuard    ;

        /// Array of hazard pointers.
        /**
            Array of hazard-pointer. Placing a pointer into this array guards the pointer against reclamation.
            Template parameter \p Count defines the size of hazard pointer array. \p Count parameter should not exceed
            GarbageCollector::getHazardPointerCount().

            It is unsafe to use this class directly. Instead, the AutoHPArray should be used.

            While creating the object of HPArray class an array of size \p Count of hazard pointers is reserved by
            the HP Manager of current thread. The object's destructor cleans all of reserved hazard pointer and
            returns reserved HP to the HP pool of ThreadGC.

            Usually, it is not necessary to create an object of this class. The object of class ThreadGC contains
            the HPArray object and implements interface for HP setting and freeing.

            Template parameter:
                \li HazardPointer - type of hazard pointer. It is hazard_pointer usually
                \li Count - capacity of array

        */
        template <typename HazardPointer, size_t Count>
        class HPArrayT
        {
        public:
            typedef HazardPointer   hazard_ptr_type ;   ///< Hazard pointer type
            typedef HPGuardT<hazard_ptr_type>   atomic_hazard_ptr    ; ///< Element type of the array
            static const size_t c_nCapacity = Count ;   ///< Capacity of the array

        private:
            //@cond
            atomic_hazard_ptr *     m_arr               ;   ///< Hazard pointer array of size = Count
            template <typename OtherHazardPointer, class Allocator> friend class HPAllocator    ;
            //@endcond

        public:
            /// Constructs uninitialized array.
            HPArrayT()
            {}

            /// Destructs object
            ~HPArrayT()
            {}

            /// Returns max count of hazard pointer for this array
            CDS_CONSTEXPR size_t capacity() const
            {
                return c_nCapacity  ;
            }

            /// Set hazard pointer \p nIndex. 0 <= \p nIndex < \p Count
            void set( size_t nIndex, hazard_ptr_type hzPtr )
            {
                assert( nIndex < capacity() )    ;
                m_arr[nIndex] = hzPtr   ;
            }

            /// Returns reference to hazard pointer of index \p nIndex (0 <= \p nIndex < \p Count)
            atomic_hazard_ptr& operator []( size_t nIndex )
            {
                assert( nIndex < capacity() )   ;
                return m_arr[nIndex]            ;
            }

            /// Returns reference to hazard pointer of index \p nIndex (0 <= \p nIndex < \p Count) [const version]
            atomic_hazard_ptr& operator []( size_t nIndex ) const
            {
                assert( nIndex < capacity() )    ;
                return m_arr[nIndex]        ;
            }

            /// Clears (sets to NULL) hazard pointer \p nIndex
            void clear( size_t nIndex )
            {
                assert( nIndex < capacity() )        ;
                m_arr[ nIndex ].clear() ;
                //CDS_COMPILER_RW_BARRIER         ;
            }
        };

        /// Specialization of HPArrayT class for hazard_pointer type
#ifdef CDS_CXX11_TEMPLATE_ALIAS_SUPPORT
        template <size_t Count> using HPArray = HPArrayT<hazard_pointer, Count > ;
#else
        template <size_t Count>
        class HPArray: public HPArrayT<hazard_pointer, Count>
        {};
#endif

        /// Allocator of hazard pointers for the thread
        /**
            The hazard pointer array is the free-list of unused hazard pointer for the thread.
            The array is managed as a stack.
            The max size (capacity) of array is defined at ctor time and cannot be changed during object's lifetime

            Each allocator object is thread-private.

            Template parameters:
                \li HazardPointer - type of hazard pointer (hazard_pointer usually)
                \li Allocator - memory allocator class, default is \ref CDS_DEFAULT_ALLOCATOR

            This helper class should not be used directly.
        */
        template < typename HazardPointer, class Allocator = CDS_DEFAULT_ALLOCATOR >
        class HPAllocator
        {
        public:
            typedef HazardPointer               hazard_ptr_type     ;   ///< type of hazard pointer
            typedef HPGuardT<hazard_ptr_type>   atomic_hazard_ptr   ;   ///< Atomic hazard pointer type
            typedef Allocator                   allocator_type      ;   ///< allocator type

        private:
            //@cond
            typedef cds::details::Allocator< atomic_hazard_ptr, allocator_type > allocator_impl ;

            atomic_hazard_ptr * m_arrHazardPtr  ;   ///< Array of hazard pointers
            size_t              m_nTop          ;   ///< The top of stack
            const size_t        m_nCapacity     ;   ///< Array capacity

            //@endcond

        public:
            /// Default ctor
            explicit HPAllocator(
                size_t  nCapacity            ///< max count of hazard pointer per thread
                )
                : m_arrHazardPtr( alloc_array( nCapacity ) )
                , m_nCapacity( nCapacity )
            {
                make_free()    ;
            }

            /// Dtor
            ~HPAllocator()
            {
                allocator_impl a ;
                a.Delete( m_arrHazardPtr, capacity() )  ;
            }

            /// Get capacity of array
            size_t capacity() const CDS_NOEXCEPT
            {
                return m_nCapacity  ;
            }

            /// Get size of array. The size is equal to the capacity of array
            size_t size() const CDS_NOEXCEPT
            {
                return capacity()    ;
            }

            /// Checks if all items are allocated
            bool isFull() const CDS_NOEXCEPT
            {
                return m_nTop == 0;
            }

            /// Allocates hazard pointer
            atomic_hazard_ptr& alloc()
            {
                assert( m_nTop > 0 )    ;
                --m_nTop    ;
                return m_arrHazardPtr[m_nTop]  ;
            }

            /// Frees previously allocated hazard pointer
            void free( atomic_hazard_ptr& hp )
            {
                assert( m_nTop < capacity() )       ;
                hp.clear()  ;
                ++m_nTop    ;
                CDS_COMPILER_RW_BARRIER ;   // ???
            }

            /// Allocates hazard pointers array
            /**
                Allocates \p Count hazard pointers from array \p m_arrHazardPtr
                Returns initialized object \p arr
            */
            template <size_t Count>
            void alloc( HPArrayT<hazard_ptr_type, Count>& arr )
            {
                assert( m_nTop >= Count )   ;
                m_nTop -= Count ;
                arr.m_arr = m_arrHazardPtr + m_nTop   ;
            }

            /// Frees hazard pointer array
            /**
                Frees the array of hazard pointers allocated by previous call \p this->alloc.
            */
            template <size_t Count>
            void free( const HPArrayT<hazard_ptr_type, Count>& arr )
            {
                assert( m_nTop + Count <= capacity())   ;
                for ( size_t i = m_nTop; i < m_nTop + Count; ++i )
                    m_arrHazardPtr[ i ].clear() ;
                m_nTop += Count ;
            }

            /// Makes all HP free
            void clear()
            {
                make_free()    ;
            }

            /// Returns to i-th hazard pointer
            atomic_hazard_ptr& operator []( size_t i )
            {
                assert( i < capacity() )    ;
                return m_arrHazardPtr[i]    ;
            }

        private:
            //@cond
            void make_free()
            {
                for ( size_t i = 0; i < capacity(); ++i )
                    m_arrHazardPtr[ i ].clear() ;
                m_nTop = capacity() ;
            }

            atomic_hazard_ptr * alloc_array( size_t nCapacity )
            {
                allocator_impl a    ;
                return a.NewArray( nCapacity )  ;
            }
            //@endcond
        };


    }}} // namespace gc::hzp::details
}   // namespace cds
//@endcond

#endif // #ifndef __CDS_GC_HZP_DETAILS_HP_ALLOC_H

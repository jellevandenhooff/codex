/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_COMPILER_GCC_AMD64_CXX11_ATOMIC_H
#define __CDS_COMPILER_GCC_AMD64_CXX11_ATOMIC_H
//@cond

#include <cstdint>
#include <cds/compiler/gcc/x86/cxx11_atomic32.h>

namespace cds { namespace cxx11_atomics {
    namespace platform { CDS_CXX11_INLINE_NAMESPACE namespace gcc { CDS_CXX11_INLINE_NAMESPACE namespace amd64 {
#   ifndef CDS_CXX11_INLINE_NAMESPACE_SUPPORT
        // primitives up to 32bit + fences
        using namespace cds::cxx11_atomics::platform::gcc::x86 ;
#   endif

        //*****************************************************************************
        // 64bit primitives
        //*****************************************************************************

        template <typename T>
        static inline bool cas64_strong( T volatile * pDest, T& expected, T desired, memory_order mo_success, memory_order mo_fail ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T) == 8, "Illegal size of operand" )   ;
            assert( cds::details::is_aligned( pDest, 8 ))  ;

            T prev = expected;
            fence_before(mo_success);
            __asm__ __volatile__ (
                "lock ; cmpxchgq %[desired], %[pDest]"
                : [prev] "+a" (prev), [pDest] "+m" (*pDest)
                : [desired] "r" (desired)
                );
            bool success = (prev == expected) ;
            expected = prev ;
            if (success)
                fence_after(mo_success);
            else
                fence_after(mo_fail);
            return success  ;
        }

        template <typename T>
        static inline bool cas64_weak( T volatile * pDest, T& expected, T desired, memory_order mo_success, memory_order mo_fail ) CDS_NOEXCEPT
        {
            return cas64_strong( pDest, expected, desired, mo_success, mo_fail ) ;
        }

        template <typename T>
        static inline T load64( T volatile const * pSrc, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T) == 8, "Illegal size of operand" )   ;
            assert( order ==  memory_order_relaxed
                || order ==  memory_order_consume
                || order ==  memory_order_acquire
                || order ==  memory_order_seq_cst
                ) ;
            assert( pSrc != NULL )  ;
            assert( cds::details::is_aligned( pSrc, 8 ))  ;

            T v = *pSrc ;
            fence_after_load( order )               ;
            return v    ;
        }


        template <typename T>
        static inline T exchange64( T volatile * pDest, T v, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T) == 8, "Illegal size of operand" )   ;
            assert( cds::details::is_aligned( pDest, 8 ))  ;

            fence_before(order);
            __asm__ __volatile__ (
                "xchgq %[v], %[pDest]"
                : [v] "+r" (v), [pDest] "+m" (*pDest)
                );
            fence_after(order);
            return v;
        }

        template <typename T>
        static inline void store64( T volatile * pDest, T val, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T) == 8, "Illegal size of operand" )   ;
            assert( order ==  memory_order_relaxed
                || order ==  memory_order_release
                || order == memory_order_seq_cst
                ) ;
            assert( pDest != NULL )  ;
            assert( cds::details::is_aligned( pDest, 8 ))  ;

            if (order != memory_order_seq_cst) {
                fence_before(order);
                *pDest = val ;
            }
            else {
                exchange64( pDest, val, order);
            }
        }

#       define CDS_ATOMIC_fetch64_add_defined
        template <typename T>
        static inline T fetch64_add( T volatile * pDest, T v, memory_order order) CDS_NOEXCEPT
        {
            static_assert( sizeof(T) == 8, "Illegal size of operand" )   ;
            assert( cds::details::is_aligned( pDest, 8 ))  ;

            fence_before(order);
            __asm__ __volatile__ (
                "lock ; xaddq %[v], %[pDest]"
                : [v] "+r" (v), [pDest] "+m" (*pDest)
                );
            fence_after(order);
            return v;
        }

#       define CDS_ATOMIC_fetch64_sub_defined
        template <typename T>
        static inline T fetch64_sub( T volatile * pDest, T v, memory_order order) CDS_NOEXCEPT
        {
            static_assert( sizeof(T) == 8, "Illegal size of operand" )   ;
            assert( cds::details::is_aligned( pDest, 8 ))  ;

            fence_before(order);
            __asm__ __volatile__ (
                "negq   %[v] ; \n"
                "lock ; xaddq %[v], %[pDest]"
                : [v] "+r" (v), [pDest] "+m" (*pDest)
                );
            fence_after(order);
            return v;
        }


        //*****************************************************************************
        // pointer primitives
        //*****************************************************************************

        template <typename T>
        static inline T * exchange_ptr( T * volatile * pDest, T * v, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T *) == sizeof(void *), "Illegal size of operand" )   ;

            return (T *) exchange64( (uint64_t volatile *) pDest, (uint64_t) v, order )  ;
        }

        template <typename T>
        static inline void store_ptr( T * volatile * pDest, T * src, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T *) == sizeof(void *), "Illegal size of operand" )   ;
            assert( order ==  memory_order_relaxed
                || order ==  memory_order_release
                || order == memory_order_seq_cst
                ) ;
            assert( pDest != NULL )  ;

            if ( order != memory_order_seq_cst ) {
                fence_before( order )   ;
                *pDest = src ;
            }
            else {
                exchange_ptr( pDest, src, order )   ;
            }
        }

        template <typename T>
        static inline T * load_ptr( T * volatile const * pSrc, memory_order order ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T *) == sizeof(void *), "Illegal size of operand" )   ;
            assert( order ==  memory_order_relaxed
                || order ==  memory_order_consume
                || order ==  memory_order_acquire
                || order ==  memory_order_seq_cst
                ) ;
            assert( pSrc != NULL )  ;

            T * v = *pSrc ;
            fence_after_load( order )   ;
            return v    ;
        }

        template <typename T>
        static inline bool cas_ptr_strong( T * volatile * pDest, T *& expected, T * desired, memory_order mo_success, memory_order mo_fail ) CDS_NOEXCEPT
        {
            static_assert( sizeof(T *) == sizeof(void *), "Illegal size of operand" )   ;

            return cas64_strong( (uint64_t volatile *) pDest, *reinterpret_cast<uint64_t *>( &expected ), (uint64_t) desired, mo_success, mo_fail )    ;
        }

        template <typename T>
        static inline bool cas_ptr_weak( T * volatile * pDest, T *& expected, T * desired, memory_order mo_success, memory_order mo_fail ) CDS_NOEXCEPT
        {
            return cas_ptr_strong( pDest, expected, desired, mo_success, mo_fail ) ;
        }

    }} // namespace gcc::amd64

#ifndef CDS_CXX11_INLINE_NAMESPACE_SUPPORT
    using namespace gcc::amd64 ;
#endif
    }   // namespace platform

}}  // namespace cds::cxx11_atomics

//@endcond
#endif // #ifndef __CDS_COMPILER_GCC_AMD64_CXX11_ATOMIC_H

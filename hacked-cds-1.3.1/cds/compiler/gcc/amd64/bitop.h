/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_COMPILER_GCC_AMD64_BITOP_H
#define __CDS_COMPILER_GCC_AMD64_BITOP_H

//@cond none
namespace cds {
    namespace bitop { namespace platform { namespace gcc { namespace amd64 {
        // MSB - return index (1..32) of most significant bit in nArg. If nArg == 0 return 0
#        define cds_bitop_msb32_DEFINED
        static inline int msb32( atomic32u_t nArg )
        {
            int        nRet    ;
            __asm__ __volatile__ (
                "bsrl        %[nArg], %[nRet]     ;\n\t"
                "jnz        1f                    ;\n\t"
                "xorl        %[nRet], %[nRet]    ;\n\t"
                "subl        $1, %[nRet]         ;\n\t"
                "1:"
                "addl        $1, %[nRet]         ;\n\t"
                : [nRet] "=a" (nRet)
                : [nArg] "r" (nArg)
                : "cc"
                ) ;
            return nRet    ;
        }

#        define cds_bitop_msb32nz_DEFINED
        static inline int msb32nz( atomic32u_t nArg )
        {
            assert( nArg != 0 )    ;
            int        nRet    ;
            __asm__ __volatile__ (
                "bsrl        %[nArg], %[nRet]    ;"
                : [nRet] "=a" (nRet)
                : [nArg] "r" (nArg)
                : "cc"
                ) ;
            return nRet    ;
        }

        // LSB - return index (0..31) of least significant bit in nArg. If nArg == 0 return -1U
#        define cds_bitop_lsb32_DEFINED
        static inline int lsb32( atomic32u_t nArg )
        {

            int        nRet    ;
            __asm__ __volatile__ (
                "bsfl        %[nArg], %[nRet]     ;"
                "jnz        1f        ;"
                "xorl        %[nRet], %[nRet]    ;"
                "subl        $1, %[nRet]         ;"
                "1:"
                "addl        $1, %[nRet]         ;"
                : [nRet] "=a" (nRet)
                : [nArg] "r" (nArg)
                : "cc"
                ) ;
            return nRet    ;

        }

        // LSB - return index (0..31) of least significant bit in nArg.
        // Condition: nArg != 0
#        define cds_bitop_lsb32nz_DEFINED
        static inline int lsb32nz( atomic32u_t nArg )
        {
            assert( nArg != 0 )    ;
            int        nRet    ;
            __asm__ __volatile__ (
                "bsfl        %[nArg], %[nRet]    ;"
                : [nRet] "=a" (nRet)
                : [nArg] "r" (nArg)
                : "cc"
                ) ;
            return nRet    ;
        }

#        define cds_bitop_msb64_DEFINED
        static inline int msb64( atomic64u_unaligned nArg )
        {
            atomic64u_unaligned        nRet    ;
            asm volatile (
                "bsrq        %[nArg], %[nRet]     ;\n\t"
                "jnz        1f                    ;\n\t"
                "xorq        %[nRet], %[nRet]    ;\n\t"
                "subq        $1, %[nRet]         ;\n\t"
                "1:"
                "addq        $1, %[nRet]         ;\n\t"
                : [nRet] "=a" (nRet)
                : [nArg] "r" (nArg)
                : "cc"
                ) ;
            return (int) nRet    ;
        }

#        define cds_bitop_msb64nz_DEFINED
        static inline int msb64nz( atomic64u_unaligned nArg )
        {
            assert( nArg != 0 )    ;
            atomic64u_unaligned        nRet    ;
            __asm__ __volatile__ (
                "bsrq        %[nArg], %[nRet]    ;"
                : [nRet] "=a" (nRet)
                : [nArg] "r" (nArg)
                : "cc"
                ) ;
            return (int) nRet    ;
        }

        // LSB - return index (0..31) of least significant bit in nArg. If nArg == 0 return -1U
#        define cds_bitop_lsb64_DEFINED
        static inline int lsb64( atomic64u_unaligned nArg )
        {

            atomic64u_unaligned        nRet    ;
            __asm__ __volatile__ (
                "bsfq        %[nArg], %[nRet]     ;"
                "jnz        1f        ;"
                "xorq        %[nRet], %[nRet]    ;"
                "subq        $1, %[nRet]         ;"
                "1:"
                "addq        $1, %[nRet]         ;"
                : [nRet] "=a" (nRet)
                : [nArg] "r" (nArg)
                : "cc"
                ) ;
            return (int) nRet    ;

        }

        // LSB - return index (0..31) of least significant bit in nArg.
        // Condition: nArg != 0
#        define cds_bitop_lsb64nz_DEFINED
        static inline int lsb64nz( atomic64u_unaligned nArg )
        {
            assert( nArg != 0 )    ;
            atomic64u_unaligned        nRet    ;
            __asm__ __volatile__ (
                "bsfq        %[nArg], %[nRet]    ;"
                : [nRet] "=a" (nRet)
                : [nArg] "r" (nArg)
                : "cc"
                ) ;
            return (int) nRet    ;
        }


    }} // namespace gcc::amd64

    using namespace gcc::amd64    ;

    }}}    // namespace cds::bitop::platform

//@endcond

#endif // #ifndef __CDS_COMPILER_GCC_AMD64_BITOP_H

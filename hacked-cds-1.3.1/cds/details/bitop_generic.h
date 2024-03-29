/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_DETAILS_BITOP_GENERIC_H
#define __CDS_DETAILS_BITOP_GENERIC_H

#include <stdlib.h>     // rand()
namespace cds {
    namespace bitop { namespace platform {
        // Return true if x = 2 ** k, k >= 0
#ifndef cds_bitop_isPow2_32_DEFINED
        static inline bool isPow2_32( atomic32u_t x )
        {
            return (x & ( x - 1 )) == 0    && x ;
        }
#endif

#ifndef cds_bitop_isPow2_64_DEFINED
        static inline bool isPow2_64( atomic64_unaligned x )
        {
            return (x & ( x - 1 )) == 0 && x ;
        }
#endif

        //***************************************************
        // Most significant bit number (1..N)
        // Return 0 if x == 0
        //
#ifndef cds_bitop_msb32_DEFINED
        // Return number (1..32) of most significant bit
        // Return 0 if x == 0
        // Source: Linux kernel
        static inline int msb32( atomic32u_t x )
        {
            int r = 32;

            if (!x)
                return 0;
            if (!(x & 0xffff0000u)) {
                x <<= 16;
                r -= 16;
            }
            if (!(x & 0xff000000u)) {
                x <<= 8;
                r -= 8;
            }
            if (!(x & 0xf0000000u)) {
                x <<= 4;
                r -= 4;
            }
            if (!(x & 0xc0000000u)) {
                x <<= 2;
                r -= 2;
            }
            if (!(x & 0x80000000u)) {
                x <<= 1;
                r -= 1;
            }
            return r;
        }
#endif

#ifndef cds_bitop_msb32nz_DEFINED
        static inline int msb32nz( atomic32u_t x )
        {
            return msb32( x ) - 1 ;
        }
#endif

#ifndef cds_bitop_msb64_DEFINED
        static inline int msb64( atomic64u_unaligned x )
        {
            atomic32u_t h = (atomic32u_t) (x >> 32);
            if ( h )
                return msb32( h ) + 32;
            return msb32( (atomic32u_t) x );
        }
#endif

#ifndef cds_bitop_msb64nz_DEFINED
        static inline int msb64nz( atomic64u_unaligned x )
        {
            return msb64( x ) - 1 ;
        }
#endif

        //***************************************************
        // Least significant bit number (1..N)
        // Return 0 if x == 0
        //
#ifndef cds_bitop_lsb32_DEFINED
        // Return number (1..32) of least significant bit
        // Return 0 if x == 0
        // Source: Linux kernel
        static inline int lsb32( atomic32u_t x )
        {
            int r = 1;

            if (!x)
                return 0;
            if (!(x & 0xffff)) {
                x >>= 16;
                r += 16;
            }
            if (!(x & 0xff)) {
                x >>= 8;
                r += 8;
            }
            if (!(x & 0xf)) {
                x >>= 4;
                r += 4;
            }
            if (!(x & 3)) {
                x >>= 2;
                r += 2;
            }
            if (!(x & 1)) {
                x >>= 1;
                r += 1;
            }
            return r;
        }
#endif

#ifndef cds_bitop_lsb32nz_DEFINED
        static inline int lsb32nz( atomic32u_t x )
        {
            return lsb32( x ) - 1 ;
        }
#endif

#ifndef cds_bitop_lsb64_DEFINED
        static inline int lsb64( atomic64u_unaligned x )
        {
            if ( !x )
                return 0    ;
            if ( x & 0xffffffffu )
                return lsb32( (atomic32u_t) x )    ;
            return lsb32( (atomic32u_t) (x >> 32) ) + 32    ;
        }
#endif

#ifndef cds_bitop_lsb64nz_DEFINED
        static inline int lsb64nz( atomic64u_unaligned x )
        {
            return lsb64( x ) - 1 ;
        }
#endif

        //******************************************************
        // Reverse bit order
        //******************************************************
#ifndef cds_bitop_rbo32_DEFINED
        static inline atomic32u_t rbo32( atomic32u_t x )
        {
            // swap odd and even bits
            x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
            // swap consecutive pairs
            x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
            // swap nibbles ...
            x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
            // swap bytes
            x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
            // swap 2-byte long pairs
            return ( x >> 16 ) | ( x << 16 );
        }
#endif

#ifndef cds_bitop_rbo64_DEFINED
        static inline atomic64u_t rbo64( atomic64u_unaligned x )
        {
            //                      Low 32bit                                          Hight 32bit
            return ( ((atomic64u_t) rbo32( (atomic32u_t) x )) << 32 ) | ((atomic64u_t) rbo32( (atomic32u_t) (x >> 32) )) ;
        }
#endif

        //******************************************************
        // Set bit count. Return count of non-zero bits in word
        //******************************************************
#ifndef cds_bitop_sbc32_DEFINED
        static inline int sbc32( atomic32u_t x )
        {
#        ifdef cds_beans_zbc32_DEFINED
            return 32 - zbc32( x )    ;
#        else
            // Algorithm from Sean Eron Anderson's great collection
            x = x - ((x >> 1) & 0x55555555);
            x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
            return (((x + (x >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24    ;
#        endif
        }
#endif

#ifndef cds_bitop_sbc64_DEFINED
        static inline int sbc64( atomic64u_unaligned x )
        {
#        ifdef cds_beans_zbc64_DEFINED
            return 64 - zbc64( x )    ;
#        else
            return sbc32( (atomic32u_t) (x >> 32) ) + sbc32( (atomic32u_t) x )    ;
#        endif
        }
#endif

        //******************************************************
        // Zero bit count. Return count of zero bits in word
        //******************************************************
#ifndef cds_bitop_zbc32_DEFINED
        static inline int zbc32( atomic32u_t x )
        {
            return 32 - sbc32( x ) ;
        }
#endif

#ifndef cds_bitop_zbc64_DEFINED
        static inline int zbc64( atomic64u_unaligned x )
        {
            return 64 - sbc64( x ) ;
        }
#endif

        /*
            Simple random number generator
            Source:
                [2003] George Marsaglia "Xorshift RNGs"
        */
        static inline uint32_t RandXorShift32(uint32_t x)
        {
            //static atomic32u_t xRandom = 2463534242UL    ;    //rand() | 0x0100    ;    // must be nonzero
            //atomic32u_t x = xRandom    ;
            if ( !x )
                x = ((rand() + 1) << 16) + rand() + 1 ;
            x ^= x << 13        ;
            x ^= x >> 15        ;
            return x ^= x << 5    ;
        }

        static inline uint64_t RandXorShift64(uint64_t x)
        {
            //static atomic64u_t xRandom = 88172645463325252LL;
            //atomic64u_t x = xRandom    ;
            if ( !x )
                x = 88172645463325252LL    ;
            x ^= x << 13;
            x ^= x >> 7;
            return x ^= x << 17 ;
        }
    }}    // namespace bitop::platform
} // namespace cds

#endif    // __CDS_DETAILS_BITOP_GENERIC_H

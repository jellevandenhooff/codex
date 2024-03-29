/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_BITOP_H
#define __CDS_BITOP_H

/*
    Different bit algorithms:
        LSB        get least significant bit number
        MSB        get most significant bit number
        bswap    swap byte order of word
        RBO        reverse bit order of word

    Editions:
        2007.10.08    Maxim.Khiszinsky    Created
*/

#include <cds/details/defs.h>
#include <cds/compiler/bitop.h>

namespace cds {
    /// Bit operations
    namespace bitop {

        ///@cond none
        namespace details {
            template <int> struct BitOps    ;

            // 32-bit bit ops
            template <> struct BitOps<4> {
                typedef atomic32u_t        TUInt    ;

                static int      MSB( TUInt x )      { return bitop::platform::msb32( x );   }
                static int      LSB( TUInt x )      { return bitop::platform::lsb32( x );   }
                static int      MSBnz( TUInt x )    { return bitop::platform::msb32nz( x ); }
                static int      LSBnz( TUInt x )    { return bitop::platform::lsb32nz( x ); }
                static int      SBC( TUInt x )      { return bitop::platform::sbc32( x ) ;  }
                static int      ZBC( TUInt x )      { return bitop::platform::zbc32( x ) ;  }

                static TUInt    RBO( TUInt x )      { return bitop::platform::rbo32( x );   }

                static TUInt    RandXorShift(TUInt x) { return bitop::platform::RandXorShift32(x); }
            };

            // 64-bit bit ops
            template <> struct BitOps<8> {
                typedef atomic64u_unaligned        TUInt    ;

                static int      MSB( TUInt x )        { return bitop::platform::msb64( x );     }
                static int      LSB( TUInt x )        { return bitop::platform::lsb64( x );     }
                static int      MSBnz( TUInt x )      { return bitop::platform::msb64nz( x );   }
                static int      LSBnz( TUInt x )      { return bitop::platform::lsb64nz( x );   }
                static  int     SBC( TUInt x )        { return bitop::platform::sbc64( x ) ;    }
                static  int     ZBC( TUInt x )        { return bitop::platform::zbc64( x ) ;    }

                static TUInt    RBO( TUInt x )        { return bitop::platform::rbo64( x );     }

                static TUInt    RandXorShift(TUInt x) { return bitop::platform::RandXorShift64(x); }
            };
        }    // namespace details
        //@endcond


        /// Get least significant bit (LSB) number (1..32/64), 0 if nArg == 0
        template <typename T>
        static inline int LSB( T nArg )
        {
            return details::BitOps< sizeof(T) >::LSB( (typename details::BitOps<sizeof(T)>::TUInt) nArg )    ;
        }

        /// Get least significant bit (LSB) number (0..31/63)
        /**
            Precondition: nArg != 0
        */
        template <typename T>
        static inline int LSBnz( T nArg )
        {
            assert( nArg != 0 )    ;
            return details::BitOps< sizeof(T) >::LSBnz( (typename details::BitOps<sizeof(T)>::TUInt) nArg )    ;
        }

        /// Get most significant bit (MSB) number (1..32/64), 0 if nArg == 0
        template <typename T>
        static inline int MSB( T nArg )
        {
            return details::BitOps< sizeof(T) >::MSB( (typename details::BitOps<sizeof(T)>::TUInt) nArg )    ;
        }

        /// Get most significant bit (MSB) number (0..31/63)
        /**
            Precondition: nArg != 0
        */
        template <typename T>
        static inline int MSBnz( T nArg )
        {
            assert( nArg != 0 )    ;
            return details::BitOps< sizeof(T) >::MSBnz( (typename details::BitOps<sizeof(T)>::TUInt) nArg )    ;
        }

        /// Get non-zero bit count of a word
        template <typename T>
        static inline int SBC( T nArg )
        {
            return details::BitOps< sizeof(T) >::SBC( (typename details::BitOps<sizeof(T)>::TUInt) nArg ) ;
        }

        /// Get zero bit count of a word
        template <typename T>
        static inline int ZBC( T nArg )
        {
            return details::BitOps< sizeof(T) >::ZBC( (typename details::BitOps<sizeof(T)>::TUInt) nArg ) ;
        }

        /// Reverse bit order of \p nArg
        template <typename T>
        static inline T RBO( T nArg )
        {
            return (T) details::BitOps< sizeof(T) >::RBO( (typename details::BitOps<sizeof(T)>::TUInt) nArg )    ;
        }

        /// Simple random number generator
        template <typename T>
        static inline T RandXorShift( T x)
        {
            return (T) details::BitOps< sizeof(T) >::RandXorShift(x) ;
        }

    } // namespace bitop
} //namespace cds

#endif    // #ifndef __CDS_BITOP_H


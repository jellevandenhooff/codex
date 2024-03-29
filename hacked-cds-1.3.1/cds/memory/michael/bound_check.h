/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_MEMORY_MICHAEL_ALLOCATOR_BOUND_CHECK_H
#define __CDS_MEMORY_MICHAEL_ALLOCATOR_BOUND_CHECK_H

#include <cds/opt/options.h>
#include <exception>
#include <memory.h>

namespace cds { namespace memory { namespace michael {

    //@cond
    namespace details {
        class bound_checker
        {
        protected:
            typedef atomic64u_t trailer_type    ;
            static const trailer_type s_BoundCheckerTrailer = 0xbadcafeedeadc0feULL ;

        public:
            enum {
                trailer_size = sizeof(trailer_type) + sizeof(size_t)
            };

            void make_trailer( void * pStartArea, void * pEndBlock, size_t nAllocSize )
            {
                char * pArea = reinterpret_cast<char *>(pStartArea) ;
                assert( reinterpret_cast<char *>(pEndBlock) - (pArea + nAllocSize) >= trailer_size )  ;

                trailer_type trailer = s_BoundCheckerTrailer        ;
                memcpy( pArea + nAllocSize, &trailer, sizeof(trailer) ) ;

                // the next assignment is correct because pBlock is at least sizeof(size_t)-byte aligned
                assert( (reinterpret_cast<uptr_atomic_t>(pEndBlock) & (sizeof(size_t) - 1)) == 0 )  ;
                *(reinterpret_cast<size_t *>( pEndBlock ) - 1) = nAllocSize   ;
            }

            bool check_bounds( void * pStartArea, void * pEndBlock, size_t nBlockSize )
            {
                trailer_type trailer = s_BoundCheckerTrailer        ;
                size_t nAllocSize = *(reinterpret_cast<size_t *>( pEndBlock ) - 1) ;

                assert( nAllocSize < nBlockSize )   ;
                return nAllocSize < nBlockSize
                    && memcmp( reinterpret_cast<char *>(pStartArea) + nAllocSize, &trailer, sizeof(trailer) ) == 0 ;
            }
        };
    }
    //@endcond

#if defined(CDS_DOXYGEN_INVOKED) || defined(_DEBUG)
    /// Debug bound checker
    /**
        This is one of value of opt::check_bounds option for Michael's \ref Heap memory allocator.
        It is intended for debug mode only. It throws an assertion when memory bound violation is detected.
        In release mode it is equal to <tt>opt::check_bounds<cds::opt::none> </tt>.
    */
    class debug_bound_checking: public details::bound_checker
    {
    //@cond
        typedef details::bound_checker  base_class  ;
    public:
        void check_bounds( void * pStartArea, void * pEndBlock, size_t nBlockSize )
        {
            // Bound checking assertion
            assert( base_class::check_bounds( pStartArea, pEndBlock, nBlockSize ) ) ;
        }

    //@endcond
    };
#else
    typedef cds::opt::none  debug_bound_checking    ;
#endif

    /// %Exception of \ref strong_bound_checking bound checker
    class bound_checker_exception: public std::exception
    {
    //@cond
    public:
        virtual const char * what() const throw()
        {
            return "Memory bound checking violation"   ;
        }
    //@endcond
    };

    /// %Exception throwing bound checker
    /**
        This is one of value of opt::check_bounds option for Michael's \ref Heap memory allocator.
        It is intended for debug and release mode.
        When memory bound violation is detected
            \li In debug mode - an assertion is raised
            \li In release mode  - an exception of type \ref bound_checker_exception is thrown
    */
    class strong_bound_checking: public details::bound_checker
    {
    //@cond
        typedef details::bound_checker  base_class  ;
    public:
        void check_bounds( void * pStartArea, void * pEndBlock, size_t nBlockSize )
        {
            if ( !base_class::check_bounds( pStartArea, pEndBlock, nBlockSize ) ) {
                throw bound_checker_exception() ;
            }
        }

    //@endcond
    };


    //@cond
    namespace details {
        template <typename BOUND_CHECKER>
        class bound_checker_selector: public BOUND_CHECKER
        {
            typedef BOUND_CHECKER base_class    ;
        public:
            enum {
                trailer_size = base_class::trailer_size
            };

            void make_trailer( void * pStartArea, void * pEndBlock, size_t nAllocSize )
            {
                base_class::make_trailer( pStartArea, pEndBlock, nAllocSize )  ;
            }

            void check_bounds( void * pStartArea, void * pEndBlock, size_t nBlockSize )
            {
                base_class::check_bounds( pStartArea, pEndBlock, nBlockSize ) ;
            }
        };

        template <>
        class bound_checker_selector<cds::opt::none>
        {
        public:
            enum {
                trailer_size = 0
            };

            void make_trailer( void * /*pStartArea*/, void * /*pEndBlock*/, size_t /*nAllocSize*/ )
            {}

            void check_bounds( void * /*pStartArea*/, void * /*pEndBlock*/, size_t /*nBlockSize*/ )
            {}
        };
    }   // namespace details
    //@endcond


}}} // namespace cds::memory::michael

#endif // #ifndef __CDS_MEMORY_MICHAEL_ALLOCATOR_BOUND_CHECK_H

/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_THREADING_DETAILS_GCC_MANAGER_H
#define __CDS_THREADING_DETAILS_GCC_MANAGER_H

#if !( CDS_COMPILER == CDS_COMPILER_GCC || CDS_COMPILER == CDS_COMPILER_CLANG )
#   error "threading/details/gccmanager.h may be used only with GCC or Clang C++ compiler"
#endif

#error "using gcc thread manager..."

#include <cds/threading/details/_common.h>

//@cond
namespace cds { namespace threading {

    /// cds::threading::Manager implementation based on GCC __thread declaration
    CDS_CXX11_INLINE_NAMESPACE namespace gcc {

        /// Thread-specific data manager based on GCC __thread feature
        class Manager {
        private :
            //@cond
            static ThreadData * _threadData()
            {
                typedef unsigned char  ThreadDataPlaceholder[ sizeof(ThreadData) ]  ;
                static __thread ThreadDataPlaceholder CDS_DATA_ALIGNMENT(8) threadData        ;

                return reinterpret_cast<ThreadData *>(threadData)   ;
            }
            //@endcond

        public:
            /// Initialize manager (empty function)
            /**
                This function is automatically called by cds::Initialize
            */
            static void init()
            {}

            /// Terminate manager (empty function)
            /**
                This function is automatically called by cds::Terminate
            */
            static void fini()
            {}

            /// Checks whether current thread is attached to \p libcds feature or not.
            static bool isThreadAttached()
            {
                ThreadData * pData = _threadData()    ;
                return pData != NULL && (pData->m_hpManager != NULL || pData->m_hrcManager != NULL || pData->m_ptbManager != NULL ) ;
            }

            /// This method must be called in beginning of thread execution
            static void attachThread()
            {
                new ( _threadData() ) ThreadData    ;

                _threadData()->init()   ;
            }

            /// This method must be called in end of thread execution
            static void detachThread()
            {
                _threadData()->fini()   ;

                _threadData()->ThreadData::~ThreadData()   ;
            }

            /// Get gc::HP thread GC implementation for current thread
            /**
                The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
                or if you did not use gc::HP.
                To initialize gc::HP GC you must constuct cds::HP object in the beginning of your application
            */
            static gc::HP::thread_gc_impl&   getHZPGC()
            {
                assert( _threadData()->m_hpManager != NULL )    ;
                return *(_threadData()->m_hpManager)            ;
            }

            /// Get gc::HRC thread GC implementation for current thread
            /**
                The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
                or if you did not use gc::HRC.
                To initialize gc::HRC GC you must constuct cds::HRC object in the beginning of your application
            */
            static gc::HRC::thread_gc_impl&   getHRCGC()
            {
                assert( _threadData()->m_hrcManager != NULL )   ;
                return *(_threadData()->m_hrcManager)           ;
            }

            /// Get gc::PTB thread GC implementation for current thread
            /**
                The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
                or if you did not use gc::PTB.
                To initialize gc::PTB GC you must constuct cds::PTB object in the beginning of your application
            */
            static gc::PTB::thread_gc_impl&   getPTBGC()
            {
                assert( _threadData()->m_ptbManager != NULL )   ;
                return *(_threadData()->m_ptbManager)           ;
            }

            //@cond
            static size_t fake_current_processor()
            {
                return _threadData()->fake_current_processor()  ;
            }
            //@endcond
        } ;

    } // namespace gcc

}} // namespace cds::threading
//@endcond

#endif // #ifndef __CDS_THREADING_DETAILS_GCC_MANAGER_H

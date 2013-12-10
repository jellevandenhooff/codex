/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_THREADING__COMMON_H
#define __CDS_THREADING__COMMON_H

#include <cds/gc/hp_decl.h>
#include <cds/gc/hrc_decl.h>
#include <cds/gc/ptb_decl.h>

namespace cds {
    /// Threading support
    /** \anchor cds_threading
        The \p CDS library requires support from the threads.
        Each garbage collector manages a control structure on the per-thread basis.
        The library does not provide any thread model. To embed the library to your application you must provide
        appropriate implementation of \p cds::threading::Manager interface. This interface manages cds::threading::ThreadData
        structure that contains thread specific data of GCs.

        Any \p cds::threading::Manager implementation is a singleton and it must be accessible from any thread and from any point of
        your application. Note that you should not mix different implementation of the \p cds::threading::Manager in your application.

        Before compiling of your application you may define one of \p CDS_THREADING_xxx macro in cds/user_setup/threading.h file:
            \li \p CDS_THREADING_AUTODETECT - auto-detect appropriate threading model for your platform and compiler. This is
                predefined value of threading model in <tt>cds/user_setup/threading.h</tt>.
            \li \p CDS_THREADING_WIN_TLS - use <tt>cds::threading::wintls::Manager</tt> implementation based on Windows TLS API.
                Intended for Windows and Microsoft Visual C++ only. This is default threading model for Windows and MS Visual C++.
            \li \p CDS_THREADING_MSVC - use <tt>cds::threading::msvc::Manager</tt> implementation based on Microsoft Visual C++ <tt>__declspec(thread)</tt>
                declaration. Intended for Windows and Microsoft Visual C++ only.
                This macro should be explicitly specified if you want to use <tt>__declspec(thread)</tt> keyword.
            \li \p CDS_THREADING_PTHREAD - use <tt>cds::threading::pthread::Manager</tt> implementation based on pthread thread-specific
                data functions \p pthread_getspecific / \p pthread_setspecific. Intended for GCC compiler.
                This is default threading model for GCC.
            \li \p CDS_THREADING_GCC - use <tt>cds::threading::gcc::Manager</tt> implementation based on GCC \p __thread
                keyword. Intended for GCC compiler only. Note, that GCC compiler supports \p __thread keyword properly
                not for all platforms and even not for all GCC version.
                This macro should be explicitly specified if you want to use \p __thread keyword.
            \li \p CDS_THREADING_USER_DEFINED - use user-provided threading model.

        These macros select appropriate implementation of \p cds::threading::Manager class. The child namespaces of cds::threading
        provide suitable implementation and import it to cds::threading by using \p using directive (or by using inline namespace if the compiler
        supports it). So, if you need to call threading manager functions directly you should refer to \p cds::threading::Manager class.

        The interface of \p cds::threading::Manager class is the following:
        \code
        class Manager {
        public:
            // Initialize manager (called by cds::Initialize() )
            static void init();

            // Terminate manager (called by cds::Terminate() )
            static void fini();

            // Checks whether current thread is attached to \p libcds feature or not.
            static bool isThreadAttached() ;

            // This method must be called in beginning of thread execution
            // (called by ctor of GC thread object, for example, by ctor of cds::gc::HP::thread_gc)
            static void attachThread()  ;

            // This method must be called in end of thread execution
            // (called by dtor of GC thread object, for example, by dtor of cds::gc::HP::thread_gc)
            static void detachThread() ;

            // Get cds::gc::HP thread GC implementation for current thread
            static gc::HP::thread_gc_impl&   getHZPGC() ;

            // Get cds::gc::HRC thread GC implementation for current thread
            static gc::HRC::thread_gc_impl&   getHRCGC() ;

            // Get cds::gc::PTB thread GC implementation for current thread ;
            static gc::PTB::thread_gc_impl&   getPTBGC() ;
        };
        \endcode

        The library's core (dynamic linked library) is free of usage of user-supplied \p cds::threading::Manager code.
        \p cds::threading::Manager is necessary for header-only part of \p CDS library (for \ref cds::threading::getGC functions).


        Each thread that uses \p libcds data structures should be attached to threading::Manager before using
        lock-free data structs.
        See \ref cds_how_to_use "How to use" section for details

        <b>Note for Windows</b>

        When you use Garbage Collectors (GC) provided by \p libcds in your dll that dynamically loaded by \p LoadLibrary then there is no way
        to use \p __declspec(thread) declaration to support threading model for \p libcds. MSDN says:

        \li If a DLL declares any nonlocal data or object as __declspec( thread ), it can cause a protection fault if dynamically loaded.
            After the DLL is loaded with \p LoadLibrary, it causes system failure whenever the code references the nonlocal __declspec( thread ) data.
            Because the global variable space for a thread is allocated at run time, the size of this space is based on a calculation of the requirements
            of the application plus the requirements of all the DLLs that are statically linked. When you use \p LoadLibrary, there is no way to extend
            this space to allow for the thread local variables declared with __declspec( thread ). Use the TLS APIs, such as TlsAlloc, in your
            DLL to allocate TLS if the DLL might be loaded with LoadLibrary.

        Thus, in case when \p libcds or a dll that depends on \p libcds is loaded dynamically by calling \p LoadLibrary explicitly,
        you may not use \p CDS_THREADING_MSVC macro.  Instead, you should build your dll projects with \p CDS_THREADING_WIN_TLS only.
    */
    namespace threading {

        //@cond
        /// Thread-specific data
        struct ThreadData {

            //@cond
            char CDS_DATA_ALIGNMENT(8) m_hpManagerPlaceholder[sizeof(cds::gc::HP::thread_gc_impl)]   ;   ///< Michael's Hazard Pointer GC placeholder
            char CDS_DATA_ALIGNMENT(8) m_hrcManagerPlaceholder[sizeof(cds::gc::HRC::thread_gc_impl)]  ;   ///< Gidenstam's GC placeholder
            char CDS_DATA_ALIGNMENT(8) m_ptbManagerPlaceholder[sizeof(cds::gc::PTB::thread_gc_impl)]  ;   ///< Pass The Buck GC placeholder
            //@endcond

            cds::gc::HP::thread_gc_impl * m_hpManager     ;   ///< Michael's Hazard Pointer GC thread-specific data
            cds::gc::HRC::thread_gc_impl * m_hrcManager    ;   ///< Gidenstam's GC thread-specific data
            cds::gc::PTB::thread_gc_impl * m_ptbManager    ;   ///< Pass The Buck GC thread-specific data

            size_t  m_nFakeProcessorNumber  ;   ///< fake "current processor" number

            //@cond
            static CDS_EXPORT_API CDS_ATOMIC::atomic<size_t> s_nLastUsedProcNo   ;
            static CDS_EXPORT_API size_t                     s_nProcCount        ;
            //@endcond

            //@cond
            ThreadData()
                : m_nFakeProcessorNumber( s_nLastUsedProcNo.fetch_add(1, CDS_ATOMIC::memory_order_relaxed) % s_nProcCount )
            {
                if (cds::gc::HP::isUsed() )
                    m_hpManager = new (m_hpManagerPlaceholder) cds::gc::HP::thread_gc_impl ;
                else
                    m_hpManager = NULL  ;

                if ( cds::gc::HRC::isUsed() )
                    m_hrcManager = new (m_hrcManagerPlaceholder) cds::gc::HRC::thread_gc_impl  ;
                else
                    m_hrcManager = NULL ;

                if ( cds::gc::PTB::isUsed() )
                    m_ptbManager = new (m_ptbManagerPlaceholder) cds::gc::PTB::thread_gc_impl  ;
                else
                    m_ptbManager = NULL ;
            }

            ~ThreadData()
            {
                if ( m_hpManager ) {
                    typedef cds::gc::HP::thread_gc_impl hp_thread_gc_impl ;
                    m_hpManager->~hp_thread_gc_impl()     ;
                    m_hpManager = NULL  ;
                }

                if ( m_hrcManager ) {
                    typedef cds::gc::HRC::thread_gc_impl hrc_thread_gc_impl ;
                    m_hrcManager->~hrc_thread_gc_impl()    ;
                    m_hrcManager = NULL ;
                }

                if ( m_ptbManager ) {
                    typedef cds::gc::PTB::thread_gc_impl ptb_thread_gc_impl ;
                    m_ptbManager->~ptb_thread_gc_impl()  ;
                    m_ptbManager = NULL     ;
                }
            }

            void init()
            {
                if ( cds::gc::HP::isUsed() )
                    m_hpManager->init()   ;
                if ( cds::gc::HRC::isUsed() )
                    m_hrcManager->init()  ;
                if ( cds::gc::PTB::isUsed() )
                    m_ptbManager->init()  ;
            }

            void fini()
            {
                if ( cds::gc::PTB::isUsed() )
                    m_ptbManager->fini()   ;
                if ( cds::gc::HRC::isUsed() )
                    m_hrcManager->fini()  ;
                if ( cds::gc::HP::isUsed() )
                    m_hpManager->fini()   ;
            }

            size_t fake_current_processor()
            {
                return m_nFakeProcessorNumber   ;
            }
            //@endcond
        };
        //@endcond

    } // namespace threading
} // namespace cds::threading

#endif // #ifndef __CDS_THREADING__COMMON_H

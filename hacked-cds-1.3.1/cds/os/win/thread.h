/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_OS_WIN_THREAD_H
#define __CDS_OS_WIN_THREAD_H

#include <windows.h>

namespace cds { namespace OS {
    /// Windows-specific functions
    namespace Win32 {

        /// OS-specific type of thread identifier
        typedef DWORD           ThreadId        ;

        /// OS-specific type of thread handle
        typedef HANDLE            ThreadHandle    ;

        /// Get null thread id
        CDS_CONSTEXPR static inline ThreadId nullThreadId()
        {
            return 0 ;
        }

        /// Get current thread id
        static inline ThreadId getCurrentThreadId()
        {
            return ::GetCurrentThreadId() ;
        }

        /// Tests whether the thread is alive
        static inline bool isThreadAlive( ThreadId id )
        {
            HANDLE h = ::OpenThread( SYNCHRONIZE, FALSE, id ) ;
            if ( h == NULL )
                return false    ;
            ::CloseHandle( h )    ;
            return true         ;
        }

        /// Yield current thread, switch to another
        static inline void yield()
        {
    #    if _WIN32_WINNT >= 0x0400
            ::SwitchToThread() ;
    #   else
            ::Sleep(0)    ;
    #   endif
        }

        /// Default backoff::yield implementation
        static inline void    backoff()
        {
            yield() ;
        }
    }    // namespace Win32

    using Win32::ThreadId       ;
    using Win32::ThreadHandle   ;

    using Win32::nullThreadId    ;
    using Win32::getCurrentThreadId    ;
    using Win32::isThreadAlive    ;
    using Win32::yield            ;
    using Win32::backoff        ;

}} // namespace cds::OS

#endif // #ifndef __CDS_OS_WIN_THREAD_H

/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_OS_THREAD_H
#define __CDS_OS_THREAD_H

#include <cds/details/defs.h>

#if CDS_OS_TYPE == CDS_OS_WIN32 || CDS_OS_TYPE == CDS_OS_WIN64 || CDS_OS_TYPE == CDS_OS_MINGW
#    include <cds/os/win/thread.h>
#elif CDS_OS_TYPE == CDS_OS_LINUX
#    include <cds/os/posix/thread.h>
#elif CDS_OS_TYPE == CDS_OS_SUN_SOLARIS
#    include <cds/os/posix/thread.h>
#elif CDS_OS_TYPE == CDS_OS_HPUX
#    include <cds/os/posix/thread.h>
#elif CDS_OS_TYPE == CDS_OS_AIX
#    include <cds/os/posix/thread.h>
#elif CDS_OS_TYPE == CDS_OS_FREE_BSD || CDS_OS_TYPE == CDS_OS_OPEN_BSD || CDS_OS_TYPE == CDS_OS_NET_BSD
#    include <cds/os/posix/thread.h>
#else
extern void RequestYield(int argument);
extern int ThreadId();

//************************************************************************
// Other OSes
//************************************************************************
namespace cds { namespace OS {
    typedef int       ThreadId    ;

    /// Null thread id constant
    CDS_CONSTEXPR static inline ThreadId nullThreadId()   { return -666 ; }

    /// Get current thread id
    static inline ThreadId getCurrentThreadId()    { return ::ThreadId(); }

    /// Checks if thread \p id is alive
    static inline bool isThreadAlive( ThreadId id )
    {
        return true;
    }

    /// Yield thread
    static inline void yield()      { RequestYield(0); }

    /// Default back-off thread strategy (yield)
    static inline void backoff()    { RequestYield(0); }
}} // namespace cds::OS
#endif

#endif    // #ifndef __CDS_OS_THREAD_H


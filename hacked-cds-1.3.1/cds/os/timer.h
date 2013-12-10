/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_OS_TIMER_H
#define __CDS_OS_TIMER_H
//@cond

#include <cds/details/defs.h>

#if CDS_OS_TYPE == CDS_OS_WIN32 || CDS_OS_TYPE == CDS_OS_WIN64 || CDS_OS_TYPE == CDS_OS_MINGW
#    include <cds/os/win/timer.h>
#elif CDS_OS_TYPE == CDS_OS_LINUX
#    include <cds/os/linux/timer.h>
#elif CDS_OS_TYPE == CDS_OS_SUN_SOLARIS
#    include <cds/os/sunos/timer.h>
#elif CDS_OS_TYPE == CDS_OS_HPUX
#    include <cds/os/hpux/timer.h>
#elif CDS_OS_TYPE == CDS_OS_AIX
#    include <cds/os/aix/timer.h>
#elif CDS_OS_TYPE == CDS_OS_FREE_BSD
#   include <cds/os/free_bsd/timer.h>
#else
//************************************************************************
// Other OSes
//************************************************************************

namespace cds { namespace OS {
    // High resolution timer
    class Timer {
    public:
        typedef int64_t native_timer_type       ;
        typedef int64_t native_duration_type    ;

    private:

    public:
        static void current( native_timer_type& tmr )
        {
            tmr = 0;
        }

        static native_timer_type    current()
        {
            native_timer_type    tmr    ;
            current(tmr)    ;
            return tmr        ;
        }

        double reset()
        {
            return 0;
        }

        double duration( native_duration_type dur )
        {
            return double( dur ) / 1.0E9    ;
        }

        double duration()
        {
            return duration( native_duration() ) ;
        }

        native_duration_type    native_duration()
        {
            native_timer_type ts    ;
            current( ts )            ;
            return native_duration( 0, ts )    ;
        }

        static native_duration_type    native_duration( const native_timer_type& nStart, const native_timer_type& nEnd )
        {
            return nEnd - nStart;
        }

        static unsigned long long random_seed()
        {
            return 0;
        }
    };

}}    // namespace cds::OS
//@endcond


#endif

//@endcond
#endif    // #ifndef __CDS_OS_TIMER_H

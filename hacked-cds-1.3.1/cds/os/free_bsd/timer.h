/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_OS_FREE_BSD_TIMER_H
#define __CDS_OS_FREE_BSD_TIMER_H

#ifndef __CDS_OS_TIMER_H
#   error "<cds/os/timer.h> must be included"
#endif

#include <time.h>
#include <sys/timespec.h>

//@cond none
namespace cds { namespace OS {

    CDS_CXX11_INLINE_NAMESPACE namespace Free_BSD {

        // High resolution timer
        class Timer {
        public:
            typedef struct timespec native_timer_type       ;
            typedef long long       native_duration_type    ;

        private:
            native_timer_type    m_tmStart        ;

        public:

            Timer() { current( m_tmStart ) ; }

            static void current( native_timer_type& tmr )
            {
                // faster than gettimeofday() and posix
#           ifdef CLOCK_REALTIME_FAST
                ::clock_gettime( CLOCK_REALTIME_FAST, &tmr )    ;
#           else
                ::clock_gettime( CLOCK_REALTIME_FAST, &tmr )    ;
#           endif
            }

            static native_timer_type    current()
            {
                native_timer_type    tmr    ;
                current(tmr)    ;
                return tmr        ;
            }

            double reset()
            {
                native_timer_type ts    ;
                current( ts )        ;
                double dblRet = ( ts.tv_sec - m_tmStart.tv_sec ) + ( ts.tv_nsec - m_tmStart.tv_nsec ) / 1.0E9;
                m_tmStart = ts        ;
                return dblRet        ;
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
                return native_duration( m_tmStart, ts )    ;
            }

            static native_duration_type    native_duration( const native_timer_type& nStart, const native_timer_type& nEnd )
            {
                return native_duration_type( nEnd.tv_sec - nStart.tv_sec ) * 1000000000 + ( nEnd.tv_nsec - nStart.tv_nsec)    ;
            }

            static unsigned long long random_seed()
            {
                native_timer_type tmr ;
                current( tmr )    ;
                return ( ((unsigned long long)(tmr.tv_sec)) << 32 ) + tmr.tv_nsec ;
            }

        };
    }   // namespace Free_BSD

#ifndef CDS_CXX11_INLINE_NAMESPACE_SUPPORT
    typedef Free_BSD::Timer    Timer   ;
#endif

}}   // namespace cds::OS
//@endcond

#endif // #ifndef __CDS_OS_FREE_BSD_TIMER_H

/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_OS_WIN_TIMER_H
#define __CDS_OS_WIN_TIMER_H

#ifndef __CDS_OS_TIMER_H
#   error "<cds/os/timer.h> must be included"
#endif

#include <windows.h>

//@cond none
namespace cds { namespace OS {
    CDS_CXX11_INLINE_NAMESPACE namespace Win32 {

        /// High resolution timer
        /**
            Implementation of high resolution timer for Windows platforms.
            The implementation build on QueryPerformanceCounter API.
        */
        class Timer {
        public:
            typedef LARGE_INTEGER    native_timer_type        ;    ///< Native timer type
            typedef long long        native_duration_type    ;    ///< Native duration type

        private:
            native_timer_type    m_nFrequency    ;
            native_timer_type    m_nStart        ;

        public:
            Timer()
            {
                ::QueryPerformanceFrequency( &m_nFrequency );
                current( m_nStart )    ;
            }

            /// Places into \p tmr the current time in native Windows format
            static void current( native_timer_type& tmr )
            {
                ::QueryPerformanceCounter( &tmr )            ;
            }

            /// Returns current time in native Windows format
            static native_timer_type current()
            {
                native_timer_type    tmr    ;
                current(tmr)    ;
                return tmr        ;
            }

            /// Sets internal start time to current time. Returns duration from prevoius start time to current.
            double reset()
            {
                native_timer_type nCur                ;
                current( nCur )                        ;
                double dblRet = double(nCur.QuadPart - m_nStart.QuadPart) / m_nFrequency.QuadPart    ;
                m_nStart.QuadPart = nCur.QuadPart    ;
                return dblRet    ;
            }

            /// Translates \p dur from native format to seconds
            double duration( native_duration_type dur )
            {
                return double( dur ) / m_nFrequency.QuadPart    ;
            }

            /// Returns duration (in seconds) from start time to current
            double duration()
            {
                return duration( native_duration() )    ;
            }

            /// Returns duration (in native format) from start time to current
            native_duration_type    native_duration()
            {
                native_timer_type ts    ;
                current( ts )            ;
                return native_duration( m_nStart, ts )    ;
            }

            /// Calculates duration (in native format) between \p nEnd and \p nStart
            static native_duration_type    native_duration( const native_timer_type& nStart, const native_timer_type& nEnd )
            {
                return nEnd.QuadPart - nStart.QuadPart    ;
            }

            static unsigned long long random_seed()
            {
                return current().QuadPart ;
            }
        };


    }   // namespace Win32

#ifndef CDS_CXX11_INLINE_NAMESPACE_SUPPORT
    typedef Win32::Timer    Timer   ;
#endif

}}  // namespae cds::OS
//@endcond

#endif // #ifndef __CDS_OS_WIN_TIMER_H

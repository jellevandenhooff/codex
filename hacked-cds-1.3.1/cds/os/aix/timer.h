/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_OS_AIX_TIMER_H
#define __CDS_OS_AIX_TIMER_H

// Source: http://publib16.boulder.ibm.com/doc_link/en_US/a_doc_lib/libs/basetrf2/read_real_time.htm

#ifndef __CDS_OS_TIMER_H
#   error "<cds/os/timer.h> must be included"
#endif

#include <sys/time.h>
#include <sys/systemcfg.h>

//@cond none
namespace cds { namespace OS {
    CDS_CXX11_INLINE_NAMESPACE namespace Aix {

        // High resolution timer
        class Timer {
            timebasestruct_t    m_tmStart    ;

        protected:
            static unsigned long long nano( native_timer_type const& nStart, native_timer_type const& nEnd )
            {
                return (((unsigned long long) (nEnd.tb_high - nStart.tb_high)) << 32) + (nEnd.tb_low - nStart.tb_low) ;
            }

        public:
            typedef timebasestruct_t    native_timer_type        ;
            typedef long long            native_duration_type    ;

            Timer() : m_tmStart( current() ) {}

            static native_timer_type    current()
            {
                native_timer_type tm ;
                current( &tm )  ;
                return tm       ;
            }
            static void current( native_timer_type& tmr )
            {
                read_real_time( &tmr, sizeof(tmr) )     ;
                time_base_to_time( &tmr, sizeof(tmr ))  ;
            }

            double reset()
            {
                native_timer_type tt    ;
                current( tt )            ;
                double ret = nano( m_tmStart, tt ) / 1.0E9  ;
                m_tmStart = tt          ;
                return ret              ;
            }

            double duration( native_duration_type dur )
            {
                return double( dur ) / 1.0E9    ;
            }

            double duration()
            {
                return duration( native_duration() )    ;
            }

            native_duration_type    native_duration()
            {
                return native_duration( m_tmStart, current() ) ;
            }

            static native_duration_type    native_duration( native_timer_type const & nStart, native_timer_type const & nEnd )
            {
                return nano( nStart, nEnd ) ;
            }

            static unsigned long long random_seed()
            {
                native_timer_type tmr ;
                read_real_time( &tmr, sizeof(tmr) )     ;
                return ( ((unsigned long long)(tmr.tb_hight)) << 32 ) + tmr.tb_low ;
            }
        };
    }    // namespace Aix

#ifndef CDS_CXX11_INLINE_NAMESPACE_SUPPORT
    typedef Aix::Timer    Timer   ;
#endif

}}   // namespace cds::OS
//@endcond

#endif // #ifndef __CDS_OS_AIX_TIMER_H

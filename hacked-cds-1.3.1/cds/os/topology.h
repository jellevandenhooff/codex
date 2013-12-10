/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_OS_TOPOLOGY_H
#define __CDS_OS_TOPOLOGY_H

#include <cds/details/defs.h>

#if CDS_OS_TYPE == CDS_OS_WIN32 || CDS_OS_TYPE == CDS_OS_WIN64 || CDS_OS_TYPE == CDS_OS_MINGW
#   include <cds/os/win/topology.h>
#elif CDS_OS_TYPE == CDS_OS_LINUX
#   include <cds/os/linux/topology.h>
#elif CDS_OS_TYPE == CDS_OS_SUN_SOLARIS
#   include <cds/os/sunos/topology.h>
#elif CDS_OS_TYPE == CDS_OS_HPUX
#   include <cds/os/hpux/topology.h>
#elif CDS_OS_TYPE == CDS_OS_AIX
#   include <cds/os/aix/topology.h>
#elif CDS_OS_TYPE == CDS_OS_FREE_BSD || CDS_OS_TYPE == CDS_OS_OPEN_BSD || CDS_OS_TYPE == CDS_OS_NET_BSD
#   include <cds/os/free_bsd/topology.h>
#else
#   include <cds/os/details/fake_topology.h>
namespace cds { namespace OS {
  struct topology {
    static unsigned int processor_count() {
      return 1;
    }

    static unsigned int native_current_processor() {
      return 0;
    }

    static unsigned int current_processor() {
      return 0;
    }

    static void init() {}
    static void fini() {}
  };
}}
#endif

#endif  // #ifndef __CDS_OS_TOPOLOGY_H

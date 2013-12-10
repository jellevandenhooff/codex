/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_CONTAINER_STRIPED_SET_STD_HASH_SET_ADAPTER_H
#define __CDS_CONTAINER_STRIPED_SET_STD_HASH_SET_ADAPTER_H

#include <cds/container/striped_set/adapter.h>
#if CDS_COMPILER == CDS_COMPILER_MSVC && CDS_COMPILER_VERSION < 1600    // MS VC 2008
#   include <cds/container/striped_set/std_hash_set_vc.h>
#else
#   include <cds/container/striped_set/std_hash_set_std.h>
#endif

#endif // #ifndef __CDS_CONTAINER_STRIPED_SET_STD_HASH_SET_ADAPTER_H

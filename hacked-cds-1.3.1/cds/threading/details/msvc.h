/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_THREADING_DETAILS_MSVC_H
#define __CDS_THREADING_DETAILS_MSVC_H

#include <cds/threading/details/msvc_manager.h>

#ifndef CDS_CXX11_INLINE_NAMESPACE_SUPPORT
namespace cds { namespace threading {

    using msvc::Manager   ;

}} // namespace cds::threading
#endif

#endif // #ifndef __CDS_THREADING_DETAILS_MSVC_H

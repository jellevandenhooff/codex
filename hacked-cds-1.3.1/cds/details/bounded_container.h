/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_BOUNDED_CONTAINER_H
#define __CDS_BOUNDED_CONTAINER_H

namespace cds {
    /// Bounded container
    /**
        If a container has upper limit of item then it should be based on bounded_container class.
        Example of those containers: cyclic queue (cds::container::TsigasCycleQueue)
    */
    struct bounded_container {};
}    // namespace cds

#endif    // __CDS_BOUNDED_CONTAINER_H

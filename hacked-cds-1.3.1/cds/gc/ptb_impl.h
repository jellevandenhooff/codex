/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_GC_PTB_IMPL_H
#define __CDS_GC_PTB_IMPL_H

#include <cds/threading/model.h>

//@cond
namespace cds { namespace gc {

    inline PTB::thread_gc::thread_gc(
        bool    bPersistent
        )
        : m_bPersistent( bPersistent )
    {
        if ( !cds::threading::Manager::isThreadAttached() )
            cds::threading::Manager::attachThread() ;
    }

    inline PTB::thread_gc::~thread_gc()
    {
        if ( !m_bPersistent )
            cds::threading::Manager::detachThread() ;
    }

    inline PTB::Guard::Guard()
        : Guard::base_class( cds::threading::getGC<PTB>() )
    {}

    template <size_t COUNT>
    inline PTB::GuardArray<COUNT>::GuardArray()
        : GuardArray::base_class( cds::threading::getGC<PTB>() )
    {}

    inline void PTB::scan()
    {
        cds::threading::getGC<PTB>().scan()  ;
    }

}} // namespace cds::gc
//@endcond

#endif // #ifndef __CDS_GC_PTB_IMPL_H

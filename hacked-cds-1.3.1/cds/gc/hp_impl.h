/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_GC_HP_IMPL_H
#define __CDS_GC_HP_IMPL_H

#include <cds/threading/model.h>

//@cond
namespace cds { namespace gc {

    inline HP::thread_gc::thread_gc(
        bool    bPersistent
        )
        : m_bPersistent( bPersistent )
    {
        if ( !threading::Manager::isThreadAttached() )
            threading::Manager::attachThread() ;
    }

    inline HP::thread_gc::~thread_gc()
    {
        if ( !m_bPersistent )
            cds::threading::Manager::detachThread() ;
    }

    inline HP::Guard::Guard()
        : Guard::base_class( cds::threading::getGC<HP>() )
    {}

    template <size_t COUNT>
    inline HP::GuardArray<COUNT>::GuardArray()
        : GuardArray::base_class( cds::threading::getGC<HP>() )
    {}

    template <typename T>
    inline void HP::retire( T * p, void (* pFunc)(T *) )
    {
        cds::threading::getGC<HP>().retirePtr( p, pFunc )   ;
    }

    template <class Disposer, typename T>
    inline void HP::retire( T * p )
    {
        cds::threading::getGC<HP>().retirePtr( p, disposer<Disposer, T>::dispose )    ;
    }

    inline void HP::scan()
    {
        cds::threading::getGC<HP>().scan()  ;
    }


}} // namespace cds::gc
//@endcond

#endif // #ifndef __CDS_GC_HP_IMPL_H

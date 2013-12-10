/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_THREADING_MODEL_H
#define __CDS_THREADING_MODEL_H

#include <cds/threading/details/_common.h>
#include <cds/user_setup/threading.h>
#include <cds/threading/details/auto_detect.h>

namespace cds { namespace threading {

    /// Returns thread specific data of \p GC garbage collector
    template <class GC> typename GC::thread_gc_impl&  getGC()  ;

    /// Get cds::gc::HP thread GC implementation for current thread
    /**
        The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
        or if you did not use cds::gc::HP.
        To initialize cds::gc::HP GC you must constuct cds::gc::HP object in the beginning of your application,
        see \ref cds_how_to_use "How to use libcds"
    */
    template <>
    inline cds::gc::HP::thread_gc_impl&   getGC<cds::gc::HP>()
    {
        return Manager::getHZPGC()  ;
    }

    /// Get cds::gc::HRC thread GC implementation for current thread
    /**
        The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
        or if you did not use cds::gc::HRC.
        To initialize cds::gc::HRC GC you must constuct cds::gc::HRC object in the beginning of your application,
        see \ref cds_how_to_use "How to use libcds"
    */
    template <>
    inline cds::gc::HRC::thread_gc_impl&   getGC<cds::gc::HRC>()
    {
        return Manager::getHRCGC()  ;
    }

    /// Get cds::gc::PTB thread GC implementation for current thread
    /**
        The object returned may be uninitialized if you did not call attachThread in the beginning of thread execution
        or if you did not use cds::gc::PTB.
        To initialize cds::gc::PTB GC you must constuct cds::gc::PTB object in the beginning of your application,
        see \ref cds_how_to_use "How to use libcds"
    */
    template <>
    inline cds::gc::PTB::thread_gc_impl&   getGC<cds::gc::PTB>()
    {
        return Manager::getPTBGC()  ;
    }

}} // namespace cds::threading

#endif // #ifndef __CDS_THREADING_MODEL_H

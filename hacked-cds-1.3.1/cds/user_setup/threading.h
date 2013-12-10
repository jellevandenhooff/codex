/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_USER_SETUP_THREADING_MODEL_H
#define __CDS_USER_SETUP_THREADING_MODEL_H

/**
    CDS threading model

    CDS_THREADING_AUTODETECT - auto-detect appropriate threading model (default)

    CDS_THREADING_MSVC - use MS Visual C++ declspec( thread ) declaration to mantain thread-specific data

    CDS_THREADING_WIN_TLS - use Windows TLS API to mantain thread-specific data

    CDS_THREADING_GCC - use GCC __thread keyword to mantain thread-specific data

    CDS_THREADING_PTHREAD - use cds::Threading::Manager implementation based on pthread thread-specific
    data functions pthread_getspecific/pthread_setspecific

    CDS_THREADING_USER_DEFINED - use user-defined threading model
*/
#define CDS_THREADING_USER_DEFINED

#include "helper.h"

#include <cds/threading/details/_common.h>


extern ThreadLocalStorage<cds::threading::ThreadData*> cdsTLS;

namespace cds { namespace threading {
      class Manager {
      private :

      public:
          static void init()
          {
              cdsTLS.Reset();
          }

          static void fini()
          {
          }

          static bool isThreadAttached()
          {
              return cdsTLS.Get() != nullptr;
          }

          static void attachThread()
          {
              ThreadData * pData = new ThreadData();
              pData->init();
              cdsTLS.Get() = pData;
          }

          static void detachThread()
          {
              delete cdsTLS.Get();
              cdsTLS.Get() = nullptr;
          }

          static gc::HP::thread_gc_impl&   getHZPGC()
          {
              return *cdsTLS.Get()->m_hpManager;
          }

          static gc::HRC::thread_gc_impl&   getHRCGC()
          {
              return *cdsTLS.Get()->m_hrcManager;
          }

          static gc::PTB::thread_gc_impl&   getPTBGC()
          {
              return *cdsTLS.Get()->m_ptbManager;
          }

          static size_t fake_current_processor()
          {
              return cdsTLS.Get()->fake_current_processor()  ;
          }
    };
}}


#endif    // #ifndef __CDS_USER_SETUP_THREADING_MODEL_H

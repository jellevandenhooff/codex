/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_INTRUSIVE_MOIR_QUEUE_H
#define __CDS_INTRUSIVE_MOIR_QUEUE_H

#include <cds/intrusive/msqueue.h>

namespace cds { namespace intrusive {

    /// A variation of Michael & Scott's lock-free queue (intrusive variant)
    /** @ingroup cds_intrusive_queue
        This is slightly optimized Michael & Scott's queue algorithm that overloads \ref dequeue function.

        Source:
            \li [2000] Simon Doherty, Lindsay Groves, Victor Luchangco, Mark Moir
                "Formal Verification of a practical lock-free queue algorithm"

        Cite from this work about difference from Michael & Scott algo:
        "Our algorithm differs from Michael and Scott�s [MS98] in that we test whether \p Tail points to the header
        node only <b>after</b> \p Head has been updated, so a dequeuing process reads \p Tail only once. The dequeue in
        [MS98] performs this test before checking whether the next pointer in the dummy node is null, which
        means that it reads \p Tail every time a dequeuing process loops. Under high load, when operations retry
        frequently, our modification will reduce the number of accesses to global memory. This modification, however,
        introduces the possibility of \p Head and \p Tail �crossing�."

        Type of node: \ref single_link::node

        Explanation of template arguments see intrusive::MSQueue.

        \par Examples
        \code
        #include <cds/intrusive/moir_queue.h>
        #include <cds/gc/hp.h>

        namespace ci = cds::inrtusive ;
        typedef cds::gc::HP hp_gc;

        // MoirQueue with Hazard Pointer garbage collector, base hook + item disposer:
        struct Foo: public ci::single_link::node< hp_gc >
        {
            // Your data
            ...
        };

        // Disposer for Foo struct just deletes the object passed in
        struct fooDisposer {
            void operator()( Foo * p )
            {
                delete p    ;
            }
        };

        typedef ci::MoirQueue<
            hp_gc
            ,Foo
            ,ci::opt::hook<
                ci::single_link::base_hook< ci::opt::gc<hp_gc> >
            >
            ,ci::opt::disposer< fooDisposer >
        > fooQueue  ;

        // MoirQueue with Hazard Pointer garbage collector,
        // member hook + item disposer + item counter,
        // without alignment of internal queue data:
        struct Bar
        {
            // Your data
            ...
            ci::single_link::node< hp_gc > hMember ;
        };

        typedef ci::MoirQueue<
            hp_gc
            ,Foo
            ,ci::opt::hook<
                ci::single_link::member_hook<
                    offsetof(Bar, hMember)
                    ,ci::opt::gc<hp_gc>
                >
            >
            ,ci::opt::disposer< fooDisposer >
            ,cds::opt::item_counter< cds::atomicity::item_counter >
            ,cds::opt::alignment< cds::opt::no_special_alignment >
        > barQueue  ;

        \endcode
    */
    template <typename GC, typename T, CDS_DECL_OPTIONS9>
    class MoirQueue: public MSQueue< GC, T, CDS_OPTIONS9 >
    {
        //@cond
        typedef MSQueue< GC, T, CDS_OPTIONS9 > base_class ;
        typedef typename base_class::node_type node_type ;
        //@endcond

    public:
        //@cond
        typedef typename base_class::value_type value_type  ;
        typedef typename base_class::back_off   back_off    ;
        typedef typename base_class::gc         gc          ;
        typedef typename base_class::node_traits node_traits;
        typedef typename base_class::memory_model   memory_model    ;
        //@endcond

        /// Rebind template arguments
        template <typename GC2, typename T2, CDS_DECL_OTHER_OPTIONS9>
        struct rebind {
            typedef MoirQueue< GC2, T2, CDS_OTHER_OPTIONS9> other   ;   ///< Rebinding result
        };

    protected:
        //@cond
        typedef typename base_class::dequeue_result dequeue_result   ;
        typedef typename base_class::node_to_value node_to_value    ;

        bool do_dequeue( dequeue_result& res )
        {
            back_off bkoff  ;

            node_type * pNext    ;
            node_type * h        ;
            while ( true ) {
                h = res.guards.protect( 0, base_class::m_pHead, node_to_value() )  ;
                pNext = res.guards.protect( 1, h->m_pNext, node_to_value() )   ;

                if ( pNext == null_ptr<node_type *>() )
                    return false    ;    // queue is empty

                if ( base_class::m_pHead.compare_exchange_strong( h, pNext, memory_model::memory_order_release, CDS_ATOMIC::memory_order_relaxed )) {
                    node_type * t = base_class::m_pTail.load(memory_model::memory_order_acquire)    ;
                    if ( h == t )
                        base_class::m_pTail.compare_exchange_strong( t, pNext, memory_model::memory_order_release, CDS_ATOMIC::memory_order_relaxed ) ;
                    break ;
                }

                base_class::m_Stat.onDequeueRace()    ;
                bkoff()    ;
            }

            --base_class::m_ItemCounter     ;
            base_class::m_Stat.onDequeue()  ;

            res.pHead = h   ;
            res.pNext = pNext   ;
            return true     ;
        }
        //@endcond

    public:
        /// Dequeues a value from the queue
        /**
            See warning about item disposing in \ref MSQueue::dequeue.
        */
        value_type * dequeue()
        {
            dequeue_result res  ;
            if ( do_dequeue( res )) {
                base_class::dispose_result( res )   ;
                return node_traits::to_value_ptr( *res.pNext )  ;
            }
            return null_ptr<value_type *>()     ;
        }

        /// Synonym for \ref dequeue function
        value_type * pop()
        {
            return dequeue()    ;
        }
    };

}} // namespace cds::intrusive

#endif // #ifndef __CDS_INTRUSIVE_MOIR_QUEUE_H

/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_INTRUSIVE_SKIP_LIST_IMPL_H
#define __CDS_INTRUSIVE_SKIP_LIST_IMPL_H

#include <cds/intrusive/skip_list_base.h>
#include <cds/details/std/type_traits.h>
#include <cds/details/std/memory.h>
#include <cds/opt/compare.h>
#include <cds/ref.h>

namespace cds { namespace intrusive {

    //@cond
    namespace skip_list { namespace details {

        template <typename NodeType, typename AtomicNodePtr, typename Alloc>
        struct intrusive_node_builder
        {
            typedef NodeType        node_type       ;
            typedef AtomicNodePtr   atomic_node_ptr ;
            typedef Alloc           allocator_type  ;

            typedef cds::details::Allocator< atomic_node_ptr, allocator_type >  tower_allocator ;

            template <typename RandomGen>
            static node_type * make_tower( node_type * pNode, RandomGen& gen )
            {
                return make_tower( pNode, gen() + 1 )  ;
            }

            static node_type * make_tower( node_type * pNode, unsigned int nHeight )
            {
                if ( nHeight > 1 )
                    pNode->make_tower( nHeight, tower_allocator().NewArray( nHeight - 1, null_ptr<node_type *>() )) ;
                return pNode ;
            }

            static void dispose_tower( node_type * pNode )
            {
                unsigned int nHeight = pNode->height() ;
                if ( nHeight > 1 )
                    tower_allocator().Delete( pNode->release_tower(), nHeight ) ;
            }

            struct node_disposer {
                void operator()( node_type * pNode )
                {
                    dispose_tower( pNode )  ;
                }
            };
        };

        template <class GC, typename NodeTraits, typename BackOff, bool IsConst>
        class iterator {
        public:
            typedef GC                                  gc          ;
            typedef NodeTraits                          node_traits ;
            typedef BackOff                             back_off    ;
            typedef typename node_traits::node_type     node_type   ;
            typedef typename node_traits::value_type    value_type  ;
            static bool const c_isConst = IsConst ;

            typedef typename std::conditional< c_isConst, value_type const &, value_type &>::type   value_ref ;

        protected:
            typedef typename node_type::marked_ptr          marked_ptr          ;
            typedef typename node_type::atomic_marked_ptr   atomic_marked_ptr   ;

            typename gc::Guard      m_guard ;
            node_type *             m_pNode ;

        protected:
            static value_type * gc_protect( marked_ptr p )
            {
                return node_traits::to_value_ptr( p.ptr() ) ;
            }

            void next()
            {
                typename gc::Guard g    ;
                g.copy( m_guard )     ;
                back_off bkoff ;

                for (;;) {
                    if ( m_pNode->next( m_pNode->height() - 1 ).load( CDS_ATOMIC::memory_order_acquire ).bits() ) {
                        // Current node is marked as deleted. So, its next pointer can point to anything
                        // In this case we interrupt our iteration and returns end() iterator.
                        *this = iterator() ;
                        return ;
                    }

                    marked_ptr p = m_guard.protect( (*m_pNode)[0], gc_protect ) ;
                    node_type * pp = p.ptr() ;
                    if ( p.bits() ) {
                        // p is marked as deleted. Spin waiting for physical removal
                        bkoff()  ;
                        continue ;
                    }
                    else if ( pp && pp->next( pp->height() - 1 ).load( CDS_ATOMIC::memory_order_acquire ).bits() ) {
                        // p is marked as deleted. Spin waiting for physical removal
                        bkoff()  ;
                        continue ;
                    }

                    m_pNode = pp ;
                    break ;
                }
            }

        public: // for internal use only!!!
            iterator( node_type& refHead )
                : m_pNode( null_ptr<node_type *>() )
            {
                back_off bkoff ;

                for (;;) {
                    marked_ptr p = m_guard.protect( refHead[0], gc_protect ) ;
                    if ( !p.ptr() ) {
                        // empty skip-list
                        m_guard.clear() ;
                        break ;
                    }

                    node_type * pp = p.ptr() ;
                    // Logically deleted node is marked from highest level
                    if ( !pp->next( pp->height() - 1 ).load( CDS_ATOMIC::memory_order_acquire ).bits() ) {
                        m_pNode = pp    ;
                        break ;
                    }

                    bkoff() ;
                }
            }

        public:
            iterator()
                : m_pNode( null_ptr<node_type *>())
            {}

            iterator( iterator const& s)
                : m_pNode( s.m_pNode )
            {
                m_guard.assign( node_traits::to_value_ptr(m_pNode) ) ;
            }

            value_type * operator ->() const
            {
                assert( m_pNode != null_ptr< node_type *>() )   ;
                assert( node_traits::to_value_ptr( m_pNode ) != null_ptr<value_type *>() ) ;

                return node_traits::to_value_ptr( m_pNode ) ;
            }

            value_ref operator *() const
            {
                assert( m_pNode != null_ptr< node_type *>() )   ;
                assert( node_traits::to_value_ptr( m_pNode ) != null_ptr<value_type *>() ) ;

                return *node_traits::to_value_ptr( m_pNode ) ;
            }

            /// Pre-increment
            iterator& operator ++()
            {
                next()  ;
                return *this;
            }

            iterator& operator = (const iterator& src)
            {
                m_pNode = src.m_pNode   ;
                m_guard.copy( src.m_guard ) ;
                return *this    ;
            }

            template <typename Bkoff, bool C>
            bool operator ==(iterator<gc, node_traits, Bkoff, C> const& i ) const
            {
                return m_pNode == i.m_pNode ;
            }
            template <typename Bkoff, bool C>
            bool operator !=(iterator<gc, node_traits, Bkoff, C> const& i ) const
            {
                return !( *this == i )  ;
            }
        };
    }}  // namespace skip_list::details
    //@endcond

    /// Lock-free skip-list set
    /** @ingroup cds_intrusive_map

        The implementation of well-known probabilistic data structure called skip-list
        invented by W.Pugh in his papers:
            - [1989] W.Pugh Skip Lists: A Probabilistic Alternative to Balanced Trees
            - [1990] W.Pugh A Skip List Cookbook

        A skip-list is a probabilistic data structure that provides expected logarithmic
        time search without the need of rebalance. The skip-list is a collection of sorted
        linked list. Nodes are ordered by key. Each node is linked into a subset of the lists.
        Each list has a level, ranging from 0 to 32. The bottom-level list contains
        all the nodes, and each higher-level list is a sublist of the lower-level lists.
        Each node is created with a random top level (with a random height), and belongs
        to all lists up to that level. The probability that a node has the height 1 is 1/2.
        The probability that a node has the height N is 1/2 ** N (more precisely,
        the distribution depends on an random generator provided, but our generators
        have this property).

        The lock-free variant of skip-list is implemented according to book
            - [2008] M.Herlihy, N.Shavit "The Art of Multiprocessor Programming",
                chapter 14.4 "A Lock-Free Concurrent Skiplist".
        \note The algorithm described in this book cannot be directly adapted for C++ (roughly speaking,
        the algo contains a lot of bugs). The \b libcds implementation applies the approach discovered
        by M.Michael in his \ref cds_intrusive_MichaelList_hp "lock-free linked list".

        <b>Template arguments</b>:
        - \p GC - Garbage collector used. Note the \p GC must be the same as the GC used for item type \p T (see skip_list::node).
        - \p T - type to be stored in the list. The type must be based on skip_list::node (for skip_list::base_hook)
            or it must have a member of type skip_list::node (for skip_list::member_hook).
        - \p Traits - type traits. See skip_list::type_traits for explanation.

        It is possible to declare option-based list with cds::intrusive::skip_list::make_traits metafunction istead of \p Traits template
        argument.
        Template argument list \p Options of cds::intrusive::skip_list::make_traits metafunction are:
        - opt::hook - hook used. Possible values are: skip_list::base_hook, skip_list::member_hook, skip_list::traits_hook.
            If the option is not specified, <tt>skip_list::base_hook<></tt> and gc::HP is used.
        - opt::compare - key comparison functor. No default functor is provided.
            If the option is not specified, the opt::less is used.
        - opt::less - specifies binary predicate used for key comparision. Default is \p std::less<T>.
        - opt::disposer - the functor used for dispose removed items. Default is opt::v::empty_disposer. Due the nature
            of GC schema the disposer may be called asynchronously.
        - opt::item_counter - the type of item counting feature. Default is \ref atomicity::empty_item_counter that is no item counting.
        - opt::memory_model - C++ memory ordering model. Can be opt::v::relaxed_ordering (relaxed memory model, the default)
            or opt::v::sequential_consistent (sequentially consisnent memory model).
        - skip_list::random_level_generator - random level generator. Can be skip_list::xorshift, skip_list::turbo_pascal or
            user-provided one. See skip_list::random_level_generator option description for explanation.
            Default is \p %skip_list::turbo_pascal.
        - opt::allocator - although the skip-list is an intrusive container,
            an allocator should be provided to maintain variable randomly-calculated height of the node
            since the node can contain up to 32 next pointers. The allocator option is used to allocate an array of next pointers
            for nodes which height is more than 1. Default is \ref CDS_DEFAULT_ALLOCATOR.
        - opt::back_off - back-off strategy used. If the option is not specified, the cds::backoff::Default is used.
        - opt::stat - internal statistics. Available types: skip_list::stat, skip_list::empty_stat (the default)

        \warning The skip-list requires up to 67 hazard pointers that may be critical for some GCs for which
            the guard count is limited (like as gc::HP, gc::HRC). Those GCs should be explicitly initialized with
            hazard pointer enough: \code cds::gc::HP myhp( 67 ) \endcode. Otherwise an run-time exception may be raised
            when you try to create skip-list object.

        \note There are several specializations of \p %SkipListSet for each \p GC. You should include:
        - <tt><cds/intrusive/skip_list_hp.h></tt> for gc::HP garbage collector
        - <tt><cds/intrusive/skip_list_hrc.h></tt> for gc::HRC garbage collector
        - <tt><cds/intrusive/skip_list_ptb.h></tt> for gc::PTB garbage collector

        <b>Iterators</b>

        The class supports a forward iterator (\ref iterator and \ref const_iterator).
        The iteration is ordered.
        The iterator object is thread-safe: the element pointed by the iterator object is guarded,
        so, the element cannot be reclaimed while the iterator object is alive.
        However, passing an iterator object between threads is dangerous.

        \warning Due to concurrent nature of skip-list set it is not guarantee that you can iterate
        all elements in the set: any concurrent deletion can exclude the element
        pointed by the iterator from the set, and your iteration can be terminated
        before end of the set. Therefore, such iteration is more suitable for debugging purpose only

        Remember, each iterator object requires 2 additional hazard pointers, that may be
        a limited resource for \p GC like as gc::HP and gc::HRC (for gc::PTB the count of
        guards is unlimited).

        The iterator class supports the following minimalistic interface:
        \code
        struct iterator {
            // Default ctor
            iterator();

            // Copy ctor
            iterator( iterator const& s) ;

            value_type * operator ->() const ;
            value_type& operator *() const ;

            // Pre-increment
            iterator& operator ++() ;

            // Copy assignment
            iterator& operator = (const iterator& src) ;

            bool operator ==(iterator const& i ) const ;
            bool operator !=(iterator const& i ) const ;
        };
        \endcode
        Note, the iterator object returned by \ref end, \ref cend member functions points to \p NULL and should not be dereferenced.

        <b>How to use</b>

        You should incorporate skip_list::node into your struct \p T and provide
        appropriate skip_list::type_traits::hook in your \p Traits template parameters. Usually, for \p Traits you
        define a struct based on skip_list::type_traits.

        Example for gc::HP and base hook:
        \code
        // Include GC-related skip-list specialization
        #include <cds/intrusive/skip_list_hp.h>

        // Data stored in skip list
        struct my_data: public cds::intrusive::skip_list::node< cds::gc::HP >
        {
            // key field
            std::string     strKey  ;

            // other data
            // ...
        }   ;

        // my_data compare functor
        struct my_data_cmp {
            int operator()( const my_data& d1, const my_data& d2 )
            {
                return d1.strKey.compare( d2.strKey )   ;
            }

            int operator()( const my_data& d, const std::string& s )
            {
                return d.strKey.compare(s)   ;
            }

            int operator()( const std::string& s, const my_data& d )
            {
                return s.compare( d.strKey )    ;
            }
        } ;


        // Declare type_traits
        struct my_traits: public cds::intrusive::skip_list::type_traits
        {
            typedef cds::intrusive::skip_list::base_hook< cds::opt::gc< cds::gc::HP > >   hook    ;
            typedef my_data_cmp compare ;
        };

        // Declare skip-list set type
        typedef cds::intrusive::SkipListSet< cds::gc::HP, my_data, my_traits >     traits_based_set   ;
        \endcode

        Equivalent option-based code:
        \code
        // GC-related specialization
        #include <cds/intrusive/skip_list_hp.h>

        struct my_data {
            // see above
        }   ;
        struct compare {
            // see above
        }   ;

        // Declare option-based skip-list set
        typedef cds::intrusive::SkipListSet< cds::gc::HP
            ,my_data
            , typename cds::intrusive::skip_list::make_traits<
                cds::intrusive::opt::hook< cds::intrusive::skip_list::base_hook< cds::opt::gc< cds::gc::HP > > >
                ,cds::intrusive::opt::compare< my_data_cmp >
            >::type
        > option_based_set   ;

        \endcode

    */
    template <
        class GC
       ,typename T
#ifdef CDS_DOXYGEN_INVOKED
       ,typename Traits = skip_list::type_traits
#else
       ,typename Traits
#endif
    >
    class SkipListSet
    {
    public:
        typedef T       value_type      ;   ///< type of value stored in the skip-list
        typedef Traits  options         ;   ///< Traits template parameter

        typedef typename options::hook      hook        ;   ///< hook type
        typedef typename hook::node_type    node_type   ;   ///< node type

#   ifdef CDS_DOXYGEN_INVOKED
        typedef implementation_defined key_comparator  ;    ///< key comparision functor based on opt::compare and opt::less option setter.
#   else
        typedef typename opt::details::make_comparator< value_type, options >::type key_comparator  ;
#   endif

        typedef typename options::disposer  disposer    ;   ///< disposer used
        typedef typename get_node_traits< value_type, node_type, hook>::type node_traits ;    ///< node traits

        typedef GC  gc          ;   ///< Garbage collector
        typedef typename options::item_counter  item_counter ;   ///< Item counting policy used
        typedef typename options::memory_model  memory_model;   ///< Memory ordering. See cds::opt::memory_model option
        typedef typename options::random_level_generator    random_level_generator  ;   ///< random level generator
        typedef typename options::allocator     allocator_type  ;   ///< allocator for maintaining array of next pointers of the node
        typedef typename options::back_off      back_off    ;   ///< Back-off trategy
        typedef typename options::stat          stat        ;   ///< internal statistics type

        /// Max node height. The actual node height should be in range <tt>[0 .. c_nMaxHeight)</tt>
        /**
            The max height is specified by \ref skip_list::random_level_generator "random level generator" constant \p m_nUpperBound
            but it should be no more than 32 (\ref skip_list::c_nHeightLimit).
        */
        static unsigned int const c_nMaxHeight = std::conditional<
            (random_level_generator::c_nUpperBound <= skip_list::c_nHeightLimit),
            std::integral_constant< unsigned int, random_level_generator::c_nUpperBound >,
            std::integral_constant< unsigned int, skip_list::c_nHeightLimit >
        >::type::value ;

        //@cond
        static unsigned int const c_nMinHeight = 5 ;
        //@endcond

    protected:
        typedef typename node_type::atomic_marked_ptr   atomic_node_ptr ;   ///< Atomic marked node pointer
        typedef typename node_type::marked_ptr          marked_node_ptr ;   ///< Node marked pointer

    protected:
        //@cond
        typedef skip_list::details::intrusive_node_builder< node_type, atomic_node_ptr, allocator_type > intrusive_node_builder ;

        typedef typename std::conditional<
            std::is_same< typename options::internal_node_builder, cds::opt::none >::value
            ,intrusive_node_builder
            ,typename options::internal_node_builder
        >::type node_builder    ;

        typedef std::unique_ptr< node_type, typename node_builder::node_disposer >    scoped_node_ptr ;

        // c_nMaxHeight * 2 - pPred/pSucc guards
        // + 1 - find_position internal (for pCur)
        // + 1 - for erase, unlink
        // + 1 - for clear
        static size_t const c_nHazardPtrCount = c_nMaxHeight * 2 + 3 ;
        struct position {
            node_type *   pPrev[ c_nMaxHeight ]   ;
            node_type *   pSucc[ c_nMaxHeight ]   ;

            typename gc::template GuardArray< c_nMaxHeight * 2 > guards  ;   ///< Guards array for pPrev/pSucc

            node_type *   pCur  ;   // guarded by guards; needed only for *ensure* function
        };

#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        struct empty_insert_functor {
            void operator()( value_type& )
            {}
        };

        struct empty_erase_functor  {
            void operator()( value_type const& )
            {}
        };

        struct empty_find_functor {
            template <typename Q>
            void operator()( value_type& item, Q& val )
            {}
        };

        template <typename Func>
        struct insert_at_ensure_functor {
            Func m_func ;
            insert_at_ensure_functor( Func f ) : m_func(f) {}

            void operator()( value_type& item )
            {
                cds::unref( m_func)( true, item, item ) ;
            }
        };

        template <typename Func>
        struct less_wrapper {
            Func m_less ;

            template <typename Q1, typename Q2>
            int operator()( Q1 const& v1, Q2 const& v2 ) const
            {
                return m_less( v1, v2 ) ? -1 : ( m_less( v2, v1 ) ? 1 : 0 ) ;
            }
        };

#   endif // ifndef CDS_CXX11_LAMBDA_SUPPORT

        //@endcond

    protected:
        skip_list::details::head_node< node_type >      m_Head  ;   ///< head tower (max height)

        item_counter                m_ItemCounter       ;   ///< item counter
        random_level_generator      m_RandomLevelGen    ;   ///< random level generator instance
        CDS_ATOMIC::atomic<unsigned int>    m_nHeight   ;   ///< estimated high level
        mutable stat                m_Stat              ;   ///< internal statistics

    protected:
        //@cond
        unsigned int random_level()
        {
            // Random generator produces a number from range [0..31]
            // We need a number from range [1..32]
            return m_RandomLevelGen() + 1 ;
        }

        template <typename Q>
        node_type * build_node( Q v )
        {
            return node_builder::make_tower( v, m_RandomLevelGen ) ;
        }

        static value_type * gc_protect( marked_node_ptr p )
        {
            return node_traits::to_value_ptr( p.ptr() ) ;
        }

        static void dispose_node( value_type * pVal )
        {
            assert( pVal != NULL )  ;
            typename node_builder::node_disposer()( node_traits::to_node_ptr(pVal) ) ;
            disposer()( pVal )      ;
        }

        template <typename Q, typename Compare >
        bool find_position( Q const& val, position& pos, Compare cmp, bool bStopIfFound )
        {
            node_type * pPred ;
            marked_node_ptr pSucc ;
            marked_node_ptr pCur ;

            //key_comparator cmp ;
            int nCmp = 1    ;
            typename gc::Guard gCur ;

            // Hazard pointer array:
            //  pPred: [nLevel * 2]
            //  pSucc: [nLevel * 2 + 1]

        retry:
            pPred = m_Head.head() ;

            for ( int nLevel = (int) c_nMaxHeight - 1; nLevel >= 0; --nLevel ) {
                pos.guards.assign( nLevel * 2, node_traits::to_value_ptr( pPred )) ;
                while ( true ) {
                    pCur = pPred->next( nLevel ).load( memory_model::memory_order_relaxed ) ;
                    gCur.assign( node_traits::to_value_ptr( pCur.ptr() )) ;
                    if ( pPred->next( nLevel ).load( memory_model::memory_order_acquire ) != pCur
                        || pCur.bits() )
                    {
                        // pCur.bits() means that pPred is logically deleted
                        goto retry ;
                    }

                    if ( pCur.ptr() == null_ptr<node_type *>()) {
                        // end of the list at level nLevel - goto next level
                        break   ;
                    }

                    // pSucc contains deletion mark for pCur
                    pSucc = pCur->next( nLevel ).load( memory_model::memory_order_relaxed ) ;
                    pos.guards.assign( nLevel * 2 + 1, node_traits::to_value_ptr( pSucc.ptr() )) ;
                    if ( pCur->next( nLevel ).load( memory_model::memory_order_acquire ) != pSucc )
                        goto retry ;

                    if ( pPred->next( nLevel ).load( memory_model::memory_order_relaxed ).all() != pCur.ptr() )
                        goto retry ;

                    if ( pSucc.bits() ) {
                        // pCur is marked, i.e. logically deleted.
                        marked_node_ptr p( pCur.ptr() ) ;
                        if ( !pPred->next( nLevel ).compare_exchange_strong( p, marked_node_ptr( pSucc.ptr() ), memory_model::memory_order_release, memory_model::memory_order_relaxed ))
                            goto retry ;
                        if ( nLevel == 0 ) {
                            decrease_height( pCur->height() )  ;
                            gc::retire( node_traits::to_value_ptr( pCur.ptr() ), dispose_node ) ;
                        }
                    }
                    else {
                        nCmp = cmp( *node_traits::to_value_ptr( pCur.ptr()), val ) ;
                        if ( nCmp < 0 ) {
                            pPred = pCur.ptr()  ;
                            //pCur = pSucc        ;
                            pos.guards.copy( nLevel * 2, gCur ) ;   // pPrev guard := gCur
                            //gCur.assign( pos.guards.get_native(nLevel * 2 + 1) ) ;
                        }
                        else if ( nCmp == 0 && bStopIfFound )
                            goto found ;
                        else
                            break ;
                    }
                }

            //next_level:
                pos.pPrev[ nLevel ] = pPred ;
                pos.pSucc[ nLevel ] = pCur.ptr() ;
                pos.guards.copy( nLevel * 2 + 1, gCur ) ;
            }

            if ( nCmp != 0 )
                return false ;

        found:
            pos.pCur = pCur.ptr() ;
            pos.guards.copy( 1, gCur ) ;
            return pCur.ptr() && nCmp == 0 ;
        }

        template <typename Func>
        bool insert_at_position( value_type& val, node_type * pNode, position& pos, Func f )
        {
            unsigned int nHeight = pNode->height() ;

            for ( unsigned int nLevel = 1; nLevel < nHeight; ++nLevel )
                pNode->next(nLevel).store( marked_node_ptr(), memory_model::memory_order_relaxed ) ;

            {
                marked_node_ptr p( pos.pSucc[0] ) ;
                pNode->next( 0 ).store( marked_node_ptr( pos.pSucc[ 0 ] ), memory_model::memory_order_release ) ;
                if ( !pos.pPrev[0]->next(0).compare_exchange_strong( p, marked_node_ptr(pNode), memory_model::memory_order_release, memory_model::memory_order_relaxed ) ) {
                    return false ;
                }
                cds::unref( f )( val ) ;
            }

            for ( unsigned int nLevel = 1; nLevel < nHeight; ++nLevel ) {
                marked_node_ptr p ;
                while ( true ) {
                    marked_node_ptr q( pos.pSucc[ nLevel ]) ;
                    if ( !pNode->next( nLevel ).compare_exchange_strong( p, q, memory_model::memory_order_release, memory_model::memory_order_relaxed )) {
                        // pNode has been marked as removed while we are inserting it
                        // Stop inserting
                        assert( p.bits() )  ;
                        return true         ;
                    }
                    p = q   ;
                    if ( pos.pPrev[nLevel]->next(nLevel).compare_exchange_strong( q, marked_node_ptr( pNode ), memory_model::memory_order_release, memory_model::memory_order_relaxed ) )
                        break ;

                    // Renew insert position
                    find_position( val, pos, key_comparator(), false ) ;
                }
            }
            return true ;
        }

        template <typename Q, typename Func>
        bool try_remove_at( node_type * pDel, Q const& val, position& pos, Func f )
        {
            assert( pDel != null_ptr<node_type *>()) ;

            marked_node_ptr pSucc   ;
            typename gc::Guard gSucc ;

            // logical deletion (marking)
            for ( unsigned int nLevel = pDel->height() - 1; nLevel > 0; --nLevel ) {
                while ( true ) {
                    pSucc = gSucc.protect( pDel->next(nLevel), gc_protect ) ;
                    if ( pSucc.bits() || pDel->next(nLevel).compare_exchange_weak( pSucc, marked_node_ptr( pSucc.ptr(), 1 ),
                         memory_model::memory_order_acquire, memory_model::memory_order_relaxed ))
                    {
                        break ;
                    }
                }
            }

            while ( true ) {
                pSucc = gSucc.protect( pDel->next(0), gc_protect ) ;
                marked_node_ptr p( pSucc.ptr() ) ;
                if ( pDel->next(0).compare_exchange_strong( p, marked_node_ptr(p.ptr(), 1),
                     memory_model::memory_order_acquire, memory_model::memory_order_relaxed ))
                {
                    cds::unref(f)( *node_traits::to_value_ptr( pDel )) ;

                    // physical deletion
                    find_position( val, pos, key_comparator(), false ) ;
                    return true ;
                }
                else {
                    if ( p.bits() ) {
                        return false ;
                    }
                }
            }
        }

        enum finsd_fastpath_result {
            find_fastpath_found,
            find_fastpath_not_found,
            find_fastpath_abort
        };
        template <typename Q, typename Compare, typename Func>
        finsd_fastpath_result find_fastpath( Q& val, Compare cmp, Func f )
        {
            node_type * pPred       ;
            typename gc::template GuardArray<2>  guards ;
            marked_node_ptr pCur    ;
            marked_node_ptr pSucc   ;
            marked_node_ptr pNull   ;

            back_off bkoff ;

            pPred = m_Head.head() ;
            for ( int nLevel = (int) m_nHeight.load(memory_model::memory_order_relaxed) - 1; nLevel >= 0; --nLevel ) {
                pCur = guards.protect( 1, pPred->next(nLevel), gc_protect ) ;
                if ( pCur == pNull )
                    continue ;

                while ( pCur != pNull ) {
                    if ( pCur.bits() ) {
                        unsigned int nAttempt = 0 ;
                        while ( pCur.bits() && nAttempt++ < 16 ) {
                            bkoff() ;
                            pCur = guards.protect( 1, pPred->next(nLevel), gc_protect ) ;
                        }
                        bkoff.reset() ;

                        if ( pCur.bits() ) {
                            // Maybe, we are on deleted node sequence
                            // Abort searching, try slow-path
                            return find_fastpath_abort ;
                        }
                    }

                    if ( pCur.ptr() ) {
                        int nCmp = cmp( *node_traits::to_value_ptr( pCur.ptr() ), val ) ;
                        if ( nCmp < 0 ) {
                            guards.copy( 0, 1 ) ;
                            pPred = pCur.ptr() ;
                            pCur = guards.protect( 1, pCur->next(nLevel), gc_protect ) ;
                        }
                        else if ( nCmp == 0 ) {
                            // found
                            cds::unref(f)( *node_traits::to_value_ptr( pCur.ptr() ), val ) ;
                            return find_fastpath_found ;
                        }
                        else // pCur > val - go down
                            break;
                    }
                }
            }

            return find_fastpath_not_found ;
        }

        template <typename Q, typename Compare, typename Func>
        bool find_slowpath( Q& val, Compare cmp, Func f )
        {
            position pos ;
            if ( find_position( val, pos, cmp, true )) {
                assert( cmp( *node_traits::to_value_ptr( pos.pCur ), val ) == 0 ) ;

                cds::unref(f)( *node_traits::to_value_ptr( pos.pCur ), val ) ;
                return true ;
            }
            else
                return false ;
        }

        template <typename Q, typename Compare, typename Func>
        bool find_with_( Q& val, Compare cmp, Func f )
        {
            switch ( find_fastpath( val, cmp, f )) {
            case find_fastpath_found:
                m_Stat.onFindFastSuccess() ;
                return true ;
            case find_fastpath_not_found:
                m_Stat.onFindFastFailed() ;
                return false ;
            default:
                break;
            }

            if ( find_slowpath( val, cmp, f )) {
                m_Stat.onFindSlowSuccess() ;
                return true ;
            }

            m_Stat.onFindSlowFailed() ;
            return false ;
        }

        void increase_height( unsigned int nHeight )
        {
            if ( m_nHeight.load( memory_model::memory_order_relaxed ) < nHeight )
                m_nHeight.store( nHeight, memory_model::memory_order_relaxed ) ;
        }

        void decrease_height( unsigned int nHeight )
        {
            if ( nHeight > c_nMinHeight ) {
                unsigned int nCurHeight = m_nHeight.load( memory_model::memory_order_relaxed ) ;
                if ( nCurHeight == nHeight ) {
                    while ( nHeight > c_nMinHeight && m_Head.head()->next(nHeight).load(memory_model::memory_order_relaxed).ptr() == null_ptr<node_type *>())
                    {
                        --nHeight   ;
                    }
                    if ( nHeight < nCurHeight )
                        m_nHeight.compare_exchange_weak( nCurHeight, nHeight, memory_model::memory_order_relaxed, memory_model::memory_order_relaxed ) ;
                }
            }
        }
        //@endcond

    public:
        /// Default constructor
        /**
            The constructor checks whether the count of guards is enough
            for skip-list and may raise an exception if not.
        */
        SkipListSet()
            : m_Head( c_nMaxHeight )
            , m_nHeight( c_nMinHeight )
        {
            static_assert( (std::is_same< gc, typename node_type::gc >::value), "GC and node_type::gc must be the same type" ) ;

            gc::check_available_guards( c_nHazardPtrCount ) ;

            // Barrier for head node
            CDS_ATOMIC::atomic_thread_fence( memory_model::memory_order_release ) ;
        }

        /// Clears and destructs the skip-list
        ~SkipListSet()
        {
            clear() ;
        }

    public:
        /// Iterator type
        typedef skip_list::details::iterator< gc, node_traits, back_off, false >  iterator        ;

        /// Const iterator type
        typedef skip_list::details::iterator< gc, node_traits, back_off, true >   const_iterator  ;

        /// Returns a forward iterator addressing the first element in a set
        iterator begin()
        {
            return iterator( *m_Head.head() )    ;
        }

        /// Returns a forward const iterator addressing the first element in a set
        const_iterator cbegin()
        {
            return const_iterator( *m_Head.head() ) ;
        }

        /// Returns a forward const iterator addressing the first element in a set (same as \ref cbegin)
        const_iterator begin() const
        {
            return cbegin() ;
        }

        /// Returns a forward iterator that addresses the location succeeding the last element in a set.
        iterator end()
        {
            return iterator()   ;
        }

        /// Returns a forward const iterator that addresses the location succeeding the last element in a set.
        const_iterator cend()
        {
            return const_iterator() ;
        }

        /// Returns a forward const iterator that addresses the location succeeding the last element in a set (same as \ref cend)
        const_iterator end() const
        {
            return cend() ;
        }

    public:
        /// Inserts new node
        /**
            The function inserts \p val in the set if it does not contain
            an item with key equal to \p val.

            Returns \p true if \p val is placed into the set, \p false otherwise.
        */
        bool insert( value_type& val )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return insert( val, []( value_type& ) {} )    ;
#       else
            return insert( val, empty_insert_functor() )    ;
#       endif
        }

        /// Inserts new node
        /**
            This function is intended for derived non-intrusive containers.

            The function allows to split creating of new item into two part:
            - create item with key only
            - insert new item into the set
            - if inserting is success, calls  \p f functor to initialize value-field of \p val.

            The functor signature is:
            \code
                void func( value_type& val ) ;
            \endcode
            where \p val is the item inserted. User-defined functor \p f should guarantee that during changing
            \p val no any other changes could be made on this set's item by concurrent threads.
            The user-defined functor is called only if the inserting is success and may be passed by reference
            using <tt>boost::ref</tt>
        */
        template <typename Func>
        bool insert( value_type& val, Func f )
        {
            typename gc::Guard gNew    ;
            gNew.assign( &val )        ;

            node_type * pNode = node_traits::to_node_ptr( val ) ;
            scoped_node_ptr scp( pNode ) ;
            unsigned int nHeight = pNode->height() ;
            bool bTowerOk = nHeight > 1 && pNode->get_tower() != null_ptr<atomic_node_ptr *>() ;
            bool bTowerMade = false ;

            position pos ;
            while ( true )
            {
                bool bFound = find_position( val, pos, key_comparator(), true ) ;
                if ( bFound ) {
                    // scoped_node_ptr deletes the node tower if we create it
                    if ( !bTowerMade )
                        scp.release() ;

                    m_Stat.onInsertFailed() ;
                    return false ;
                }

                if ( !bTowerOk ) {
                    build_node( pNode ) ;
                    nHeight = pNode->height() ;
                    bTowerMade =
                        bTowerOk = true     ;
                }

                if ( !insert_at_position( val, pNode, pos, f )) {
                    m_Stat.onInsertRetry() ;
                    continue ;
                }

                increase_height( nHeight )  ;
                ++m_ItemCounter ;
                m_Stat.onAddNode( nHeight ) ;
                m_Stat.onInsertSuccess() ;
                scp.release()   ;
                return true ;
            }
        }

        /// Ensures that the \p val exists in the set
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the item \p val is not found in the set, then \p val is inserted into the set.
            Otherwise, the functor \p func is called with item found.
            The functor signature is:
            \code
                void func( bool bNew, value_type& item, value_type& val ) ;
            \endcode
            with arguments:
            - \p bNew - \p true if the item has been inserted, \p false otherwise
            - \p item - item of the set
            - \p val - argument \p val passed into the \p ensure function
            If new item has been inserted (i.e. \p bNew is \p true) then \p item and \p val arguments
            refer to the same thing.

            The functor can change non-key fields of the \p item; however, \p func must guarantee
            that during changing no any other modifications could be made on this item by concurrent threads.

            You can pass \p func argument by value or by reference using <tt>boost::ref</tt> or cds::ref.

            Returns std::pair<bool, bool> where \p first is \p true if operation is successfull,
            \p second is \p true if new item has been added or \p false if the item with \p key
            already is in the set.
        */
        template <typename Func>
        std::pair<bool, bool> ensure( value_type& val, Func func )
        {
            typename gc::Guard gNew    ;
            gNew.assign( &val )        ;

            node_type * pNode = node_traits::to_node_ptr( val ) ;
            scoped_node_ptr scp( pNode ) ;
            unsigned int nHeight = pNode->height() ;
            bool bTowerOk = nHeight > 1 && pNode->get_tower() != null_ptr<atomic_node_ptr *>() ;
            bool bTowerMade = false ;

#       ifndef CDS_CXX11_LAMBDA_SUPPORT
            insert_at_ensure_functor<Func> wrapper( func ) ;
#       endif

            position pos ;
            while ( true )
            {
                bool bFound = find_position( val, pos, key_comparator(), true ) ;
                if ( bFound ) {
                    // scoped_node_ptr deletes the node tower if we create it before
                    if ( !bTowerMade )
                        scp.release() ;

                    cds::unref(func)( false, *node_traits::to_value_ptr(pos.pCur), val ) ;
                    m_Stat.onEnsureExist() ;
                    return std::make_pair( true, false ) ;
                }

                if ( !bTowerOk ) {
                    build_node( pNode ) ;
                    nHeight = pNode->height() ;
                    bTowerMade =
                        bTowerOk = true ;
                }

#       ifdef CDS_CXX11_LAMBDA_SUPPORT
                if ( !insert_at_position( val, pNode, pos, [&func]( value_type& item ) { cds::unref(func)( true, item, item ); }))
#       else
                if ( !insert_at_position( val, pNode, pos, cds::ref(wrapper) ))
#       endif
                {
                    m_Stat.onInsertRetry() ;
                    continue ;
                }

                increase_height( nHeight )  ;
                ++m_ItemCounter ;
                //cds::unref(func)( true, val, val ) ;
                scp.release()   ;
                m_Stat.onAddNode( nHeight ) ;
                m_Stat.onEnsureNew() ;
                return std::make_pair( true, true ) ;
            }
        }

        /// Unlink the item \p val from the set
        /**
            The function searches the item \p val in the set and unlink it from the set
            if it is found and is equal to \p val.

            Difference between \ref erase and \p unlink functions: \p erase finds <i>a key</i>
            and deletes the item found. \p unlink finds an item by key and deletes it
            only if \p val is an item of that set, i.e. the pointer to item found
            is equal to <tt> &val </tt>.

            The \ref disposer specified in \p Traits class template parameter is called
            by garbage collector \p GC asynchronously.

            The function returns \p true if success and \p false otherwise.
        */
        bool unlink( value_type& val )
        {
            position pos ;

            if ( !find_position( val, pos, key_comparator(), false ) ) {
                m_Stat.onUnlinkFailed() ;
                return false ;
            }

            node_type * pDel = pos.pCur ;
            assert( key_comparator()( *node_traits::to_value_ptr( pDel ), val ) == 0 ) ;

            unsigned int nHeight = pDel->height() ;
            typename gc::Guard gDel ;
            gDel.assign( node_traits::to_value_ptr(pDel) ) ;

            if ( node_traits::to_value_ptr( pDel ) == &val && try_remove_at( pDel, val, pos,
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
                [](value_type const&) {}
#       else
                empty_erase_functor()
#       endif
            ))
            {
                //decrease_height( nHeight )  ;
                --m_ItemCounter ;
                m_Stat.onRemoveNode( nHeight ) ;
                m_Stat.onUnlinkSuccess() ;
                //gc::retire( node_traits::to_value_ptr( pDel ), dispose_node ) ;
                return true ;
            }

            m_Stat.onUnlinkFailed() ;
            return false ;
        }

        /// Delete the item from the set
        /**
            The function searches an item with key equal to \p val in the set,
            unlinks it from the set, and returns \p true.
            If the item with key equal to \p val is not found the function return \p false.

            Note the hash functor should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q>
        bool erase( const Q& val )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return erase( val, [](value_type const&) {} )  ;
#       else
            return erase( val, empty_erase_functor() )  ;
#       endif
        }

        /// Delete the item from the set
        /**
            The function searches an item with key equal to \p val in the set,
            call \p f functor with item found, unlinks it from the set, and returns \p true.
            The \ref disposer specified in \p Traits class template parameter is called
            by garbage collector \p GC asynchronously.

            The \p Func interface is
            \code
            struct functor {
                void operator()( value_type const& item ) ;
            } ;
            \endcode
            The functor can be passed by reference with <tt>boost:ref</tt>

            If the item with key equal to \p val is not found the function return \p false.

            Note the hash functor should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q, typename Func>
        bool erase( Q const& val, Func f )
        {
            position pos ;

            if ( !find_position( val, pos, key_comparator(), false ) ) {
                m_Stat.onEraseFailed() ;
                return false ;
            }

            node_type * pDel = pos.pCur ;
            typename gc::Guard gDel ;
            gDel.assign( node_traits::to_value_ptr(pDel) ) ;
            assert( key_comparator()( *node_traits::to_value_ptr( pDel ), val ) == 0 ) ;

            unsigned int nHeight = pDel->height() ;
            if ( try_remove_at( pDel, val, pos, f )) {
                //decrease_height( nHeight )      ;
                --m_ItemCounter ;
                m_Stat.onRemoveNode( nHeight )  ;
                m_Stat.onEraseSuccess() ;
                //gc::retire( node_traits::to_value_ptr( pDel ), dispose_node ) ;
                return true ;
            }

            m_Stat.onEraseFailed() ;
            return false ;
        }

        /// Find the key \p val
        /**
            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You can pass \p f argument by value or by reference using <tt>boost::ref</tt> or cds::ref.

            The functor can change non-key fields of \p item. Note that the functor is only guarantee
            that \p item cannot be disposed during functor is executing.
            The functor does not serialize simultaneous access to the set \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
            can modify both arguments.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q& val, Func f )
        {
            return find_with_( val, key_comparator(), f ) ;
        }

        /// Find the key \p val
        /**
            The function searches the item with key equal to \p val and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q const& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You can pass \p f argument by value or by reference using <tt>boost::ref</tt> or cds::ref.

            The functor can change non-key fields of \p item. Note that the functor is only guarantee
            that \p item cannot be disposed during functor is executing.
            The functor does not serialize simultaneous access to the set \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& val, Func f )
        {
            return find_with_( val, key_comparator(), f ) ;
        }

        /// Find the key \p val
        /**
            The function searches the item with key equal to \p val
            and returns \p true if it is found, and \p false otherwise.

            Note the hash functor specified for class \p Traits template parameter
            should accept a parameter of type \p Q that can be not the same as \p value_type.
        */
        template <typename Q>
        bool find( Q const & val )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return find_with_( val, key_comparator(), [](value_type& , Q const& ) {} )    ;
#       else
            return find_with_( val, key_comparator(), empty_find_functor() )    ;
#       endif
        }

        /// Find the key \p val with comparing functor \p cmp
        /**
            The function is an analog of \ref find(Q&, Func) but \p cmp is used for key compare.
            \p Less has the interface like \p std::less.
            \p cmp must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q& val, Less cmp, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return find_with_( val, [cmp]( value_type const& v, Q const& q) { return cmp( v, q ) ? -1 : ( cmp( q, v ) ? 1 : 0 ) ; }, f ) ;
#       else
            return find_with_( val, less_wrapper<Less>(), f ) ;
#       endif
        }

        /// Find the key \p val with comparing functor \p cmp
        /**
            The function is an analog of \ref find(Q const&, Func) but \p cmp is used for key compare.
            \p Less has the interface like \p std::less.
            \p cmp must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less, typename Func>
        bool find_with( Q const& val, Less cmp, Func f )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return find_with_( val, [cmp]( value_type const& v, Q const& q) { return cmp( v, q ) ? -1 : ( cmp( q, v ) ? 1 : 0 ) ; }, f ) ;
#       else
            return find_with_( val, less_wrapper<Less>(), f ) ;
#       endif
        }

        /// Find the key \p val with comparing functor \p cmp
        /**
            The function is an analog of \ref find(Q const&) but \p cmp is used for key comparision.
            \p Less functor has the interface like \p std::less.
            \p cmp must imply the same element order as the comparator used for building the set.
        */
        template <typename Q, typename Less>
        bool find_with( Q const& val, Less cmp )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return find_with_( val
                , [cmp]( value_type const& v, Q const& q) { return cmp( v, q ) ? -1 : ( cmp( q, v ) ? 1 : 0 ) ; }
                , [](value_type& , Q const& ) {}
            );
#       else
            return find_with_( val, less_wrapper<Less>(), empty_find_functor() )    ;
#       endif
        }

        /// Returns item count in the set
        /**
            The value returned depends on item counter type provided by \p Traits template parameter.
            If it is atomicity::empty_item_counter this function always returns 0.
            Therefore, the function is not suitable for checking the set emptiness, use \ref empty
            member function for this purpose.
        */
        size_t size() const
        {
            return m_ItemCounter    ;
        }

        /// Checks if the set is empty
        bool empty() const
        {
            return m_Head.head()->next(0).load( memory_model::memory_order_relaxed ) == null_ptr<node_type *>() ;
        }

        /// Clear the set (non-atomic)
        /**
            The function unlink all items from the set.
            The function is not atomic, thus, in multi-threaded environment with parallel insertions
            this sequence
            \code
            set.clear() ;
            assert( set.empty() ) ;
            \endcode
            the assertion could be raised.

            For each item the \ref disposer will be called after unlinking.
        */
        void clear()
        {
            typename gc::Guard guard ;
            for (;;) {
                marked_node_ptr pNode = guard.protect( m_Head.head()->next(0), gc_protect ) ;
                if ( pNode.ptr() )
                    unlink( *node_traits::to_value_ptr( pNode.ptr() )) ;
                else
                    break ;
            }
        }

        /// Returns maximum height of skip-list. The max height is a constant for each object and does not exceed 32.
        static CDS_CONSTEXPR unsigned int max_height() CDS_NOEXCEPT
        {
            return c_nMaxHeight ;
        }

        /// Returns const reference to internal statistics
        stat const& statistics() const
        {
            return m_Stat ;
        }

    };

}} // namespace cds::intrusive


#endif // #ifndef __CDS_INTRUSIVE_SKIP_LIST_IMPL_H

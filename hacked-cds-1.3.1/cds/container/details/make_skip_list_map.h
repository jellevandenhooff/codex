/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_CONTAINER_DETAILS_MAKE_SKIP_LIST_MAP_H
#define __CDS_CONTAINER_DETAILS_MAKE_SKIP_LIST_MAP_H

#include <cds/container/skip_list_base.h>
#include <cds/container/details/compare_wrapper.h>

//@cond
namespace cds { namespace container { namespace details {

    template <typename GC, typename K, typename T, typename Traits>
    struct make_skip_list_map
    {
        typedef GC      gc ;
        typedef K       key_type    ;
        typedef T       mapped_type ;
        typedef std::pair< key_type const, mapped_type> value_type  ;
        typedef Traits  type_traits ;

        typedef cds::intrusive::skip_list::node< gc >   intrusive_node_type ;
        struct node_type: public intrusive_node_type
        {
            typedef intrusive_node_type                     base_class          ;
            typedef typename base_class::atomic_marked_ptr  atomic_marked_ptr   ;
            typedef value_type                              stored_value_type   ;

            value_type m_Value ;
            //atomic_marked_ptr m_arrTower[] ;  // allocated together with node_type in single memory block

            template <typename Q>
            node_type( unsigned int nHeight, atomic_marked_ptr * pTower, Q const& key )
                : m_Value( std::make_pair( key, mapped_type() ))
            {
                init_tower( nHeight, pTower )   ;
            }

            template <typename Q, typename U>
            node_type( unsigned int nHeight, atomic_marked_ptr * pTower, Q const& key, U const& val )
                : m_Value( std::make_pair( key, val ))
            {
                init_tower( nHeight, pTower );
            }

#       ifdef CDS_EMPLACE_SUPPORT
            template <typename Q, typename... Args>
            node_type( unsigned int nHeight, atomic_marked_ptr * pTower, Q&& key, Args&&... args )
                : m_Value( std::forward<Q>(key), std::move( mapped_type( std::forward<Args>(args)... )))
            {
                init_tower( nHeight, pTower );
            }
#       endif

        private:
            node_type() ;   // no default ctor

            void init_tower( unsigned int nHeight, atomic_marked_ptr * pTower )
            {
                if ( nHeight > 1 ) {
                    new (pTower) atomic_marked_ptr[ nHeight - 1 ] ;
                    base_class::make_tower( nHeight, pTower )   ;
                }
            }
        };

        class node_allocator: public skip_list::details::node_allocator< node_type, type_traits>
        {
            typedef skip_list::details::node_allocator< node_type, type_traits> base_class ;
        public:
            template <typename Q>
            node_type * New( unsigned int nHeight, Q const& key )
            {
                return base_class::New( nHeight, key ) ;
            }
            template <typename Q, typename U>
            node_type * New( unsigned int nHeight, Q const& key, U const& val )
            {
                unsigned char * pMem = base_class::alloc_space( nHeight ) ;
                return new( pMem )
                    node_type( nHeight,
                        nHeight > 1 ? reinterpret_cast<typename base_class::node_tower_item *>( pMem + base_class::c_nNodeSize )
                            : null_ptr<typename base_class::node_tower_item *>(),
                        key, val ) ;
            }
#       ifdef CDS_EMPLACE_SUPPORT
            template <typename... Args>
            node_type * New( unsigned int nHeight, Args&&... args )
            {
                unsigned char * pMem = base_class::alloc_space( nHeight ) ;
                return new( pMem )
                    node_type( nHeight, nHeight > 1 ? reinterpret_cast<typename base_class::node_tower_item *>( pMem + base_class::c_nNodeSize )
                        : null_ptr<typename base_class::node_tower_item *>(),
                    std::forward<Args>(args)... ) ;
            }
#       endif
        };

        struct node_deallocator {
            void operator ()( node_type * pNode )
            {
                node_allocator().Delete( pNode ) ;
            }
        };

        typedef skip_list::details::dummy_node_builder<intrusive_node_type> dummy_node_builder ;

        struct key_accessor
        {
            key_type const & operator()( node_type const& node ) const
            {
                return node.m_Value.first ;
            }
        };
        typedef typename opt::details::make_comparator< key_type, type_traits >::type key_comparator ;

        typedef typename cds::intrusive::skip_list::make_traits<
            cds::opt::type_traits< type_traits >
            ,cds::intrusive::opt::hook< intrusive::skip_list::base_hook< cds::opt::gc< gc > > >
            ,cds::intrusive::opt::disposer< node_deallocator >
            ,cds::intrusive::skip_list::internal_node_builder< dummy_node_builder >
            ,cds::opt::compare< compare_wrapper< node_type, key_comparator, key_accessor > >
        >::type intrusive_type_traits ;

        typedef cds::intrusive::SkipListSet< gc, node_type, intrusive_type_traits>   type ;
    };

}}} // namespace cds::container::details
//@endcond

#endif // __CDS_CONTAINER_DETAILS_MAKE_SKIP_LIST_MAP_H

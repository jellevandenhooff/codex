/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_CONTAINER_STRIPED_SET_BOOST_LIST_ADAPTER_H
#define __CDS_CONTAINER_STRIPED_SET_BOOST_LIST_ADAPTER_H

#include <boost/version.hpp>
#if BOOST_VERSION < 104800
#   error "For boost::container::list you must use boost 1.48 or above"
#endif

#include <cds/container/striped_set/adapter.h>
#include <cds/ref.h>
#include <boost/container/list.hpp>
#include <algorithm>    // std::lower_bound

//@cond
namespace cds { namespace container {
    namespace striped_set {

        // Copy policy for boost::container::list
        template <typename T, typename Alloc>
        struct copy_item_policy< boost::container::list< T, Alloc > >
        {
            typedef boost::container::list< T, Alloc > list_type ;
            typedef typename list_type::iterator iterator   ;

            void operator()( list_type& list, iterator itInsert, iterator itWhat )
            {
                itInsert = list.insert( itInsert, *itWhat );
            }
        };

        // Swap policy for boost::container::list
        template <typename T, typename Alloc>
        struct swap_item_policy< boost::container::list< T, Alloc > >
        {
            typedef boost::container::list< T, Alloc > list_type ;
            typedef typename list_type::iterator iterator   ;

            void operator()( list_type& list, iterator itInsert, iterator itWhat )
            {
                typename list_type::value_type newVal    ;
                itInsert = list.insert( itInsert, newVal );
                std::swap( *itWhat, *itInsert )    ;
            }
        };

#ifdef CDS_MOVE_SEMANTICS_SUPPORT
        // Move policy for boost::container::list
        template <typename T, typename Alloc>
        struct move_item_policy< boost::container::list< T, Alloc > >
        {
            typedef boost::container::list< T, Alloc > list_type ;
            typedef typename list_type::iterator iterator   ;

            void operator()( list_type& list, iterator itInsert, iterator itWhat )
            {
                list.insert( itInsert, std::move( *itWhat ) )    ;
            }
        };
#endif
    }   // namespace striped_set
}} // namespace cds::container

namespace cds { namespace intrusive { namespace striped_set {

    /// boost::container::list adapter for hash set bucket
    template <typename T, class Alloc, CDS_SPEC_OPTIONS>
    class adapt< boost::container::list<T, Alloc>, CDS_OPTIONS >
    {
    public:
        typedef boost::container::list<T, Alloc>     container_type          ;   ///< underlying container type

    private:
        /// Adapted container type
        class adapted_container: public cds::container::striped_set::adapted_sequential_container
        {
        public:
            typedef typename container_type::value_type value_type  ;   ///< value type stored in the container
            typedef typename container_type::iterator      iterator ;   ///< container iterator
            typedef typename container_type::const_iterator const_iterator ;    ///< container const iterator

        private:
            //@cond
            typedef typename cds::opt::details::make_comparator_from_option_list< value_type, CDS_OPTIONS >::type key_comparator  ;

            typedef typename cds::opt::select<
                typename cds::opt::value<
                    typename cds::opt::find_option<
                        cds::opt::copy_policy< cds::container::striped_set::move_item >
                        , CDS_OPTIONS
                    >::type
                >::copy_policy
                , cds::container::striped_set::copy_item, cds::container::striped_set::copy_item_policy<container_type>
                , cds::container::striped_set::swap_item, cds::container::striped_set::swap_item_policy<container_type>
#ifdef CDS_MOVE_SEMANTICS_SUPPORT
                , cds::container::striped_set::move_item, cds::container::striped_set::move_item_policy<container_type>
#endif
            >::type copy_item   ;

            struct find_predicate
            {
                bool operator()( value_type const& i1, value_type const& i2) const
                {
                    return key_comparator()( i1, i2 ) < 0 ;
                }

                template <typename Q>
                bool operator()( Q const& i1, value_type const& i2) const
                {
                    return key_comparator()( i1, i2 ) < 0 ;
                }

                template <typename Q>
                bool operator()( value_type const& i1, Q const& i2) const
                {
                    return key_comparator()( i1, i2 ) < 0 ;
                }
            };
            //@endcond

        private:
            //@cond
            container_type  m_List  ;
            //@endcond

        public:
            adapted_container()
            {}

            template <typename Q, typename Func>
            bool insert( Q const& val, Func f )
            {
                iterator it = std::lower_bound( m_List.begin(), m_List.end(), val, find_predicate() ) ;
                if ( it == m_List.end() || key_comparator()( val, *it ) != 0 ) {
                    value_type newItem( val )   ;
                    it = m_List.insert( it, newItem )   ;
                    cds::unref( f )( *it )      ;

                    return true     ;
                }

                // key already exists
                return false    ;
            }

#           ifdef CDS_EMPLACE_SUPPORT
            template <typename... Args>
            bool emplace( Args&&... args )
            {
                value_type val( std::forward<Args>(args)... )   ;
                iterator it = std::lower_bound( m_List.begin(), m_List.end(), val, find_predicate() ) ;
                if ( it == m_List.end() || key_comparator()( val, *it ) != 0 ) {
                    m_List.emplace( it, std::move( val ) )   ;
                    return true     ;
                }
                return false ;
            }
#           endif

            template <typename Q, typename Func>
            std::pair<bool, bool> ensure( Q const& val, Func func )
            {
                iterator it = std::lower_bound( m_List.begin(), m_List.end(), val, find_predicate() ) ;
                if ( it == m_List.end() || key_comparator()( val, *it ) != 0 ) {
                    // insert new
                    value_type newItem( val )               ;
                    it = m_List.insert( it, newItem )       ;
                    cds::unref( func )( true, *it, val )    ;
                    return std::make_pair( true, true )     ;
                }
                else {
                    // already exists
                    cds::unref( func )( false, *it, val )   ;
                    return std::make_pair( true, false )    ;
                }
            }

            template <typename Q, typename Func>
            bool erase( Q const& key, Func f )
            {
                iterator it = std::lower_bound( m_List.begin(), m_List.end(), key, find_predicate() ) ;
                if ( it == m_List.end() || key_comparator()( key, *it ) != 0 )
                    return false ;

                // key exists
                cds::unref( f )( *it )  ;
                m_List.erase( it )      ;

                return true     ;
            }

            template <typename Q, typename Func>
            bool find( Q& val, Func f )
            {
                iterator it = std::lower_bound( m_List.begin(), m_List.end(), val, find_predicate() ) ;
                if ( it == m_List.end() || key_comparator()( val, *it ) != 0 )
                    return false ;

                // key exists
                cds::unref( f )( *it, val )  ;
                return true     ;
            }

            /// Clears the container
            void clear()
            {
                m_List.clear()    ;
            }

            iterator begin()                { return m_List.begin(); }
            const_iterator begin() const    { return m_List.begin(); }
            iterator end()                  { return m_List.end(); }
            const_iterator end() const      { return m_List.end(); }

            void move_item( adapted_container& /*from*/, iterator itWhat )
            {
                iterator it = std::lower_bound( m_List.begin(), m_List.end(), *itWhat, find_predicate() ) ;
                assert( it == m_List.end() || key_comparator()( *itWhat, *it ) != 0 )   ;

                copy_item()( m_List, it, itWhat )    ;
            }

            size_t size() const
            {
                return m_List.size() ;
            }
        };

    public:
        typedef adapted_container type ; ///< Result of \p adapt metafunction

    };
}}} // namespace cds::intrsive::striped_set
//@endcond

#endif // #ifndef __CDS_CONTAINER_STRIPED_SET_BOOST_LIST_ADAPTER_H

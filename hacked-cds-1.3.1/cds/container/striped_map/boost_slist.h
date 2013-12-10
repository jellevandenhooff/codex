/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_CONTAINER_STRIPED_MAP_BOOST_SLIST_ADAPTER_H
#define __CDS_CONTAINER_STRIPED_MAP_BOOST_SLIST_ADAPTER_H

#include <boost/version.hpp>
#if BOOST_VERSION < 104800
#   error "For boost::container::slist you must use boost 1.48 or above"
#endif

#include <cds/container/striped_set/adapter.h>
#include <cds/ref.h>
#include <boost/container/slist.hpp>
#include <utility>      // std::pair

//@cond
namespace cds { namespace container {
    namespace striped_set {

        // Copy policy for map
        template <typename K, typename T, typename Alloc>
        struct copy_item_policy< boost::container::slist< std::pair< K const, T >, Alloc > >
        {
            typedef std::pair< K const, T>  pair_type ;
            typedef boost::container::slist< pair_type, Alloc > list_type ;
            typedef typename list_type::iterator iterator ;

            void operator()( list_type& list, iterator itInsert, iterator itWhat )
            {
                itInsert = list.insert_after( itInsert, *itWhat );
            }
        };

        // Swap policy for map
        template <typename K, typename T, typename Alloc>
        struct swap_item_policy< boost::container::slist< std::pair< K const, T >, Alloc > >
        {
            typedef std::pair< K const, T>  pair_type ;
            typedef boost::container::slist< pair_type, Alloc > list_type ;
            typedef typename list_type::iterator iterator ;

            void operator()( list_type& list, iterator itInsert, iterator itWhat )
            {
                pair_type newVal( itWhat->first, typename pair_type::mapped_type() )    ;
                itInsert = list.insert_after( itInsert, newVal );
                std::swap( itInsert->second, itWhat->second )    ;
            }
        };

#ifdef CDS_MOVE_SEMANTICS_SUPPORT
        // Move policy for map
        template <typename K, typename T, typename Alloc>
        struct move_item_policy< boost::container::slist< std::pair< K const, T >, Alloc > >
        {
            typedef std::pair< K const, T>  pair_type ;
            typedef boost::container::slist< pair_type, Alloc > list_type ;
            typedef typename list_type::iterator iterator ;

            void operator()( list_type& list, iterator itInsert, iterator itWhat )
            {
                list.insert_after( itInsert, std::move( *itWhat ) )    ;
            }
        };
#endif
    } // namespace striped_set
}} // namespace cds:container

namespace cds { namespace intrusive { namespace striped_set {

    /// boost::container::slist adapter for hash map bucket
    template <typename Key, typename T, class Alloc, CDS_SPEC_OPTIONS>
    class adapt< boost::container::slist< std::pair<Key const, T>, Alloc>, CDS_OPTIONS >
    {
    public:
        typedef boost::container::slist< std::pair<Key const, T>, Alloc>     container_type          ;   ///< underlying container type

    private:
        /// Adapted container type
        class adapted_container: public cds::container::striped_set::adapted_sequential_container
        {
        public:
            typedef typename container_type::value_type     value_type  ;   ///< value type stored in the container
            typedef typename value_type::first_type         key_type    ;
            typedef typename value_type::second_type        mapped_type ;
            typedef typename container_type::iterator       iterator ;   ///< container iterator
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

            template <typename Q>
            std::pair< iterator, bool > find_prev_item( Q const& key )
            {
                iterator itPrev = m_List.before_begin() ;
                iterator itEnd = m_List.end() ;
                for ( iterator it = m_List.begin(); it != itEnd; ++it ) {
                    int nCmp = key_comparator()( key, it->first ) ;
                    if ( nCmp < 0 )
                        itPrev = it ;
                    else if ( nCmp > 0 )
                        break;
                    else
                        return std::make_pair( itPrev, true ) ;
                }
                return std::make_pair( itPrev, false )   ;
            }
            //@endcond

        private:
            //@cond
            container_type  m_List  ;
            //@endcond

        public:
            adapted_container()
            {}

            /// Insert value \p key of type \p Q into the container
            /**
                The function allows to split creating of new item into two part:
                - create item with key only from \p key
                - try to insert new item into the container
                - if inserting is success, calls \p f functor to initialize value-field of the new item.

                The functor signature is:
                \code
                    void func( value_type& item ) ;
                \endcode
                where \p item is the item inserted.

                The type \p Q can differ from \ref value_type of items storing in the container.
                Therefore, the \p value_type should be comparable with type \p Q and constructible from type \p Q,

                The user-defined functor is called only if the inserting is success. It cab be passed by reference
                using <tt>boost::ref</tt>
            */
            template <typename Q, typename Func>
            bool insert( const Q& key, Func f )
            {
                std::pair< iterator, bool > pos = find_prev_item( key )    ;
                if ( !pos.second ) {
                    value_type newItem( key, mapped_type() )   ;
                    pos.first = m_List.insert_after( pos.first, newItem )   ;
                    cds::unref( f )( *pos.first )      ;
                    return true     ;
                }

                // key already exists
                return false    ;
            }

#           ifdef CDS_EMPLACE_SUPPORT
            template <typename K, typename... Args>
            bool emplace( K&& key, Args&&... args )
            {
                std::pair< iterator, bool > pos = find_prev_item( key )    ;
                if ( !pos.second ) {
                    m_List.emplace_after( pos.first, std::forward<K>(key), std::move( mapped_type( std::forward<Args>(args)... )))   ;
                    return true     ;
                }
                return false ;
            }
#           endif

            /// Ensures that the \p item exists in the container
            /**
                The operation performs inserting or changing data.

                If the \p val key not found in the container, then the new item created from \p val
                is inserted. Otherwise, the functor \p func is called with the item found.
                The \p Func functor has interface:
                \code
                    void func( bool bNew, value_type& item, const Q& val ) ;
                \endcode
                or like a functor:
                \code
                    struct my_functor {
                        void operator()( bool bNew, value_type& item, const Q& val ) ;
                    };
                \endcode

                where arguments are:
                - \p bNew - \p true if the item has been inserted, \p false otherwise
                - \p item - container's item
                - \p val - argument \p val passed into the \p ensure function

                The functor can change non-key fields of the \p item.

                The type \p Q can differ from \ref value_type of items storing in the container.
                Therefore, the \p value_type should be comparable with type \p Q and constructible from type \p Q,

                You can pass \p func argument by reference using <tt>boost::ref</tt>.

                Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
                \p second is true if new item has been added or \p false if the item with \p val key
                already exists.
            */
            template <typename Q, typename Func>
            std::pair<bool, bool> ensure( const Q& key, Func func )
            {
                std::pair< iterator, bool > pos = find_prev_item( key )    ;
                if ( !pos.second ) {
                    // insert new
                    value_type newItem( key, mapped_type() )   ;
                    pos.first = m_List.insert_after( pos.first, newItem )           ;
                    cds::unref( func )( true, *pos.first )    ;
                    return std::make_pair( true, true )     ;
                }
                else {
                    // already exists
                    cds::unref( func )( false, *(++pos.first) )   ;
                    return std::make_pair( true, false )    ;
                }
            }

            /// Delete \p key
            /**
                The function searches an item with key \p key, calls \p f functor
                and deletes the item. If \p key is not found, the functor is not called.

                The functor \p Func interface is:
                \code
                struct extractor {
                    void operator()(value_type const& val) ;
                };
                \endcode
                The functor can be passed by reference using <tt>boost:ref</tt>

                The type \p Q can differ from \ref value_type of items storing in the container.
                Therefore, the \p value_type should be comparable with type \p Q.

                Return \p true if key is found and deleted, \p false otherwise
            */
            template <typename Q, typename Func>
            bool erase( const Q& key, Func f )
            {
                std::pair< iterator, bool > pos = find_prev_item( key )    ;
                if ( !pos.second )
                    return false ;

                // key exists
                iterator it = pos.first ;
                cds::unref( f )( *(++it) )  ;
                m_List.erase_after( pos.first )      ;

                return true     ;
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

                You can pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

                The functor can change non-key fields of \p item.
                The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
                can modify both arguments.

                The type \p Q can differ from \ref value_type of items storing in the container.
                Therefore, the \p value_type should be comparable with type \p Q.

                The function returns \p true if \p val is found, \p false otherwise.
            */
            template <typename Q, typename Func>
            bool find( Q& val, Func f )
            {
                std::pair< iterator, bool > pos = find_prev_item( val )    ;
                if ( !pos.second )
                    return false ;

                // key exists
                cds::unref( f )( *(++pos.first), val )  ;
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
                std::pair< iterator, bool > pos = find_prev_item( itWhat->first )    ;
                assert( !pos.second )   ;

                copy_item()( m_List, pos.first, itWhat )   ;
            }

            size_t size() const
            {
                return m_List.size() ;
            }
        };

    public:
        typedef adapted_container type ; ///< Result of \p adapt metafunction

    };
}}} // namespace cds::intrusive::striped_set


//@endcond

#endif // #ifndef __CDS_CONTAINER_STRIPED_MAP_BOOST_SLIST_ADAPTER_H

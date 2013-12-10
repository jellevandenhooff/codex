/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_CONTAINER_STRIPED_SET_STD_LIST_ADAPTER_H
#define __CDS_CONTAINER_STRIPED_SET_STD_LIST_ADAPTER_H

#include <cds/container/striped_set/adapter.h>
#include <cds/ref.h>
#include <list>
#include <algorithm>    // std::lower_bound

//@cond
namespace cds { namespace container {
    namespace striped_set {

        // Copy policy for std::list
        template <typename T, typename Alloc>
        struct copy_item_policy< std::list< T, Alloc > >
        {
            typedef std::list< T, Alloc > list_type ;
            typedef typename list_type::iterator iterator   ;

            void operator()( list_type& list, iterator itInsert, iterator itWhat )
            {
                itInsert = list.insert( itInsert, *itWhat );
            }
        };

        // Swap policy for std::list
        template <typename T, typename Alloc>
        struct swap_item_policy< std::list< T, Alloc > >
        {
            typedef std::list< T, Alloc > list_type ;
            typedef typename list_type::iterator iterator   ;

            void operator()( list_type& list, iterator itInsert, iterator itWhat )
            {
                typename list_type::value_type newVal    ;
                itInsert = list.insert( itInsert, newVal );
                std::swap( *itWhat, *itInsert )    ;
            }
        };

#ifdef CDS_MOVE_SEMANTICS_SUPPORT
        // Move policy for std::list
        template <typename T, typename Alloc>
        struct move_item_policy< std::list< T, Alloc > >
        {
            typedef std::list< T, Alloc > list_type ;
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

    /// std::list adapter for hash set bucket
    template <typename T, class Alloc, CDS_SPEC_OPTIONS>
    class adapt< std::list<T, Alloc>, CDS_OPTIONS >
    {
    public:
        typedef std::list<T, Alloc>     container_type          ;   ///< underlying container type

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
#       ifdef __GLIBCXX__
            // GCC C++ lib bug:
            // In GCC (at least up to 4.7.x), the complexity of std::list::size() is O(N)
            // (see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=49561)
            size_t          m_nSize ;   // list size
#       endif
            //@endcond

        public:
            adapted_container()
#       ifdef __GLIBCXX__
                : m_nSize(0)
#       endif
            {}

            /// Insert value \p val of type \p Q into the container
            /**
                The function allows to split creating of new item into two part:
                - create item with key only from \p val
                - try to insert new item into the container
                - if inserting is success, calls \p f functor to initialize value-field of the new item.

                The functor signature is:
                \code
                    void func( value_type& item ) ;
                \endcode
                where \p item is the item inserted.

                The type \p Q may differ from \ref value_type of items storing in the container.
                Therefore, the \p value_type should be comparable with type \p Q and constructible from type \p Q,

                The user-defined functor is called only if the inserting is success. It may be passed by reference
                using <tt>boost::ref</tt>
            */
            template <typename Q, typename Func>
            bool insert( const Q& val, Func f )
            {
                iterator it = std::lower_bound( m_List.begin(), m_List.end(), val, find_predicate() ) ;
                if ( it == m_List.end() || key_comparator()( val, *it ) != 0 ) {
                    value_type newItem( val )   ;
                    it = m_List.insert( it, newItem )   ;
                    cds::unref( f )( *it )      ;

#           ifdef __GLIBCXX__
                    ++m_nSize       ;
#           endif
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
                    it = m_List.emplace( it, std::move( val ) )   ;
#           ifdef __GLIBCXX__
                    ++m_nSize       ;
#           endif
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

                The functor may change non-key fields of the \p item.

                The type \p Q may differ from \ref value_type of items storing in the container.
                Therefore, the \p value_type should be comparable with type \p Q and constructible from type \p Q,

                You may pass \p func argument by reference using <tt>boost::ref</tt>.

                Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
                \p second is true if new item has been added or \p false if the item with \p val key
                already exists.
            */
            template <typename Q, typename Func>
            std::pair<bool, bool> ensure( const Q& val, Func func )
            {
                iterator it = std::lower_bound( m_List.begin(), m_List.end(), val, find_predicate() ) ;
                if ( it == m_List.end() || key_comparator()( val, *it ) != 0 ) {
                    // insert new
                    value_type newItem( val )               ;
                    it = m_List.insert( it, newItem )       ;
                    cds::unref( func )( true, *it, val )    ;
#           ifdef __GLIBCXX__
                    ++m_nSize       ;
#           endif
                    return std::make_pair( true, true )     ;
                }
                else {
                    // already exists
                    cds::unref( func )( false, *it, val )   ;
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
                The functor may be passed by reference using <tt>boost:ref</tt>

                The type \p Q may differ from \ref value_type of items storing in the container.
                Therefore, the \p value_type should be comparable with type \p Q.

                Return \p true if key is found and deleted, \p false otherwise
            */
            template <typename Q, typename Func>
            bool erase( const Q& key, Func f )
            {
                iterator it = std::lower_bound( m_List.begin(), m_List.end(), key, find_predicate() ) ;
                if ( it == m_List.end() || key_comparator()( key, *it ) != 0 )
                    return false ;

                // key exists
                cds::unref( f )( *it )  ;
                m_List.erase( it )      ;
#           ifdef __GLIBCXX__
                --m_nSize       ;
#           endif

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

                You may pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

                The functor may change non-key fields of \p item.
                The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
                may modify both arguments.

                The type \p Q may differ from \ref value_type of items storing in the container.
                Therefore, the \p value_type should be comparable with type \p Q.

                The function returns \p true if \p val is found, \p false otherwise.
            */
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
#           ifdef __GLIBCXX__
                ++m_nSize       ;
#           endif
            }

            size_t size() const
            {
#           ifdef __GLIBCXX__
                return m_nSize       ;
#           else
                return m_List.size() ;
#           endif

            }
        };

    public:
        typedef adapted_container type ; ///< Result of \p adapt metafunction

    };
}}}


//@endcond

#endif // #ifndef __CDS_CONTAINER_STRIPED_SET_STD_LIST_ADAPTER_H

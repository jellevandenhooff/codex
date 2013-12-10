/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_CONTAINER_MICHAEL_LIST_IMPL_H
#define __CDS_CONTAINER_MICHAEL_LIST_IMPL_H

#include <cds/details/std/memory.h>

namespace cds { namespace container {

    /// Michael's ordered list
    /** @ingroup cds_nonintrusive_list
        \anchor cds_nonintrusive_MichaelList_gc

        Usually, ordered single-linked list is used as a building block for the hash table implementation.

        Source:
        - [2002] Maged Michael "High performance dynamic lock-free hash tables and list-based sets"

        It is non-intrusive version of cds::intrusive::MichaelList class

        Template arguments:
        - \p GC - garbage collector used
        - \p T - type stored in the list. The type must be default- and copy-constructible.
        - \p Traits - type traits, default is michael_list::type_traits

        Unlike standard container, this implementation does not divide type \p T into key and value part and
        may be used as main building block for hash set algorithms.

        The key is a function (or a part) of type \p T, and this function is specified by <tt> Traits::compare </tt> functor
        or <tt> Traits::less </tt> predicate

        MichaelKVList is a key-value version of Michael's non-intrusive list that is closer to the C++ std library approach.

        It is possible to declare option-based list with cds::container::michael_list::make_traits metafunction istead of \p Traits template
        argument. For example, the following traits-based declaration of gc::HP Michael's list
        \code
        #include <cds/container/michael_list_hp.h>
        // Declare comparator for the item
        struct my_compare {
            int operator ()( int i1, int i2 )
            {
                return i1 - i2  ;
            }
        };

        // Declare type_traits
        struct my_traits: public cds::container::michael_list::type_traits
        {
            typedef my_compare compare ;
        };

        // Declare traits-based list
        typedef cds::container::MichaelList< cds::gc::HP, int, my_traits >     traits_based_list   ;
        \endcode

        is equivalent for the following option-based list
        \code
        #include <cds/container/michael_list_hp.h>

        // my_compare is the same

        // Declare option-based list
        typedef cds::container::MichaelList< cds::gc::HP, int,
            typename cds::container::michael_list::make_traits<
                cds::container::opt::compare< my_compare >     // item comparator option
            >::type
        >     option_based_list   ;
        \endcode

        Template argument list \p Options of cds::container::michael_list::make_traits metafunction are:
        - opt::compare - key comparison functor. No default functor is provided.
            If the option is not specified, the opt::less is used.
        - opt::less - specifies binary predicate used for key comparision. Default is \p std::less<T>.
        - opt::back_off - back-off strategy used. If the option is not specified, the cds::backoff::empty is used.
        - opt::item_counter - the type of item counting feature. Default is \ref atomicity::empty_item_counter that is no item counting.
        - opt::allocator - the allocator used for creating and freeing list's item. Default is \ref CDS_DEFAULT_ALLOCATOR macro.
        - opt::memory_model - C++ memory ordering model. Can be opt::v::relaxed_ordering (relaxed memory model, the default)
            or opt::v::sequential_consistent (sequentially consisnent memory model).

        \par Usage
        There are different specializations of this template for each garbage collecting schema used.
        You should include appropriate .h-file depending on GC you are using:
        - for gc::HP: \code #include <cds/container/michael_list_hp.h> \endcode
        - for gc::PTB: \code #include <cds/container/michael_list_ptb.h> \endcode
        - for gc::HRC: \code #include <cds/container/michael_list_hrc.h> \endcode
        - for gc::nogc: \code #include <cds/container/michael_list_nogc.h> \endcode
    */
    template <
        typename GC,
        typename T,
#ifdef CDS_DOXYGEN_INVOKED
        typename Traits = michael_list::type_traits
#else
        typename Traits
#endif
    >
    class MichaelList:
#ifdef CDS_DOXYGEN_INVOKED
        protected intrusive::MichaelList< GC, T, Traits >
#else
        protected details::make_michael_list< GC, T, Traits >::type
#endif
    {
        //@cond
        typedef details::make_michael_list< GC, T, Traits > options ;
        typedef typename options::type  base_class   ;
        //@endcond

    public:
        typedef T                                   value_type      ;   ///< Type of value stored in the list
        typedef typename base_class::gc             gc              ;   ///< Garbage collector used
        typedef typename base_class::back_off       back_off        ;   ///< Back-off strategy used
        typedef typename options::allocator_type    allocator_type  ;   ///< Allocator type used for allocate/deallocate the nodes
        typedef typename base_class::item_counter   item_counter    ;   ///< Item counting policy used
        typedef typename options::key_comparator    key_comparator  ;   ///< key comparision functor
        typedef typename base_class::memory_model   memory_model    ;   ///< Memory ordering. See cds::opt::memory_model option

    protected:
        //@cond
        typedef typename base_class::value_type     node_type       ;
        typedef typename options::cxx_allocator     cxx_allocator   ;
        typedef typename options::node_deallocator  node_deallocator;

        typedef typename base_class::atomic_node_ptr head_type      ;
#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        typedef typename base_class::empty_erase_functor    empty_erase_functor ;
#   endif
        //@endcond

    private:
        //@cond
        static value_type& node_to_value( node_type& n )
        {
            return n.m_Value    ;
        }
        static value_type const& node_to_value( node_type const& n )
        {
            return n.m_Value    ;
        }

#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        template <typename Func>
        struct insert_functor
        {
            Func        m_func  ;

            insert_functor ( Func f )
                : m_func(f)
            {}

            void operator()( node_type& node )
            {
                cds::unref(m_func)( node_to_value(node) )  ;
            }
        };

        template <typename Q, typename Func>
        struct ensure_functor
        {
            Func        m_func  ;
            Q const&    m_arg   ;

            ensure_functor( Q const& arg, Func f )
                : m_func(f)
                , m_arg( arg )
            {}

            void operator ()( bool bNew, node_type& node, node_type& )
            {
                cds::unref(m_func)( bNew, node_to_value(node), m_arg ) ;
            }
        };

        template <typename Func>
        struct find_functor
        {
            Func    m_func  ;

            find_functor( Func f )
                : m_func(f)
            {}

            template <typename Q>
            void operator ()( node_type& node, Q& val )
            {
                cds::unref(m_func)( node_to_value(node), val ) ;
            }
        };

        template <typename Func>
        struct erase_functor
        {
            Func        m_func  ;

            erase_functor( Func f )
                : m_func(f)
            {}

            void operator()( node_type const& node )
            {
                cds::unref(m_func)( node_to_value(node) )  ;
            }
        };
#endif  // ifndef CDS_CXX11_LAMBDA_SUPPORT
        //@endcond

    protected:
        //@cond
        template <typename Q>
        static node_type * alloc_node( Q const& v )
        {
            return cxx_allocator().New( v ) ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        template <typename... Args>
        static node_type * alloc_node( Args&&... args )
        {
            return cxx_allocator().MoveNew( std::forward<Args>(args)... ) ;
        }
#   endif

        static void free_node( node_type * pNode )
        {
            cxx_allocator().Delete( pNode ) ;
        }

        struct node_disposer {
            void operator()( node_type * pNode )
            {
                free_node( pNode )  ;
            }
        };
        typedef std::unique_ptr< node_type, node_disposer >     scoped_node_ptr ;

        head_type& head()
        {
            return base_class::m_pHead  ;
        }

        head_type const& head() const
        {
            return base_class::m_pHead  ;
        }
        //@endcond

    protected:
                //@cond
        template <bool IsConst>
        class iterator_type: protected base_class::template iterator_type<IsConst>
        {
            typedef typename base_class::template iterator_type<IsConst>    iterator_base ;

            iterator_type( head_type const& pNode )
                : iterator_base( pNode )
            {}

            friend class MichaelList ;

        public:
            typedef typename cds::details::make_const_type<value_type, IsConst>::pointer   value_ptr;
            typedef typename cds::details::make_const_type<value_type, IsConst>::reference value_ref;

            iterator_type()
            {}

            iterator_type( iterator_type const& src )
                : iterator_base( src )
            {}

            value_ptr operator ->() const
            {
                typename iterator_base::value_ptr p = iterator_base::operator ->()      ;
                return p ? &(p->m_Value) : reinterpret_cast<value_ptr>(NULL)    ;
            }

            value_ref operator *() const
            {
                return (iterator_base::operator *()).m_Value  ;
            }

            /// Pre-increment
            iterator_type& operator ++()
            {
                iterator_base::operator ++()    ;
                return *this;
            }

            template <bool C>
            bool operator ==(iterator_type<C> const& i ) const
            {
                return iterator_base::operator ==(i)    ;
            }
            template <bool C>
            bool operator !=(iterator_type<C> const& i ) const
            {
                return iterator_base::operator !=(i)    ;
            }
        };
        //@endcond

    public:
        /// Forward iterator
        /**
            The forward iterator for Michael's list has some features:
            - it has no post-increment operator
            - to protect the value, the iterator contains a GC-specific guard + another guard is required locally for increment operator.
              For some GC (gc::HP, gc::HRC), a guard is limited resource per thread, so an exception (or assertion) "no free guard"
              may be thrown if a limit of guard count per thread is exceeded.
            - The iterator cannot be moved across thread boundary since it contains GC's guard that is thread-private GC data.
            - Iterator ensures thread-safety even if you delete the item that iterator points to. However, in case of concurrent
              deleting operations it is no guarantee that you iterate all item in the list.

            Therefore, the use of iterators in concurrent environment is not good idea. Use the iterator on the concurrent container
            for debug purpose only.
        */
        typedef iterator_type<false>    iterator        ;

        /// Const forward iterator
        /**
            For iterator's features and requirements see \ref iterator
        */
        typedef iterator_type<true>     const_iterator  ;

        /// Returns a forward iterator addressing the first element in a list
        /**
            For empty list \code begin() == end() \endcode
        */
        iterator begin()
        {
            return iterator( head() )    ;
        }

        /// Returns an iterator that addresses the location succeeding the last element in a list
        /**
            Do not use the value returned by <tt>end</tt> function to access any item.
            Internally, <tt>end</tt> returning value equals to <tt>NULL</tt>.

            The returned value can be used only to control reaching the end of the list.
            For empty list \code begin() == end() \endcode
        */
        iterator end()
        {
            return iterator()   ;
        }

        /// Returns a forward const iterator addressing the first element in a list
        const_iterator begin() const
        {
            return const_iterator( head() )    ;
        }

        /// Returns an const iterator that addresses the location succeeding the last element in a list
        const_iterator end() const
        {
            return const_iterator()   ;
        }

    public:
        /// Default constructor
        /**
            Initialize empty list
        */
        MichaelList()
        {}

        /// List desctructor
        /**
            Clears the list
        */
        ~MichaelList()
        {
            clear() ;
        }

        /// Inserts new node
        /**
            The function creates a node with copy of \p val value
            and then inserts the node created into the list.

            The type \p Q should contain as minimum the complete key of the node.
            The object of \ref value_type should be constructible from \p val of type \p Q.
            In trivial case, \p Q is equal to \ref value_type.

            Returns \p true if inserting successful, \p false otherwise.
        */
        template <typename Q>
        bool insert( Q const& val )
        {
            return insert_at( head(), val )    ;
        }

        /// Inserts new node
        /**
            This function inserts new node with default-constructed value and then it calls
            \p func functor with signature
            \code void func( value_type& itemValue ) ;\endcode

            The argument \p itemValue of user-defined functor \p func is the reference
            to the list's item inserted. User-defined functor \p func should guarantee that during changing
            item's value no any other changes could be made on this list's item by concurrent threads.
            The user-defined functor can be passed by reference using <tt>boost::ref</tt>
            and it is called only if the inserting is success.

            The type \p Q should contain the complete key of the node.
            The object of \ref value_type should be constructible from \p key of type \p Q.

            The function allows to split creating of new item into two part:
            - create item from \p key with initializing key-fields only;
            - insert new item into the list;
            - if inserting is successful, initialize non-key fields of item by calling \p f functor

            This can be useful if complete initialization of object of \p value_type is heavyweight and
            it is preferable that the initialization should be completed only if inserting is successful.
        */
        template <typename Q, typename Func>
        bool insert( Q const& key, Func func )
        {
            return insert_at( head(), key, func )    ;
        }

        /// Ensures that the \p key exists in the list
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the \p key not found in the list, then the new item created from \p key
            is inserted into the list. Otherwise, the functor \p func is called with the item found.
            The functor \p Func should be a function with signature:
            \code
                void func( bool bNew, value_type& item, const Q& val ) ;
            \endcode
            or a functor:
            \code
                struct my_functor {
                    void operator()( bool bNew, value_type& item, const Q& val ) ;
                };
            \endcode

            with arguments:
            - \p bNew - \p true if the item has been inserted, \p false otherwise
            - \p item - item of the list
            - \p val - argument \p key passed into the \p ensure function

            The functor may change non-key fields of the \p item; however, \p func must guarantee
            that during changing no any other modifications could be made on this item by concurrent threads.

            You may pass \p func argument by reference using <tt>boost::ref</tt>.

            Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already is in the list.
        */
        template <typename Q, typename Func>
        std::pair<bool, bool> ensure( Q const& key, Func f )
        {
            return ensure_at( head(), key, f ) ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        /// Inserts data of type \ref value_type constructed with <tt>std::forward<Args>(args)...</tt>
        /**
            Returns \p true if inserting successful, \p false otherwise.

            This function is available only for compiler that supports
            variadic template and move semantics
        */
        template <typename... Args>
        bool emplace( Args&&... args )
        {
            return emplace_at( head(), std::forward<Args>(args)... ) ;
        }
#   endif

        /// Delete \p key from the list
        /**
            Since the key of MichaelList's item type \p T is not explicitly specified,
            template parameter \p Q defines the key type searching in the list.
            The list item comparator should be able to compare the type \p T of list item
            and the type \p Q.

            Return \p true if key is found and deleted, \p false otherwise
        */
        template <typename Q>
        bool erase( Q const& key )
        {
#       ifdef CDS_CXX11_LAMBDA_SUPPORT
            return erase_at( head(), key, [](value_type const&){} ) ;
#       else
            return erase_at( head(), key, empty_erase_functor() ) ;
#       endif
        }

        /// Get value of item with \p key by a functor and delete it
        /**
            The function searches an item with key \p key, calls \p f functor
            and deletes the item. If \p key is not found, the functor is not called.

            The functor \p Func interface:
            \code
            struct extractor {
                void operator()(const value_type& val) { ... }
            };
            \endcode
            The functor may be passed by reference with <tt>boost:ref</tt>

            Since the key of MichaelList's item type \p T is not explicitly specified,
            template parameter \p Q defines the key type searching in the list.
            The list item comparator should be able to compare the type \p T of list item
            and the type \p Q.

            Return \p true if key is found and deleted, \p false otherwise

            See also: \ref erase
        */
        template <typename Q, typename Func>
        bool erase( Q const& key, Func f )
        {
            return erase_at( head(), key, f ) ;
        }

        /// Find the key \p key
        /**
            The function searches the item with key equal to \p key
            and returns \p true if it is found, and \p false otherwise
        */
        template <typename Q>
        bool find( Q const& key )
        {
            return find_at( head(), key )  ;
        }

        /// Find the key \p val and perform an action with it
        /**
            The function searches an item with key equal to \p val and calls the functor \p f for the item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You may pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change non-key fields of \p item. Note that the function is only guarantee
            that \p item cannot be deleted during functor is executing.
            The function does not serialize simultaneous access to the list \p item. If such access is
            possible you must provide your own synchronization schema to exclude unsafe item modifications.

            The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
            may modify both arguments.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q& val, Func f )
        {
            return find_at( head(), val, f )  ;
        }

        /// Find the key \p val and perform an action with it
        /**
            The function searches an item with key equal to \p val and calls the functor \p f for the item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item, Q const& val ) ;
            };
            \endcode
            where \p item is the item found, \p val is the <tt>find</tt> function argument.

            You may pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change non-key fields of \p item. Note that the function is only guarantee
            that \p item cannot be deleted during functor is executing.
            The function does not serialize simultaneous access to the list \p item. If such access is
            possible you must provide your own synchronization schema to exclude unsafe item modifications.

            The \p val argument is non-const since it can be used as \p f functor destination i.e., the functor
            may modify both arguments.

            The function returns \p true if \p val is found, \p false otherwise.
        */
        template <typename Q, typename Func>
        bool find( Q const& val, Func f )
        {
            return find_at( head(), val, f )  ;
        }

        /// Check if the list is empty
        bool empty() const
        {
            return base_class::empty()  ;
        }

        /// Returns list's item count
        /**
            The value returned depends on opt::item_counter option. For atomics::empty_item_counter,
            this function always returns 0.

            <b>Warning</b>: even if you use real item counter and it returns 0, this fact is not mean that the list
            is empty. To check list emptyness use \ref empty() method.
        */
        size_t size() const
        {
            return base_class::size()   ;
        }

        /// Clears the list
        /**
            Post-condition: the list is empty
        */
        void clear()
        {
            base_class::clear() ;
        }

    protected:
        //@cond
        bool insert_node_at( head_type& refHead, node_type * pNode )
        {
            assert( pNode != NULL ) ;
            scoped_node_ptr p(pNode) ;
            if ( base_class::insert_at( refHead, *pNode )) {
                p.release() ;
                return true ;
            }

            return false    ;
        }

        template <typename Q>
        bool insert_at( head_type& refHead, Q const& val )
        {
            return insert_node_at( refHead, alloc_node( val ))   ;
        }

        template <typename Q, typename Func>
        bool insert_at( head_type& refHead, Q const& key, Func f )
        {
            scoped_node_ptr pNode( alloc_node( key )) ;

#   ifdef CDS_CXX11_LAMBDA_SUPPORT
#       ifdef CDS_BUG_STATIC_MEMBER_IN_LAMBDA
            // GCC 4.5,4.6,4.7: node_to_value is unaccessible from lambda,
            // like as MichaelList::node_to_value that requires to capture *this* despite on node_to_value is static function
            value_type& (* n2v)( node_type& ) = node_to_value ;
            if ( base_class::insert_at( refHead, *pNode, [&f, n2v]( node_type& node ) { cds::unref(f)( n2v(node) ); } ))
#       else
            if ( base_class::insert_at( refHead, *pNode, [&f]( node_type& node ) { cds::unref(f)( node_to_value(node) ); } ))
#       endif
#   else
            insert_functor<Func>  wrapper( f ) ;
            if ( base_class::insert_at( refHead, *pNode, cds::ref(wrapper) ))
#   endif
            {
                pNode.release() ;
                return true ;
            }
            return false    ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        template <typename... Args>
        bool emplace_at( head_type& refHead, Args&&... args )
        {
            return insert_node_at( refHead, alloc_node( std::forward<Args>(args) ... )) ;
        }
#   endif

        template <typename Q, typename Func>
        bool erase_at( head_type& refHead, Q const& key, Func f )
        {
#   ifdef CDS_CXX11_LAMBDA_SUPPORT
#       ifdef CDS_BUG_STATIC_MEMBER_IN_LAMBDA
            // GCC 4.5-4.7: node_to_value is unaccessible from lambda,
            // like as MichaelList::node_to_value that requires to capture *this* despite on node_to_value is static function
            value_type const& (* n2v)( node_type const& ) = node_to_value ;
            return base_class::erase_at( refHead, key, [&f,n2v](node_type const& node){ cds::unref(f)( n2v(node) ); } )  ;
#       else
            return base_class::erase_at( refHead, key, [&f](node_type const& node){ cds::unref(f)( node_to_value(node) ); } )  ;
#       endif
#   else
            erase_functor<Func> wrapper( f )    ;
            return base_class::erase_at( refHead, key, cds::ref(wrapper) )  ;
#   endif
        }

        template <typename Q, typename Func>
        std::pair<bool, bool> ensure_at( head_type& refHead, Q const& key, Func f )
        {
            scoped_node_ptr pNode( alloc_node( key )) ;

#   ifdef CDS_CXX11_LAMBDA_SUPPORT
#       ifdef CDS_BUG_STATIC_MEMBER_IN_LAMBDA
            // GCC 4.5-4.7: node_to_value is unaccessible from lambda,
            // like as MichaelList::node_to_value that requires to capture *this* despite on node_to_value is static function
            value_type& (* n2v)( node_type& ) = node_to_value ;
            std::pair<bool, bool> ret = base_class::ensure_at( refHead, *pNode,
                [&f, &key, n2v](bool bNew, node_type& node, node_type&){ cds::unref(f)( bNew, n2v(node), key ); }) ;
#       else
            std::pair<bool, bool> ret = base_class::ensure_at( refHead, *pNode,
                [&f, &key](bool bNew, node_type& node, node_type&){ cds::unref(f)( bNew, node_to_value(node), key ); }) ;
#       endif
#   else
            ensure_functor<Q, Func> wrapper( key, f )    ;
            std::pair<bool, bool> ret = base_class::ensure_at( refHead, *pNode, cds::ref(wrapper)) ;
#   endif
            if ( ret.first && ret.second )
                pNode.release() ;

            return ret  ;
        }

        template <typename Q>
        bool find_at( head_type& refHead, Q const& key )
        {
            return base_class::find_at( refHead, key ) ;
        }

        template <typename Q, typename Func>
        bool find_at( head_type& refHead, Q& val, Func f )
        {
#   ifdef CDS_CXX11_LAMBDA_SUPPORT
#       ifdef CDS_BUG_STATIC_MEMBER_IN_LAMBDA
            // GCC 4.5-4.7: node_to_value is unaccessible from lambda,
            // like as MichaelList::node_to_value that requires to capture *this* despite on node_to_value is static function
            value_type& (* n2v)( node_type& ) = node_to_value ;
            return base_class::find_at( refHead, val, [&f, n2v](node_type& node, Q& v){ cds::unref(f)( n2v(node), v ); })  ;
#       else
            return base_class::find_at( refHead, val, [&f](node_type& node, Q& v){ cds::unref(f)( node_to_value(node), v ); })  ;
#       endif
#   else
            find_functor<Func>  wrapper( f ) ;
            return base_class::find_at( refHead, val, cds::ref(wrapper) )  ;
#   endif
        }

        //@endcond
    };

}}  // namespace cds::container

#endif  // #ifndef __CDS_CONTAINER_MICHAEL_LIST_IMPL_H

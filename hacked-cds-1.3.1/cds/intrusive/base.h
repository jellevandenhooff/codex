/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_INTRUSIVE_BASE_H
#define __CDS_INTRUSIVE_BASE_H

#include <cds/opt/options.h>
#include <cds/details/allocator.h>
#include <cds/backoff_strategy.h>

namespace cds {

/// Intrusive containers
/**
    @ingroup cds_intrusive_containers
    The namespace cds::intrusive contains intrusive lock-free containers.
    The idea comes from \p boost::intrusive library, see http://boost.org/doc/ as a good introduction to intrusive approach.
    The intrusive containers of libcds library is developed as close to boost::intrusive

    In terms of lock-free approach, the main advantage of intrusive containers is
    that no memory allocation is performed to maintain container items.
    However, additional requirements is imposed for types and values that can be stored in intrusive container.
    See the container documentation for details.

    Restriction for Gidenstam's garbage collector cds::gc::HRC:
    the Gidenstam's garbage collector makes additional requirements to type of item in intrusive container.
    Therefore, for this GC only \p base_hook is allowed as the value of opt::hook option.

    \anchor cds_intrusive_item_destroying
    \par Destroying items

    It should be very careful when destroying an item removed from intrusive container.
    In other threads the references to popped item may exists some time after removing.
    To destroy the removed item in thread-safe manner you should call static function \p retire
    of garbage collector you use, for example:
    \code
    struct destroyer  {
        void operator ()( my_type * p )
        {
            delete p    ;
        }
    };

    typedef cds::intrusive::TreiberStack< cds::gc::HP, my_type, cds::opt::disposer< destroyer > > stack ;
    stack s ;

    // ....

    my_type * p = s.pop() ;

    if ( p ) {
        // It is wrong
        // delete p ;

        // It is correct
        cds::gc:HP::retire< destroyer >( p ) ;
    }
    \endcode
    The situation becomes even more complicated when you want store items in different intrusive containers.
    In this case the best way is using reference counting:
    \code
    struct my_type {
        ...
        std::atomic<unsigned int> nRefCount ;

        my_type()
            : nRefCount(0)
        {}
    };

    struct destroyer  {
        void operator ()( my_type * p )
        {
            if ( --p->nRefCount == 0 )
                delete p    ;   // delete only after no reference pointing to p
        }
    };

    typedef cds::intrusive::TreiberStack< cds::gc::HP, my_type, cds::opt::disposer< destroyer > > stack ;
    typedef cds::intrusive::MSQueue< cds::gc::HP, my_type, cds::opt::disposer< destroyer > > queue ;
    stack s ;
    queue q ;

    my_type * v = new my_type() ;

    v.nRefCount++   ; // increment counter before pushing the item to the stack
    s.push(v)   ;

    v.nRefCount++   ; // increment counter before pushing the item to the queue
    q.push(v)   ;

    // ....

    my_type * ps = s.pop() ;
    if ( ps ) {
        // It is wrong
        // delete ps ;

        // It is correct
        cds::gc:HP::retire< destroyer >( ps ) ;
    }

    my_type * pq = q.pop() ;
    if ( pq ) {
        // It is wrong
        // delete pq ;

        // It is correct
        cds::gc:HP::retire< destroyer >( pq ) ;
    }
    \endcode
    Violation of these rules may lead to a crash.

    \par Intrusive containers and Hazard Pointer-like garbage collectors

    If you develop your intrusive container based on <b>libcds</b> library framework, you should
    take in the account the following.
    The main idea of garbage collectors (GC) based on Hazard Pointer schema is protecting a shared pointer
    by publishing it as a "hazard" one i.e. as a pointer that is changing at the current time and cannot be
    deleted at this moment. In intrusive container paradigm, the pointer to the node of the container
    and the pointer to the item stored in the container are not equal in the general case.
    However, any pointer to the node should be cast to the appropriate pointer to the container's item.
    In general, any item can be placed to some different intrusive containers simultaneously,
    and each of those container holds a unique pointer to its node that refers to the same item.
    When we protect a pointer, we want to protect an <b>item</b> pointer that is the invariant
    for any container stored that item. Conclusion: instead of protecting by GC's guard a pointer to an node
    you should convert it to the pointer to the item and then protect resulting item pointer.
    Otherwise an unpredictable result may occurs.

*/
namespace intrusive {

    /// @defgroup cds_intrusive_containers Intrusive containers
    /** @defgroup cds_intrusive_helper Helper structs for intrusive containers
        @ingroup cds_intrusive_containers
    */
    /** @defgroup cds_intrusive_stack Stacks
        @ingroup cds_intrusive_containers
    */
    /** @defgroup cds_intrusive_queue Queues
        @ingroup cds_intrusive_containers
    */
    /** @defgroup cds_intrusive_deque Deque
        @ingroup cds_intrusive_containers
    */
    /** @defgroup cds_intrusive_map Sets
        @ingroup cds_intrusive_containers
    */
    /** @defgroup cds_intrusive_list Lists
        @ingroup cds_intrusive_containers
    */

    /// Common options for intrusive containers
    /** @ingroup cds_intrusive_helper
        This namespace contains options for intrusive containers.
        It imports all definitions from cds::opt namespace and introduces a lot
        of options specific for intrusive approach.
    */
    namespace opt {
        using namespace cds::opt    ;

        //@cond
        struct base_hook_tag    ;
        struct member_hook_tag  ;
        struct traits_hook_tag  ;
        //@endcond

        /// Hook option
        /**
            Hook is a class that a user must add as a base class or as a member to make the user class compatible with intrusive containers.
            \p Hook template parameter strongly depends on the type of intrusive container you use.
        */
        template <typename Hook>
        struct hook {
            //@cond
            template <typename Base> struct pack: public Base
            {
                typedef Hook hook ;
            };
            //@endcond
        };

        /// Item disposer option setter
        /**
            The option specifies a functor that is used for dispose removed items.
            The interface of \p Type functor is:
            \code
            struct myDisposer {
                void operator ()( T * val ) ;
            };
            \endcode

            Predefined types for \p Type:
            - opt::v::empty_disposer - the disposer that does nothing
            - opt::v::delete_disposer - the disposer that calls operator \p delete

            Usually, the disposer should be stateless default-constructible functor.
            It is called by garbage collector in deferred mode.
        */
        template <typename Type>
        struct disposer {
            //@cond
            template <typename Base> struct pack: public Base
            {
                typedef Type disposer   ;
            };
            //@endcond
        };

        /// Values of \ref cds::intrusive::opt::link_checker option
        enum link_check_type {
            never_check_link,    ///< no link checking performed
            debug_check_link,    ///< check only in debug build
            always_check_link    ///< check in debug and release build
        };

        /// Link checking
        /**
            The option specifies a type of link checking.
            Possible values for \p Value are is one of \ref link_check_type enum:
            - \ref never_check_link - no link checking performed
            - \ref debug_check_link - check only in debug build
            - \ref always_check_link - check in debug and release build (not yet implemented for release mode).

            When link checking is on, the container tests that the node's link fields
            must be NULL before inserting the item. If the link is not NULL an assertion is generated
        */
        template <link_check_type Value>
        struct link_checker {
            //@cond
            template <typename Base> struct pack: public Base
            {
                static const link_check_type link_checker = Value ;
            };
            //@endcond
        };

        /// Predefined option values
        namespace v {
            using namespace cds::opt::v ;

            //@cond
            /// No link checking
            template <typename Node>
            struct empty_link_checker
            {
                //@cond
                typedef Node node_type  ;

                static void is_empty( const node_type * pNode )
                {}
                //@endcond
            };
            //@endcond

            /// Empty item disposer
            /**
                The disposer does nothing.
                This is one of possible values of opt::disposer option.
            */
            struct empty_disposer
            {
                /// Empty dispose functor
                template <typename T>
                void operator ()( T * )
                {}
            };

            /// Deletion item disposer
            /**
                Analogue of operator \p delete call.
                The disposer that calls \p T destructor and deallocates the item via \p Alloc allocator.
            */
            template <typename Alloc = CDS_DEFAULT_ALLOCATOR >
            struct delete_disposer
            {
                /// Dispose functor
                template <typename T>
                void operator ()( T * p )
                {
                    cds::details::Allocator<T, Alloc> alloc ;
                    alloc.Delete( p )   ;
                }
            };
        }   // namespace v

        //@cond
        // Lazy-list specific option (for split-list support)
        template <typename Type>
        struct boundary_node_type {
            //@cond
            template <typename Base> struct pack: public Base
            {
                typedef Type boundary_node_type   ;
            };
            //@endcond
        };
        //@endcond
    } // namespace opt

#ifdef CDS_DOXYGEN_INVOKED
    /// Container's node traits
    /** @ingroup cds_intrusive_helper
        This traits is intended for converting between type \p T of value stored in the intrusive container
        and container's node type \p NodeType.

        There are separate specializations for each \p Hook type.
    */
    template <typename T, typename NodeType, typename Hook>
    struct node_traits
    {
        typedef T        value_type ;  ///< Value type
        typedef NodeType node_type  ;  ///< Node type

        /// Convert value reference to node pointer
        static node_type * to_node_ptr( value_type& v ) ;

        /// Convert value pointer to node pointer
        static node_type * to_node_ptr( value_type * v ) ;

        /// Convert value reference to node pointer (const version)
        static const node_type * to_node_ptr( value_type const& v ) ;

        /// Convert value pointer to node pointer (const version)
        static const node_type * to_node_ptr( value_type const * v ) ;

        /// Convert node refernce to value pointer
        static value_type * to_value_ptr( node_type&  n ) ;

        /// Convert node pointer to value pointer
        static value_type * to_value_ptr( node_type *  n ) ;

        /// Convert node reference to value pointer (const version)
        static const value_type * to_value_ptr( node_type const & n );

        /// Convert node pointer to value pointer (const version)
        static const value_type * to_value_ptr( node_type const * n );
    };

#else
    template <typename T, typename NodeType, class Hook, typename HookType>
    struct node_traits  ;
#endif

    //@cond
    template <typename T, typename NodeType, class Hook>
    struct node_traits<T, NodeType, Hook, opt::base_hook_tag>
    {
        typedef T        value_type ;
        typedef NodeType node_type  ;

        static node_type * to_node_ptr( value_type& v )
        {
            return static_cast<node_type *>( &v )    ;
        }
        static node_type * to_node_ptr( value_type * v )
        {
            return v ? static_cast<node_type *>( v ) : reinterpret_cast<node_type *>( NULL )  ;
        }
        static const node_type * to_node_ptr( const value_type& v )
        {
            return static_cast<const node_type *>( &v )    ;
        }
        static const node_type * to_node_ptr( const value_type * v )
        {
            return v ? static_cast<const node_type *>( v ) : reinterpret_cast<const node_type *>( NULL )  ;
        }
        static value_type * to_value_ptr( node_type&  n )
        {
            return static_cast<value_type *>( &n )   ;
        }
        static value_type * to_value_ptr( node_type *  n )
        {
            return n ? static_cast<value_type *>( n ) : reinterpret_cast<value_type *>( NULL )  ;
        }
        static const value_type * to_value_ptr( const node_type& n )
        {
            return static_cast<const value_type *>( &n )   ;
        }
        static const value_type * to_value_ptr( const node_type * n )
        {
            return n ? static_cast<const value_type *>( n ) : reinterpret_cast<const value_type *>( NULL )  ;
        }
    };

    template <typename T, typename NodeType, class Hook>
    struct node_traits<T, NodeType, Hook, opt::member_hook_tag>
    {
        typedef T        value_type ;
        typedef NodeType node_type  ;

        static node_type * to_node_ptr( value_type& v )
        {
            return reinterpret_cast<node_type *>( reinterpret_cast<char *>(&v) + Hook::c_nMemberOffset )    ;
        }
        static node_type * to_node_ptr( value_type * v )
        {
            return v ? to_node_ptr(*v) : reinterpret_cast<node_type *>( NULL )    ;
        }
        static const node_type * to_node_ptr( const value_type& v )
        {
            return reinterpret_cast<const node_type *>( reinterpret_cast<const char *>(&v) + Hook::c_nMemberOffset )    ;
        }
        static const node_type * to_node_ptr( const value_type * v )
        {
            return v ? to_node_ptr(*v) : reinterpret_cast<const node_type *>( NULL )    ;
        }
        static value_type * to_value_ptr( node_type& n )
        {
            return reinterpret_cast<value_type *>( reinterpret_cast<char *>(&n) - Hook::c_nMemberOffset )    ;
        }
        static value_type * to_value_ptr( node_type * n )
        {
            return n ? to_value_ptr(*n) : reinterpret_cast<value_type *>( NULL )    ;
        }
        static const value_type * to_value_ptr( const node_type& n )
        {
            return reinterpret_cast<const value_type *>( reinterpret_cast<const char *>(&n) - Hook::c_nMemberOffset )    ;
        }
        static const value_type * to_value_ptr( const node_type * n )
        {
            return n ? to_value_ptr(*n) : reinterpret_cast<const value_type *>( NULL )    ;
        }
    };

    template <typename T, typename NodeType, class Hook>
    struct node_traits<T, NodeType, Hook, opt::traits_hook_tag>: public Hook::node_traits
    {} ;
    //@endcond

    /// Node traits selector metafunction
    /** @ingroup cds_intrusive_helper
        The metafunction selects appropriate \ref node_traits specialization based on value type \p T, node type \p NodeType, and hook type \p Hook.
    */
    template <typename T, typename NodeType, class Hook>
    struct get_node_traits
    {
        //@cond
        typedef node_traits<T, NodeType, Hook, typename Hook::hook_type> type  ;
        //@endcond
    };

    //@cond
    /// Functor converting container's node type to value type
    template <class Container>
    struct node_to_value {
        typename Container::value_type * operator()( typename Container::node_type * p )
        {
            typedef typename Container::node_traits node_traits ;
            return node_traits::to_value_ptr( p )  ;
        }
    };
    //@endcond

}} // namespace cds::intrusuve

#endif  // #ifndef __CDS_INTRUSIVE_BASE_H

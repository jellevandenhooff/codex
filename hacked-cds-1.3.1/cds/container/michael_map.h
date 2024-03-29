/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_CONTAINER_MICHAEL_MAP_H
#define __CDS_CONTAINER_MICHAEL_MAP_H

#include <cds/container/michael_map_base.h>
#include <cds/details/allocator.h>

namespace cds { namespace container {

    /// Michael's hash map
    /** @ingroup cds_nonintrusive_map
        \anchor cds_nonintrusive_MichaelHashMap_hp

        Source:
            - [2002] Maged Michael "High performance dynamic lock-free hash tables and list-based sets"

        Michael's hash table algorithm is based on lock-free ordered list and it is very simple.
        The main structure is an array \p T of size \p M. Each element in \p T is basically a pointer
        to a hash bucket, implemented as a singly linked list. The array of buckets cannot be dynamically expanded.
        However, each bucket may contain unbounded number of items.

        Template parameters are:
        - \p GC - Garbage collector used. Note the \p GC must be the same as the GC used for \p OrderedList
        - \p OrderedList - ordered key-value list implementation used as bucket for hash set, for example, MichaelKVList.
            The ordered list implementation specifies the \p Key and \p Value types stored in the hash-map, the reclamation
            schema \p GC used by hash-map, the comparison functor for the type \p Key and other features specific for
            the ordered list.
        - \p Traits - type traits. See michael_map::type_traits for explanation.

        Instead of defining \p Traits struct you may use option-based syntax with \p michael_map::make_traits metafunction
        (this metafunction is a synonym for michael_set::make_traits).
        For \p michael_map::make_traits the following option may be used:
        - opt::hash - mandatory option, specifies hash functor.
        - opt::item_counter - optional, specifies item counting policy. See michael_map::type_traits for explanation.
        - opt::allocator - optional, bucket table allocator. Default is \ref CDS_DEFAULT_ALLOCATOR.

        Many of the class function take a key argument of type \p K that in general is not \ref key_type.
        \p key_type and an argument of template type \p K must meet the following requirements:
        - \p key_type should be constructible from value of type \p K;
        - the hash functor should be able to calculate correct hash value from argument \p key of type \p K:
            <tt> hash( key_type(key) ) == hash( key ) </tt>
        - values of type \p key_type and \p K should be comparable

        There is a specialization for \ref cds::gc::nogc declared in <tt>cds/container/michael_map_nogc.h</tt>,
        see \ref cds_nonintrusive_MichaelHashMap_nogc "MichaelHashMap<gc::nogc>".

        <b>How to use</b>
        Suppose, you want to make \p int to \p int map for Hazard Pointer garbage collector. You should
        choose suitable ordered list class that will be used as a bucket for the map; it may be MichaelKVList.
        \code
        #include <cds/container/michael_kvlist_hp.h>    // MichaelKVList for gc::HP
        #include <cds/container/michael_map.h>          // MIchaelHashMap

        // List traits based on std::less predicate
        struct list_traits: public cds::container::michael_list::type_traits
        {
            typedef std::less<int>      less ;
        };

        // Ordered list
        typedef cds::container::MichaelKVList< cds::gc::HP, int, int, list_traits> int2int_list  ;

        // Map traits
        struct map_traits: public cds::container::michael_map::type_traits
        {
            struct hash {
                size_t operator()( int i ) const
                {
                    return cds::opt::v::hash<int>()( i ) ;
                }
            }
        };

        // Your map
        typedef cds::container::MichaelHashMap< cds::gc::HP, int2int_list, map_traits > int2int_map ;

        // Now you can use int2int_map class

        int main()
        {
            int2int_map theMap  ;

            theMap.insert( 100 )    ;
            ...
        }
        \endcode

        You may use option-based declaration:
        \code
        #include <cds/container/michael_kvlist_hp.h>    // MichaelKVList for gc::HP
        #include <cds/container/michael_map.h>          // MIchaelHashMap

        // Ordered list
        typedef cds::container::MichaelKVList< cds::gc::HP, int, int,
            typename cds::container::michael_list::make_traits<
                cds::container::opt::less< std::less<int> >     // item comparator option
            >::type
        >  int2int_list   ;

        // Map
        typedef cds::container::MichaelHashMap< cds::gc::HP, int2int_list,
            cds::container::michael_map::make_traits<
                cc::opt::hash< cds::opt::v::hash<int> >
            >
        > int2int_map ;
        \endcode
    */
    template <
        class GC,
        class OrderedList,
#ifdef CDS_DOXYGEN_INVOKED
        class Traits = michael_map::type_traits
#else
        class Traits
#endif
    >
    class MichaelHashMap
    {
    public:
        typedef OrderedList bucket_type     ;   ///< type of ordered list used as a bucket implementation
        typedef Traits      options         ;   ///< Traits template parameters

        typedef typename bucket_type::key_type          key_type        ;   ///< key type
        typedef typename bucket_type::mapped_type       mapped_type     ;   ///< value type
        typedef typename bucket_type::value_type        value_type      ;   ///< key/value pair stored in the list

        typedef GC                                      gc              ;   ///< Garbage collector
        typedef typename bucket_type::key_comparator    key_comparator  ;   ///< key comparision functor

        /// Hash functor for \ref key_type and all its derivatives that you use
        typedef typename cds::opt::v::hash_selector< typename options::hash >::type   hash ;
        typedef typename options::item_counter          item_counter    ;   ///< Item counter type

        /// Bucket table allocator
        typedef cds::details::Allocator< bucket_type, typename options::allocator >  bucket_table_allocator ;

    protected:
        item_counter    m_ItemCounter   ;   ///< Item counter
        hash            m_HashFunctor   ;   ///< Hash functor

        bucket_type *   m_Buckets       ;   ///< bucket table

    private:
        //@cond
        const size_t    m_nHashBitmask ;
        //@endcond

    protected:
        /// Calculates hash value of \p key
        size_t hash_value( key_type const & key ) const
        {
            return m_HashFunctor( key ) & m_nHashBitmask   ;
        }

        /// Returns the bucket (ordered list) for \p key
        bucket_type&    bucket( key_type const& key )
        {
            return m_Buckets[ hash_value( key ) ]  ;
        }

    protected:
        /// Forward iterator
        /**
            \p IsConst - constness boolean flag

            The forward iterator for Michael's map is based on \p OrderedList forward iterator and has the following features:
            - it has no post-increment operator, only pre-increment
            - it iterates items in unordered fashion
            - The iterator cannot be moved across thread boundary since it may contain GC's guard that is thread-private GC data.
            - Iterator ensures thread-safety even if you delete the item that iterator points to. However, in case of concurrent
              deleting operations it is no guarantee that you iterate all item in the set.

            Therefore, the use of iterators in concurrent environment is not good idea. Use the iterator for the concurrent container
            for debug purpose only.
        */
        template <bool IsConst>
        class iterator_type: private cds::intrusive::michael_set::details::iterator< bucket_type, IsConst >
        {
            //@cond
            typedef cds::intrusive::michael_set::details::iterator< bucket_type, IsConst >  base_class  ;
            friend class MichaelHashMap ;
            //@endcond

        protected:
            //@cond
            //typedef typename base_class::bucket_type    bucket_type     ;
            typedef typename base_class::bucket_ptr     bucket_ptr      ;
            typedef typename base_class::list_iterator  list_iterator   ;

            //typedef typename bucket_type::key_type      key_type        ;
            //@endcond

        public:
            /// Value pointer type (const for const_iterator)
            typedef typename cds::details::make_const_type<typename MichaelHashMap::mapped_type, IsConst>::pointer   value_ptr;
            /// Value reference type (const for const_iterator)
            typedef typename cds::details::make_const_type<typename MichaelHashMap::mapped_type, IsConst>::reference value_ref;

            /// Key-value pair pointer type (const for const_iterator)
            typedef typename cds::details::make_const_type<typename MichaelHashMap::value_type, IsConst>::pointer   pair_ptr;
            /// Key-value pair reference type (const for const_iterator)
            typedef typename cds::details::make_const_type<typename MichaelHashMap::value_type, IsConst>::reference pair_ref;

        protected:
            //@cond
            iterator_type( list_iterator const& it, bucket_ptr pFirst, bucket_ptr pLast )
                : base_class( it, pFirst, pLast )
            {}
            //@endcond

        public:
            /// Default ctor
            iterator_type()
                : base_class()
            {}

            /// Copy ctor
            iterator_type( const iterator_type& src )
                : base_class( src )
            {}

            /// Dereference operator
            pair_ptr operator ->() const
            {
                assert( base_class::m_pCurBucket != null_ptr<bucket_ptr>() )  ;
                return base_class::m_itList.operator ->()   ;
            }

            /// Dereference operator
            pair_ref operator *() const
            {
                assert( base_class::m_pCurBucket != null_ptr<bucket_ptr>() )  ;
                return base_class::m_itList.operator *()    ;
            }

            /// Pre-increment
            iterator_type& operator ++()
            {
                base_class::operator++()    ;
                return *this;
            }

            /// Assignment operator
            iterator_type& operator = (const iterator_type& src)
            {
                base_class::operator =(src)   ;
                return *this    ;
            }

            /// Returns current bucket (debug function)
            bucket_ptr bucket() const
            {
                return base_class::bucket() ;
            }

            /// Equality operator
            template <bool C>
            bool operator ==(iterator_type<C> const& i )
            {
                return base_class::operator ==( i ) ;
            }
            /// Equality operator
            template <bool C>
            bool operator !=(iterator_type<C> const& i )
            {
                return !( *this == i )  ;
            }
        };

    public:
        /// Forward iterator
        typedef iterator_type< false >    iterator    ;

        /// Const forward iterator
        typedef iterator_type< true >     const_iterator    ;

        /// Returns a forward iterator addressing the first element in a set
        /**
            For empty set \code begin() == end() \endcode
        */
        iterator begin()
        {
            return iterator( m_Buckets[0].begin(), m_Buckets, m_Buckets + bucket_count() )    ;
        }

        /// Returns an iterator that addresses the location succeeding the last element in a set
        /**
            Do not use the value returned by <tt>end</tt> function to access any item.
            The returned value can be used only to control reaching the end of the set.
            For empty set \code begin() == end() \endcode
        */
        iterator end()
        {
            return iterator( m_Buckets[bucket_count() - 1].end(), m_Buckets + bucket_count() - 1, m_Buckets + bucket_count() )   ;
        }

        /// Returns a forward const iterator addressing the first element in a set
        const_iterator begin() const
        {
            return const_iterator( const_cast<bucket_type const&>(m_Buckets[0]).begin(), m_Buckets, m_Buckets + bucket_count() )    ;
        }

        /// Returns an const iterator that addresses the location succeeding the last element in a set
        const_iterator end() const
        {
            return const_iterator( const_cast<bucket_type const&>(m_Buckets[bucket_count() - 1]).end(), m_Buckets + bucket_count() - 1, m_Buckets + bucket_count() )   ;
        }

    public:
        /// Initialize the map
        /**
            The Michael's hash map is non-expandable container. You should point the average count of items \p nMaxItemCount
            when you create an object.
            \p nLoadFactor parameter defines average count of items per bucket and it should be small number between 1 and 10.
            Remember, since the bucket implementation is an ordered list, searching in the bucket is linear [<tt>O(nLoadFactor)</tt>].
            Note, that many popular STL hash map implementation uses load factor 1.

            The ctor defines hash table size as rounding <tt>nMacItemCount / nLoadFactor</tt> up to nearest power of two.
        */
        MichaelHashMap(
            size_t nMaxItemCount,   ///< estimation of max item count in the hash set
            size_t nLoadFactor      ///< load factor: estimation of max number of items in the bucket
        ) : m_nHashBitmask( michael_map::details::init_hash_bitmask( nMaxItemCount, nLoadFactor ))
        {
            // GC and OrderedList::gc must be the same
            static_assert(( std::is_same<gc, typename bucket_type::gc>::value ), "GC and OrderedList::gc must be the same")  ;

            // atomicity::empty_item_counter is not allowed as a item counter
            static_assert(( !std::is_same<item_counter, atomicity::empty_item_counter>::value ), "atomicity::empty_item_counter is not allowed as a item counter")  ;

            m_Buckets = bucket_table_allocator().NewArray( bucket_count() ) ;
        }

        /// Clear hash set and destroy it
        ~MichaelHashMap()
        {
            clear() ;
            bucket_table_allocator().Delete( m_Buckets, bucket_count() ) ;
        }

        /// Inserts new node with key and default value
        /**
            The function creates a node with \p key and default value, and then inserts the node created into the map.

            Preconditions:
            - The \ref key_type should be constructible from value of type \p K.
                In trivial case, \p K is equal to \ref key_type.
            - The \ref mapped_type should be default-constructible.

            Returns \p true if inserting successful, \p false otherwise.
        */
        template <typename K>
        bool insert( const K& key )
        {
            const bool bRet = bucket( key ).insert( key ) ;
            if ( bRet )
                ++m_ItemCounter ;
            return bRet ;
        }

        /// Inserts new node
        /**
            The function creates a node with copy of \p val value
            and then inserts the node created into the map.

            Preconditions:
            - The \ref key_type should be constructible from \p key of type \p K.
            - The \ref mapped_type should be constructible from \p val of type \p V.

            Returns \p true if \p val is inserted into the set, \p false otherwise.
        */
        template <typename K, typename V>
        bool insert( K const& key, V const& val )
        {
            const bool bRet = bucket( key ).insert( key, val ) ;
            if ( bRet )
                ++m_ItemCounter ;
            return bRet ;
        }

        /// Inserts new node and initialize it by a functor
        /**
            This function inserts new node with key \p key and if inserting is successful then it calls
            \p func functor with signature
            \code
                struct functor {
                    void operator()( value_type& item )  ;
                };
            \endcode

            The argument \p item of user-defined functor \p func is the reference
            to the map's item inserted:
                - <tt>item.first</tt> is a const reference to item's key that cannot be changed.
                - <tt>item.second</tt> is a reference to item's value that may be changed.

            User-defined functor \p func should guarantee that during changing item's value no any other changes
            could be made on this map's item by concurrent threads.
            The user-defined functor can be passed by reference using <tt>boost::ref</tt>
            and it is called only if inserting is successful.

            The key_type should be constructible from value of type \p K.

            The function allows to split creating of new item into two part:
            - create item from \p key;
            - insert new item into the map;
            - if inserting is successful, initialize the value of item by calling \p func functor

            This can be useful if complete initialization of object of \p mapped_type is heavyweight and
            it is preferable that the initialization should be completed only if inserting is successful.
        */
        template <typename K, typename Func>
        bool insert_key( const K& key, Func func )
        {
            const bool bRet = bucket( key ).insert_key( key, func ) ;
            if ( bRet )
                ++m_ItemCounter ;
            return bRet ;
        }


        /// Ensures that the \p key exists in the map
        /**
            The operation performs inserting or changing data with lock-free manner.

            If the \p key not found in the map, then the new item created from \p key
            is inserted into the map (note that in this case the \ref key_type should be
            constructible from type \p K).
            Otherwise, the functor \p func is called with item found.
            The functor \p Func may be a function with signature:
            \code
                void func( bool bNew, value_type& item ) ;
            \endcode
            or a functor:
            \code
                struct my_functor {
                    void operator()( bool bNew, value_type& item ) ;
                };
            \endcode

            with arguments:
            - \p bNew - \p true if the item has been inserted, \p false otherwise
            - \p item - item of the list

            The functor may change any fields of the \p item.second that is \ref mapped_type;
            however, \p func must guarantee that during changing no any other modifications
            could be made on this item by concurrent threads.

            You may pass \p func argument by reference using <tt>boost::ref</tt>.

            Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
            \p second is true if new item has been added or \p false if the item with \p key
            already is in the list.
        */
        template <typename K, typename Func>
        std::pair<bool, bool> ensure( K const& key, Func func )
        {
            std::pair<bool, bool> bRet = bucket( key ).ensure( key, func )    ;
            if ( bRet.first && bRet.second )
                ++m_ItemCounter ;
            return bRet ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        /// For key \p key inserts data of type \ref mapped_type constructed with <tt>std::forward<Args>(args)...</tt>
        /**
            \p key_type should be constructible from type \p K

            Returns \p true if inserting successful, \p false otherwise.

            This function is available only for compiler that supports
            variadic template and move semantics
        */
        template <typename K, typename... Args>
        bool emplace( K&& key, Args&&... args )
        {
            const bool bRet = bucket( key ).emplace( std::forward<K>(key), std::forward<Args>(args)... ) ;
            if ( bRet )
                ++m_ItemCounter ;
            return bRet ;
        }
#   endif

        /// Delete \p key from the map
        /**
            Return \p true if \p key is found and deleted, \p false otherwise
        */
        template <typename K>
        bool erase( const K& key )
        {
            const bool bRet = bucket( key ).erase( key )    ;
            if ( bRet )
                --m_ItemCounter ;
            return bRet ;
        }

        /// Get value of item with \p key by a functor and delete it
        /**
            The function searches an item with key \p key, calls \p f functor
            and deletes the item. If \p key is not found, the functor is not called.

            The functor \p Func interface:
            \code
            struct extractor {
                void operator()(value_type& item) { ... }
            };
            \endcode
            The functor may be passed by reference using <tt>boost:ref</tt>

            Return \p true if key is found and deleted, \p false otherwise

            See also: \ref erase
        */
        template <typename K, typename Func>
        bool erase( const K& key, Func f )
        {
            const bool bRet = bucket( key ).erase( key, f )    ;
            if ( bRet )
                --m_ItemCounter ;
            return bRet ;
        }


        /// Find the key \p key
        /**
            The function searches the item with key equal to \p key and calls the functor \p f for item found.
            The interface of \p Func functor is:
            \code
            struct functor {
                void operator()( value_type& item ) ;
            };
            \endcode
            where \p item is the item found.

            You may pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

            The functor may change \p item.second. Note that the functor is only guarantee
            that \p item cannot be disposed during functor is executing.
            The functor does not serialize simultaneous access to the map's \p item. If such access is
            possible you must provide your own synchronization schema on item level to exclude unsafe item modifications.

            The function returns \p true if \p key is found, \p false otherwise.
        */
        template <typename K, typename Func>
        bool find( K const& key, Func f )
        {
            return bucket( key ).find( key, f )  ;
        }

        /// Find the key \p key
        /**
            The function searches the item with key equal to \p key
            and returns \p true if it is found, and \p false otherwise.
        */
        template <typename K>
        bool find( K const & key )
        {
            return bucket( key ).find( key )  ;
        }

        /// Clears the set (non-atomic)
        /**
            The function erases all items from the map.

            The function is not atomic. It cleans up each bucket and then resets the item counter to zero.
            If there are a thread that performs insertion while \p clear is working the result is undefined in general case:
            <tt> empty() </tt> may return \p true but the map may contain item(s).
            Therefore, \p clear may be used only for debugging purposes.
        */
        void clear()
        {
            for ( size_t i = 0; i < bucket_count(); ++i )
                m_Buckets[i].clear()  ;
            m_ItemCounter.reset()   ;
        }

        /// Checks if the map is empty
        /**
            Emptiness is checked by item counting: if item count is zero then the map is empty.
            Thus, the correct item counting is an important part of the map implementation.
        */
        bool empty() const
        {
            return size() == 0  ;
        }

        /// Returns item count in the map
        size_t size() const
        {
            return m_ItemCounter    ;
        }

        /// Returns the size of hash table
        /**
            Since MichaelHashMap cannot dynamically extend the hash table size,
            the value returned is an constant depending on object initialization parameters;
            see MichaelHashMap::MichaelHashMap for explanation.
        */
        size_t bucket_count() const
        {
            return m_nHashBitmask + 1   ;
        }
    };
}}  // namespace cds::container

#endif // ifndef __CDS_CONTAINER_MICHAEL_MAP_H

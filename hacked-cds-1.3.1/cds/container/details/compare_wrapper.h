/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_CONTAINER_DETAILS_COMPARE_WRAPPER_H
#define __CDS_CONTAINER_DETAILS_COMPARE_WRAPPER_H

//@cond
namespace cds { namespace container { namespace details {

    // Wrapper for intrusive key comparator which maps node_type to value_type
    template <typename NodeType, typename Comparator, typename KeyAccessor >
    struct compare_wrapper
    {
        typedef NodeType            node_type           ;
        typedef Comparator          comparator_type     ;
        typedef KeyAccessor         key_accessor        ;

        int operator()( node_type const& n1, node_type const& n2 ) const
        {
            return comparator_type()( key_accessor()(n1), key_accessor()(n2) ) ;
        }

        template <typename Q>
        int operator()( node_type const& n, Q const& q ) const
        {
            return comparator_type()( key_accessor()(n), q ) ;
        }

        template <typename Q>
        int operator()( Q const& q, node_type const& n ) const
        {
            return comparator_type()( q, key_accessor()(n) ) ;
        }
    };
}}} // namespace cds::container::details
//@endcond

#endif // #ifndef __CDS_CONTAINER_DETAILS_COMPARE_WRAPPER_H

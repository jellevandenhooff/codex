/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_MEMORY_MICHAEL_ALLOCATOR_PROCHEAP_STAT_H
#define __CDS_MEMORY_MICHAEL_ALLOCATOR_PROCHEAP_STAT_H

#include <cds/cxx11_atomic.h>

namespace cds { namespace memory { namespace michael {

        /// processor heap statistics
    /**
        This class is implementation of \ref opt::procheap_stat option.
        The statistic counter implementation is based on atomic operations.

        Template parameters:
            - \p INC_FENCE - memory fence for increment operation (default is release semantics)
            - \p READ_FENCE - memory fence for reading of statistic values (default is acquire semantics)
    */
    class procheap_atomic_stat
    {
        //@cond
        CDS_ATOMIC::atomic<size_t>      nAllocFromActive    ;  ///< Event count of allocation from active superblock
        CDS_ATOMIC::atomic<size_t>      nAllocFromPartial   ;  ///< Event count of allocation from partial superblock
        CDS_ATOMIC::atomic<size_t>      nAllocFromNew       ;  ///< Event count of allocation from new superblock
        CDS_ATOMIC::atomic<size_t>      nFreeCount          ;  ///< \ref free function call count
        CDS_ATOMIC::atomic<size_t>      nBlockCount         ;  ///< Count of superblock allocated
        CDS_ATOMIC::atomic<size_t>      nBlockDeallocCount  ;  ///< Count of superblock deallocated
        CDS_ATOMIC::atomic<size_t>      nDescAllocCount     ;  ///< Count of superblock descriptors
        CDS_ATOMIC::atomic<size_t>      nDescFull           ;  ///< Count of full superblock
        CDS_ATOMIC::atomic<unsigned long long> nBytesAllocated     ;  ///< Count of allocated bytes
        CDS_ATOMIC::atomic<unsigned long long> nBytesDeallocated   ;  ///< Count of deallocated bytes

        CDS_ATOMIC::atomic<size_t>      nActiveDescCASFailureCount ;   ///< CAS failure counter for active block of \p alloc_from_active Heap function
        CDS_ATOMIC::atomic<size_t>      nActiveAnchorCASFailureCount;   ///< CAS failure counter for active block of \p alloc_from_active Heap function
        CDS_ATOMIC::atomic<size_t>      nPartialDescCASFailureCount ;   ///< CAS failure counter for partial block of \p alloc_from_partial Heap function
        CDS_ATOMIC::atomic<size_t>      nPartialAnchorCASFailureCount;   ///< CAS failure counter for partial block of \p alloc_from_partial Heap function

        //@endcond

    public:
        //@cond
        procheap_atomic_stat()
            : nAllocFromActive(0)
            , nAllocFromPartial(0)
            , nAllocFromNew(0)
            , nFreeCount(0)
            , nBlockCount(0)
            , nDescFull(0)
            , nBytesAllocated(0)
            , nBytesDeallocated(0)
            , nActiveDescCASFailureCount(0)
            , nActiveAnchorCASFailureCount(0)
            , nPartialDescCASFailureCount(0)
            , nPartialAnchorCASFailureCount(0)
        {}
        //@endcond

    public:
        /// Increment event counter of allocation from active superblock
        void incAllocFromActive()
        {
            nAllocFromActive.fetch_add( 1, CDS_ATOMIC::memory_order_relaxed ) ;
        }
        /// Increment event counter of allocation from active superblock by \p n
        void incAllocFromActive( size_t n )
        {
            nAllocFromActive.fetch_add( n, CDS_ATOMIC::memory_order_relaxed )   ;
        }

        /// Increment event counter of allocation from partial superblock
        void incAllocFromPartial()
        {
            nAllocFromPartial.fetch_add( 1, CDS_ATOMIC::memory_order_relaxed )  ;
        }
        /// Increment event counter of allocation from partial superblock by \p n
        void incAllocFromPartial( size_t n )
        {
            nAllocFromPartial.fetch_add( n, CDS_ATOMIC::memory_order_relaxed )  ;
        }

        /// Increment event count of allocation from new superblock
        void incAllocFromNew()
        {
            nAllocFromNew.fetch_add( 1, CDS_ATOMIC::memory_order_relaxed )  ;
        }
        /// Increment event count of allocation from new superblock by \p n
        void incAllocFromNew( size_t n )
        {
            nAllocFromNew.fetch_add( n, CDS_ATOMIC::memory_order_relaxed )  ;
        }

        /// Increment event counter of free calling
        void incFreeCount()
        {
            nFreeCount.fetch_add( 1, CDS_ATOMIC::memory_order_relaxed ) ;
        }
        /// Increment event counter of free calling by \p n
        void incFreeCount( size_t n )
        {
            nFreeCount.fetch_add( n, CDS_ATOMIC::memory_order_relaxed ) ;
        }

        /// Increment counter of superblock allocated
        void incBlockAllocated()
        {
            nBlockCount.fetch_add( 1, CDS_ATOMIC::memory_order_relaxed ) ;
        }
        /// Increment counter of superblock allocated by \p n
        void incBlockAllocated( size_t n )
        {
            nBlockCount.fetch_add( n, CDS_ATOMIC::memory_order_relaxed ) ;
        }

        /// Increment counter of superblock deallocated
        void incBlockDeallocated()
        {
            nBlockDeallocCount.fetch_add( 1, CDS_ATOMIC::memory_order_relaxed ) ;
        }
        /// Increment counter of superblock deallocated by \p n
        void incBlockDeallocated( size_t n )
        {
            nBlockDeallocCount.fetch_add( n, CDS_ATOMIC::memory_order_relaxed ) ;
        }

        /// Increment counter of superblock descriptor allocated
        void incDescAllocCount()
        {
            nDescAllocCount.fetch_add( 1, CDS_ATOMIC::memory_order_relaxed ) ;
        }
        /// Increment counter of superblock descriptor allocated by \p n
        void incDescAllocCount( size_t n )
        {
            nDescAllocCount.fetch_add( n, CDS_ATOMIC::memory_order_relaxed ) ;
        }

        /// Increment counter of full superblock descriptor
        void incDescFull()
        {
            nDescFull.fetch_add( 1, CDS_ATOMIC::memory_order_relaxed ) ;
        }
        /// Increment counter of full superblock descriptor by \p n
        void incDescFull( size_t n )
        {
            nDescFull.fetch_add( n, CDS_ATOMIC::memory_order_relaxed ) ;
        }

        /// Decrement counter of full superblock descriptor
        void decDescFull()
        {
            nDescFull.fetch_sub( 1, CDS_ATOMIC::memory_order_relaxed ) ;
        }
        /// Decrement counter of full superblock descriptor by \p n
        void decDescFull(size_t n)
        {
            nDescFull.fetch_sub( n, CDS_ATOMIC::memory_order_relaxed )  ;
        }
        /// Add \p nBytes to allocated bytes counter
        void incAllocatedBytes( size_t nBytes )
        {
            nBytesAllocated.fetch_add( nBytes, CDS_ATOMIC::memory_order_relaxed ) ;
        }
        /// Add \p nBytes to deallocated bytes counter
        void incDeallocatedBytes( size_t nBytes )
        {
            nBytesDeallocated.fetch_add( nBytes, CDS_ATOMIC::memory_order_relaxed) ;
        }

        /// Add \p nCount to CAS failure counter of updating \p active field of active descriptor for \p alloc_from_active internal Heap function
        void incActiveDescCASFailureCount( int nCount )
        {
            nActiveDescCASFailureCount.fetch_add( nCount, CDS_ATOMIC::memory_order_relaxed ) ;
        }

        /// Add \p nCount to CAS failure counter of updating \p anchor field of active descriptor for \p alloc_from_active internal Heap function
        void incActiveAnchorCASFailureCount( int nCount )
        {
            nActiveAnchorCASFailureCount.fetch_add( nCount, CDS_ATOMIC::memory_order_relaxed ) ;
        }

        /// Add \p nCount to CAS failure counter of updating \p active field of partial descriptor for \p alloc_from_partial internal Heap function
        void incPartialDescCASFailureCount( int nCount )
        {
            nPartialDescCASFailureCount.fetch_add( nCount, CDS_ATOMIC::memory_order_relaxed ) ;
        }

        /// Add \p nCount to CAS failure counter of updating \p anchor field of partial descriptor for \p alloc_from_partial internal Heap function
        void incPartialAnchorCASFailureCount( int nCount )
        {
            nPartialAnchorCASFailureCount.fetch_add( nCount, CDS_ATOMIC::memory_order_relaxed ) ;
        }

        // -----------------------------------------------------------------
        // Reading

        /// Read event counter of allocation from active superblock
        size_t allocFromActive() const
        {
            return nAllocFromActive.load(CDS_ATOMIC::memory_order_relaxed)   ;
        }

        /// Read event counter of allocation from partial superblock
        size_t allocFromPartial() const
        {
            return nAllocFromPartial.load(CDS_ATOMIC::memory_order_relaxed)   ;
        }

        /// Read event count of allocation from new superblock
        size_t allocFromNew() const
        {
            return nAllocFromNew.load(CDS_ATOMIC::memory_order_relaxed)  ;
        }

        /// Read event counter of free calling
        size_t freeCount() const
        {
            return nFreeCount.load(CDS_ATOMIC::memory_order_relaxed) ;
        }

        /// Read counter of superblock allocated
        size_t blockAllocated() const
        {
            return nBlockCount.load(CDS_ATOMIC::memory_order_relaxed)    ;
        }

        /// Read counter of superblock deallocated
        size_t blockDeallocated() const
        {
            return nBlockDeallocCount.load(CDS_ATOMIC::memory_order_relaxed)    ;
        }

        /// Read counter of superblock descriptor allocated
        size_t descAllocCount() const
        {
            return nDescAllocCount.load(CDS_ATOMIC::memory_order_relaxed)    ;
        }

        /// Read counter of full superblock descriptor
        size_t descFull() const
        {
            return nDescFull.load(CDS_ATOMIC::memory_order_relaxed)  ;
        }

        /// Get counter of allocated bytes
        /**
            This counter only counts the bytes allocated by Heap, OS allocation (large blocks) is not counted.

            To get count of bytes allocated but not yet deallocated you should call
            \code allocatedBytes() - deallocatedBytes() \endcode
        */
        atomic64u_t allocatedBytes() const
        {
            return nBytesAllocated.load(CDS_ATOMIC::memory_order_relaxed)   ;
        }

        /// Get counter of deallocated bytes
        /**
            This counter only counts the bytes allocated by Heap, OS allocation (large blocks) is not counted.unter of deallocated bytes

            See \ref allocatedBytes notes
        */
        atomic64u_t deallocatedBytes() const
        {
            return nBytesDeallocated.load(CDS_ATOMIC::memory_order_relaxed) ;
        }

        /// Get CAS failure counter of updating \p active field of active descriptor for \p alloc_from_active internal Heap function
        size_t activeDescCASFailureCount() const
        {
            return nActiveDescCASFailureCount.load(CDS_ATOMIC::memory_order_relaxed) ;
        }

        /// Get CAS failure counter of updating \p anchor field of active descriptor for \p alloc_from_active internal Heap function
        size_t activeAnchorCASFailureCount() const
        {
            return nActiveAnchorCASFailureCount.load(CDS_ATOMIC::memory_order_relaxed) ;
        }

        /// Get CAS failure counter of updating \p active field of partial descriptor for \p alloc_from_active internal Heap function
        size_t partialDescCASFailureCount() const
        {
            return nPartialDescCASFailureCount.load(CDS_ATOMIC::memory_order_relaxed) ;
        }

        /// Get CAS failure counter of updating \p anchor field of partial descriptor for \p alloc_from_active internal Heap function
        size_t partialAnchorCASFailureCount() const
        {
            return nPartialAnchorCASFailureCount.load(CDS_ATOMIC::memory_order_relaxed) ;
        }
    };

    /// Empty processor heap statistics
    /**
        This class is dummy implementation of \ref opt::procheap_stat option.
        No statistic gathering is performed.

        Interface - see procheap_atomic_stat.
        All getter methods return 0.
    */
    class procheap_empty_stat
    {
    //@cond
    public:
        void incAllocFromActive()
        {}
        void incAllocFromPartial()
        {}
        void incAllocFromNew()
        {}
        void incFreeCount()
        {}
        void incBlockAllocated()
        {}
        void incBlockDeallocated()
        {}
        void incDescAllocCount()
        {}
        void incDescFull()
        {}
        void decDescFull()
        {}

        // Add -------------------------------------------------------------
        void incAllocFromActive(size_t)
        {}
        void incAllocFromPartial(size_t)
        {}
        void incAllocFromNew(size_t)
        {}
        void incFreeCount(size_t)
        {}
        void incBlockAllocated(size_t)
        {}
        void incBlockDeallocated(size_t)
        {}
        void incDescAllocCount(size_t)
        {}
        void incDescFull(size_t)
        {}
        void decDescFull(size_t)
        {}
        void incAllocatedBytes( size_t /*nBytes*/ )
        {}
        void incDeallocatedBytes( size_t /*nBytes*/ )
        {}
        void incActiveDescCASFailureCount( int /*nCount*/ )
        {}
        void incActiveAnchorCASFailureCount( int /*nCount*/ )
        {}
        void incPartialDescCASFailureCount( int /*nCount*/ )
        {}
        void incPartialAnchorCASFailureCount( int /*nCount*/ )
        {}

        // -----------------------------------------------------------------
        // Reading

        size_t allocFromActive() const
        { return 0; }
        size_t allocFromPartial() const
        { return 0; }
        size_t allocFromNew() const
        { return 0; }
        size_t freeCount() const
        { return 0; }
        size_t blockAllocated() const
        { return 0; }
        size_t blockDeallocated() const
        { return 0; }
        size_t descAllocCount() const
        { return 0; }
        size_t descFull() const
        { return 0; }
        atomic64u_t allocatedBytes() const
        { return 0; }
        atomic64u_t deallocatedBytes() const
        { return 0; }
        size_t activeDescCASFailureCount() const
        { return 0; }
        size_t activeAnchorCASFailureCount() const
        { return 0; }
        size_t partialDescCASFailureCount() const
        { return 0; }
        size_t partialAnchorCASFailureCount() const
        { return 0; }

    //@endcond
    };


}}} // namespace cds::memory::michael

#endif  /// __CDS_MEMORY_MICHAEL_ALLOCATOR_PROCHEAP_STAT_H

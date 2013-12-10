/*
    This file is a part of libcds - Concurrent Data Structures library
    See http://libcds.sourceforge.net/

    (C) Copyright Maxim Khiszinsky [khizmax at gmail dot com] 2006-2013
    Distributed under the BSD license (see accompanying file license.txt)

    Version 1.3.1
*/


#ifndef __CDS_CXX11_ATOMIC_H
#define __CDS_CXX11_ATOMIC_H

#include <cds/details/defs.h>

namespace cds {
/// @defgroup cds_atomic_impl Atomic implementation

/// C++11 Atomic library support
/** @ingroup cds_atomic_impl
    <b>libcds</b> has an implementation of C++11 atomic library (header <tt><cds/cxx11_atomic.h></tt>)
    specified in <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3242.pdf">N3242, p.29</a>.

    This implementation has full support
    - <tt>atomic<T></tt> class and its specializations for integral types and pointers
    - <tt>atomic_flag</tt> class
    - free <tt>atomic_xxx</tt> functions

    Exclusions: the following features specified in C++11 standard are not implemented:
    - Atomic emulation. The library implements only genuine atomic operations for supported processors
    - Static initialization macros (like \p ATOMIC_FLAG_INIT and others)
    - \p atomic_init functions

    The library supports the following processor architectures:
    - Intel and AMD x86 (32bit) and amd64 (64bit)
    - Intel Itanium IA64 (64bit)
    - UltraSparc (64bit)

    Internal atomic implementation is used when the standard library provided by compiler
    has no <tt>\<atomic\></tt> header or it is not standard compliant.
    The library defines \p CDS_ATOMIC macro that specifies atomic library namespace:
    - \p std for compiler-provided <tt>\<atomic\></tt> library
    - \p boost if you use <tt>boost.atomic</tt> library (see note below)
    - \p cds::cxx11_atomic if internal atomic implementation used

    Using \p CDS_ATOMIC macro you may call <tt>\<atomic\></tt> library functions and classes,
    for example:
    \code
    CDS_ATOMIC::atomic<int> atomInt ;
    CDS_ATOMIC::atomic_store_explicit( &atomInt, 0, CDS_ATOMIC::memory_order_release ) ;
    \endcode

    \par Microsoft Visual C++

    MS Visual C++ has native <tt>\<atomic\></tt> header beginning from Visual C++ 11 RC.
    The \p libcds library defines \p CDS_ATOMIC as
    - \p cds::cxx11_atomic (internal implementation) for MS VC++ 2008 and 2010
    - \p std for MS VC++ 11 and above.

    \par GCC

    GCC team implements full support for C++ memory model in version 4.7
    (see <a href="http://gcc.gnu.org/wiki/Atomic/GCCMM">http://gcc.gnu.org/wiki/Atomic/GCCMM</a>).
    \p libcds uses its own implementation of C++11 <tt>\<atomic\></tt> library located in
    file <tt><cds/cxx11_atomic.h></tt> for GCC version up to 4.6. This implementation almost conforms to C++11 standard draft
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3242.pdf">N3242</a> (see exclusions above)
    that is closest to final version.
    However, GCC 4.6 has the implementation of <tt>\<atomic\></tt> header in its <tt>libstdc++</tt>
    that is built on <tt>__sync_xxx</tt> (or <tt>__atomic_xxx</tt>) built-in functions. You can use <b>libcds</b> with GCC 4.6
    <tt>\<atomic\></tt> specifying \p CDS_CXX11_ATOMIC_GCC macro in g++ command line:
    \code g++ -DCDS_CXX11_ATOMIC_GCC ... \endcode
    GCC 4.6 atomic implementation does not support <tt>atomic<T></tt> for <b>any</b> \p T. The linker
    generates "undefined symbol" error for <tt>atomic<T></tt> if \p T is not an integral type or a pointer. It is
    not essential for intrusive and non-intrusive containers represented in \p libcds.
    However, cds::memory::michael memory allocator cannot be linked with GCC 4.6 <tt>\<atomic\></tt> header.
    This error has been fixed in GCC 4.7.

    Thus, for GCC compiler the macro \p CDS_ATOMIC is defined as:
    - \p cds::cxx11_atomic by default
    - \p std if the compiler version is 4.6 and \p CDS_CXX11_ATOMIC_GCC is defined
    - \p std for GCC 4.7  and above

    \par Clang

    The macro \p CDS_ATOMIC is defined as \p cds::cxx11_atomic

    \par boost::atomic

    <a href="http://boost.org">boost</a> library community is working to implement
    atomic library in <tt>boost.atomic</tt>. This library has been accepted but not yet officially released.
    The <b>libcds</b> can use <tt>boost.atomic</tt> library if macro <tt>CDS_USE_BOOST_ATOMIC</tt>
    is defined. This feature is not tested since <tt>boost.atomic</tt> has a lot of typos
    in its <a href="http://git.chaoticmind.net/cgi-bin/cgit.cgi/boost.atomic/">git repository</a>.
*/
namespace cxx11_atomics {
}} // namespace cds::cxx11_atomics

//@cond

#if defined(CDS_USE_BOOST_ATOMIC)
#   include <boost/atomic.hpp>
#   define CDS_ATOMIC boost
#   define CDS_CXX11_ATOMIC_BEGIN_NAMESPACE namespace boost {
#   define CDS_CXX11_ATOMIC_END_NAMESPACE }
#elif CDS_CXX11_ATOMIC_SUPPORT == 1
    // Compiler supports C++11 atomic (conditionally defined in cds/details/defs.h)
#   include <cds/compiler/cxx11_atomic_prepatches.h>
#   include <atomic>
#   define CDS_ATOMIC std
#   define CDS_CXX11_ATOMIC_BEGIN_NAMESPACE namespace std {
#   define CDS_CXX11_ATOMIC_END_NAMESPACE }
#   include <cds/compiler/cxx11_atomic_patches.h>
#else
#   include <cds/compiler/cxx11_atomic.h>
#   define CDS_ATOMIC cds::cxx11_atomics
#   define CDS_CXX11_ATOMIC_BEGIN_NAMESPACE namespace cds { namespace cxx11_atomics {
#   define CDS_CXX11_ATOMIC_END_NAMESPACE }}
#endif
//@endcond

namespace cds {

    /// Atomic primitives
    /**
        This namespace contains useful primitives derived from <tt>std::atomic</tt>.
    */
    namespace atomicity {

        /// Atomic event counter.
        /**
            This class is based on <tt>std::atomic_size_t</tt>.
            It uses relaxed memory ordering \p memory_order_relaxed and may be used as a statistic counter.
        */
        class event_counter
        {
            //@cond
            CDS_ATOMIC::atomic_size_t   m_counter   ;
            //@endcond

        public:
            typedef size_t      value_type  ;       ///< Type of counter

        public:
            // Initializes event counter with zero
            event_counter() CDS_NOEXCEPT
                : m_counter(size_t(0))
            {}

            /// Assign operator
            /**
                Returns \p n.
            */
            value_type operator =(
                value_type n    //< new value of the counter
            ) CDS_NOEXCEPT
            {
                m_counter.exchange( n, CDS_ATOMIC::memory_order_relaxed ) ;
                return n;
            }

            /// Addition
            /**
                Returns new value of the atomic counter.
            */
            size_t operator +=(
                size_t n    ///< addendum
            ) CDS_NOEXCEPT
            {
                return m_counter.fetch_add( n, CDS_ATOMIC::memory_order_relaxed ) + n ;
            }

            /// Substraction
            /**
                Returns new value of the atomic counter.
            */
            size_t operator -=(
                size_t n    ///< subtrahend
            ) CDS_NOEXCEPT
            {
                return m_counter.fetch_sub( n, CDS_ATOMIC::memory_order_relaxed ) - n ;
            }

            /// Get current value of the counter
            operator size_t () const CDS_NOEXCEPT
            {
                return m_counter.load( CDS_ATOMIC::memory_order_relaxed )   ;
            }

            /// Preincrement
            size_t operator ++() CDS_NOEXCEPT
            {
                return m_counter.fetch_add( 1, CDS_ATOMIC::memory_order_relaxed ) + 1 ;
            }
            /// Postincrement
            size_t operator ++(int) CDS_NOEXCEPT
            {
                return m_counter.fetch_add( 1, CDS_ATOMIC::memory_order_relaxed ) ;
            }

            /// Predecrement
            size_t operator --() CDS_NOEXCEPT
            {
                return m_counter.fetch_sub( 1, CDS_ATOMIC::memory_order_relaxed ) - 1 ;
            }
            /// Postdecrement
            size_t operator --(int) CDS_NOEXCEPT
            {
                return m_counter.fetch_sub( 1, CDS_ATOMIC::memory_order_relaxed ) ;
            }

            /// Get current value of the counter
            size_t get() const CDS_NOEXCEPT
            {
                return m_counter.load( CDS_ATOMIC::memory_order_relaxed ) ;
            }
        };

        /// Atomic item counter
        /**
            This class is simplified interface around <tt>std::atomic_size_t</tt>.
            The class supports getting of current value of the counter and increment/decrement its value.
        */
        class item_counter
        {
        public:
            typedef CDS_ATOMIC::atomic_size_t   atomic_type ;   ///< atomic type used
            typedef size_t counter_type    ;   ///< Integral item counter type (size_t)

        private:
            //@cond
            atomic_type                         m_Counter   ;   ///< Atomic item counter
            //@endcond

        public:
            /// Default ctor initializes the counter to zero.
            item_counter()
                : m_Counter(counter_type(0))
            {}

            /// Returns current value of the counter
            counter_type    value(CDS_ATOMIC::memory_order order = CDS_ATOMIC::memory_order_relaxed) const
            {
                return m_Counter.load( order ) ;
            }

            /// Same as \ref value() with relaxed memory ordering
            operator counter_type() const
            {
                return value()  ;
            }

            /// Returns underlying atomic interface
            atomic_type&  getAtomic()
            {
                return m_Counter    ;
            }

            /// Returns underlying atomic interface (const)
            const atomic_type&  getAtomic() const
            {
                return m_Counter    ;
            }

            /// Increments the counter. Semantics: postincrement
            counter_type inc(CDS_ATOMIC::memory_order order = CDS_ATOMIC::memory_order_relaxed )
            {
                return m_Counter.fetch_add( 1, order )   ;
            }

            /// Decrements the counter. Semantics: postdecrement
            counter_type dec(CDS_ATOMIC::memory_order order = CDS_ATOMIC::memory_order_relaxed)
            {
                return m_Counter.fetch_sub( 1, order ) ;
            }

            /// Preincrement
            counter_type operator ++()
            {
                return inc() + 1    ;
            }
            /// Postincrement
            counter_type operator ++(int)
            {
                return inc()    ;
            }

            /// Predecrement
            counter_type operator --()
            {
                return dec() - 1    ;
            }
            /// Postdecrement
            counter_type operator --(int)
            {
                return dec()    ;
            }

            /// Resets count to 0
            void reset(CDS_ATOMIC::memory_order order = CDS_ATOMIC::memory_order_relaxed)
            {
                m_Counter.store( 0, order ) ;
            }
        };

        /// Empty item counter
        /**
            This class may be used instead of \ref item_counter when you do not need full \ref item_counter interface.
            All methods of the class is empty and returns 0.

            The object of this class should not be used in data structure that behavior significantly depends on item counting
            (for example, in many hash map implementation).
        */
        class empty_item_counter {
        public:
            typedef size_t counter_type    ;  ///< Counter type
        public:
            /// Returns 0
            counter_type    value(CDS_ATOMIC::memory_order /*order*/ = CDS_ATOMIC::memory_order_relaxed) const
            {
                return 0    ;
            }

            /// Same as \ref value(), always returns 0.
            operator counter_type() const
            {
                return value()  ;
            }

            /// Dummy increment. Always returns 0
            size_t inc(CDS_ATOMIC::memory_order /*order*/ = CDS_ATOMIC::memory_order_relaxed)
            {
                return 0    ;
            }

            /// Dummy increment. Always returns 0
            size_t dec(CDS_ATOMIC::memory_order /*order*/ = CDS_ATOMIC::memory_order_relaxed)
            {
                return 0    ;
            }

            /// Dummy pre-increment. Always returns 0
            size_t operator ++()
            {
                return 0    ;
            }
            /// Dummy post-increment. Always returns 0
            size_t operator ++(int)
            {
                return 0    ;
            }

            /// Dummy pre-decrement. Always returns 0
            size_t operator --()
            {
                return 0    ;
            }
            /// Dummy post-decrement. Always returns 0
            size_t operator --(int)
            {
                return 0    ;
            }

            /// Dummy function
            void reset(CDS_ATOMIC::memory_order /*order*/ = CDS_ATOMIC::memory_order_relaxed)
            {}
        };


    }   // namespace atomicity

}   // namespace cds

#endif // #ifndef __CDS_CXX11_ATOMIC_H

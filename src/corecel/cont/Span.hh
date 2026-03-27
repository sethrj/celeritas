//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/cont/Span.hh
//---------------------------------------------------------------------------//
#pragma once

#include <cstddef>
#include <type_traits>

#include "corecel/Macros.hh"

#include "Array.hh"

#include "detail/SpanImpl.hh"

#if !CELER_DEVICE_COMPILE
#    include "corecel/io/StreamableContainer.hh"
#endif

namespace celeritas
{
//---------------------------------------------------------------------------//
//! Sentinel value for span of dynamic type
constexpr auto dynamic_extent = detail::dynamic_extent;

//---------------------------------------------------------------------------//
/*!
 * Non-owning device-compatible reference to a contiguous span of data.
 * \tparam T value type
 * \tparam Extent fixed size; defaults to dynamic.
 *
 * A \c Span , like \c std::string_view , provides access to externally managed
 * data.  In Celeritas, this class is typically used as a return result
 * from accessing a range of elements in a \c Collection.
 *
 * This implementation is a \em nonconforming backport of the C++20 \c
 * std::span. Improvements for standards compatibility are welcome as long as
 * they retain the same behavior in device code. Important differences from
 * the standard \c std::span include:
 * - Supports a special marker/tag type `LdgValue<T>` which causes element
 *   accessors and iterators to use value-semantics loads (optimized device
 *   loads) instead of references.
 * - Uses a restricted constructor for iterators: instead of two separate
 *   iterator/end types, it uses only one.
 * - Provides additional free helpers tailored to Celeritas: `make_span`
 *   overloads for `Array<T,N>`, C arrays, and generic containers, plus
 *   `to_array()` convenience and a host-only `operator<<` using
 *   `StreamableContainer`.
 * - All public methods are decorated with `CELER_CONSTEXPR_FUNCTION` for
 *   host/device compatibility.
 * - Some subview helpers use `CELER_EXPECT` to check for bounds validation in
 *   debug builds.
 *
 * \par Synopsis
 *
 * Construction:
 * - Default constructs to an empty span.
 * - Construct from a pointer and size: `Span(pointer, size)`.
 * - Construct from two contiguous random-access iterators: `Span(first,
 *   last)` (non-standard convenience).
 * - Construct from C arrays or `Array<T,N>` (fixed-size spans).
 * - Converting constructor from a compatible `Span<U,N>` (e.g., mutable to
 *   const element type) when extents are compatible.
 *
 * Data access:
 * - Element access: `operator[]`, `front()`, `back()`.
 * - Observers: `data()`, `size()`, `size_bytes()`, `empty()`.
 * - Iteration: `begin()`, `end()`.
 *
 * Subviews and utilities:
 * - `first<Count>()`, `first(count)`, `last<Count>()`, `last(count)`.
 * - `subspan<Offset,Count>()` and `subspan(offset,count)` for compile-time
 *   and runtime subviews.
 * - Deduction guides are provided for pointer+size, iterator pairs and
 *   C arrays; free functions `make_span(...)` and `to_array(...)` are
 *   provided for convenience.
 */
template<class T, std::size_t Extent = dynamic_extent>
class Span
{
    using SpanTraitsT = detail::SpanTraits<T>;

  public:
    //!@{
    //! \name Type aliases
    using element_type = typename SpanTraitsT::element_type;
    using value_type = std::remove_cv_t<element_type>;
    using size_type = std::size_t;
    using pointer = typename SpanTraitsT::pointer;
    using const_pointer = typename SpanTraitsT::const_pointer;
    using reference = typename SpanTraitsT::reference;
    using const_reference = typename SpanTraitsT::const_reference;
    using iterator = typename SpanTraitsT::iterator;
    using const_iterator = typename SpanTraitsT::const_iterator;
    //!@}

    //! Size (may be dynamic)
    static constexpr size_type extent = Extent;

  public:
    //// CONSTRUCTION ////

    //! Construct with default null pointer and size zero
    constexpr Span() = default;

    //! Construct from data and size
    CELER_CONSTEXPR_FUNCTION Span(pointer d, size_type s) : s_(d, s) {}

    //! Construct from two contiguous random-access iterators
    template<class Iter>
    CELER_CONSTEXPR_FUNCTION Span(Iter first, Iter last)
        : s_(&(*first), static_cast<size_type>(last - first))
    {
    }

    //! Construct from a C array
    template<std::size_t N>
    CELER_CONSTEXPR_FUNCTION Span(element_type (&arr)[N]) : s_(arr, N)
    {
    }

    //! Construct from a span whose pointer type is implicitly convertible to
    //! ours (e.g., mutable T to const T), with compatible extent.
    //! Note that the enable-if prevents LdgSpan->Span conversion.
    template<class U,
             std::size_t N,
             std::enable_if_t<detail::is_array_convertible_v<U, element_type>
                                  && detail::is_span_size_convertible(N, Extent),
                              bool>
             = true>
    CELER_CONSTEXPR_FUNCTION Span(Span<U, N> const& other)
        : s_(other.data(), other.size())
    {
    }

    CELER_DEFAULT_COPY_MOVE(Span);
    ~Span() = default;

    //// ACCESS ////

    //!@{
    //! \name Iterators
    CELER_CONSTEXPR_FUNCTION iterator begin() const { return s_.data; }
    CELER_CONSTEXPR_FUNCTION iterator end() const { return s_.data + s_.size; }
    //!@}

    //!@{
    //! \name Element access
    CELER_CONSTEXPR_FUNCTION reference operator[](size_type i) const
    {
        return s_.data[i];
    }
    CELER_CONSTEXPR_FUNCTION reference front() const { return s_.data[0]; }
    CELER_CONSTEXPR_FUNCTION reference back() const
    {
        return s_.data[s_.size - 1];
    }
    CELER_CONSTEXPR_FUNCTION pointer data() const
    {
        return static_cast<pointer>(s_.data);
    }
    //!@}

    //!@{
    //! \name Observers
    CELER_CONSTEXPR_FUNCTION bool empty() const { return s_.size == 0; }
    CELER_CONSTEXPR_FUNCTION size_type size() const { return s_.size; }
    CELER_CONSTEXPR_FUNCTION size_type size_bytes() const
    {
        return sizeof(element_type) * s_.size;
    }
    //!@}

    //!@{
    //! \name Subviews
    template<std::size_t Count>
    CELER_CONSTEXPR_FUNCTION Span<T, Count> first() const
    {
        CELER_EXPECT(Count == 0 || Count <= this->size());
        return {this->data(), Count};
    }
    CELER_CONSTEXPR_FUNCTION
    Span<T, dynamic_extent> first(size_type count) const
    {
        CELER_EXPECT(count <= this->size());
        return {this->data(), count};
    }

    template<std::size_t Offset, std::size_t Count = dynamic_extent>
    CELER_CONSTEXPR_FUNCTION
        Span<T, detail::subspan_extent(Extent, Offset, Count)>
        subspan() const
    {
        CELER_EXPECT((Count == dynamic_extent) || (Offset == 0 && Count == 0)
                     || (Offset + Count <= this->size()));
        return {this->data() + Offset,
                detail::subspan_size(this->size(), Offset, Count)};
    }
    CELER_CONSTEXPR_FUNCTION
    Span<T, dynamic_extent>
    subspan(size_type offset, size_type count = dynamic_extent) const
    {
        CELER_EXPECT(offset + count <= this->size());
        return {this->data() + offset,
                detail::subspan_size(this->size(), offset, count)};
    }

    template<std::size_t Count>
    CELER_CONSTEXPR_FUNCTION Span<T, Count> last() const
    {
        CELER_EXPECT(Count == 0 || Count <= this->size());
        return {this->data() + this->size() - Count, Count};
    }
    CELER_CONSTEXPR_FUNCTION
    Span<T, dynamic_extent> last(size_type count) const
    {
        CELER_EXPECT(count <= this->size());
        return {this->data() + this->size() - count, count};
    }
    //!@}

  private:
    //! Storage
    detail::SpanImpl<T, Extent> s_;
};

//---------------------------------------------------------------------------//
// DEDUCTION GUIDES
//---------------------------------------------------------------------------//

// Deduction guide for pointer and size
template<class T>
Span(T*, std::size_t) -> Span<T>;

// Deduction guide for two iterators
template<class Iter>
Span(Iter, Iter) -> Span<typename std::iterator_traits<Iter>::value_type>;

// Deduction guide for C array
template<class T, std::size_t N>
Span(T (&)[N]) -> Span<T, N>;

//---------------------------------------------------------------------------//
// FREE FUNCTIONS
//---------------------------------------------------------------------------//
//! Get a mutable fixed-size view to an array
template<class T, std::size_t N>
CELER_CONSTEXPR_FUNCTION Span<T, N> make_span(Array<T, N>& x)
{
    return {x.data(), N};
}

//---------------------------------------------------------------------------//
//! Get a constant fixed-size view to an array
template<class T, std::size_t N>
CELER_CONSTEXPR_FUNCTION Span<T const, N> make_span(Array<T, N> const& x)
{
    return {x.data(), N};
}

//---------------------------------------------------------------------------//
//! Get a mutable fixed-size view to a C array
template<class T, std::size_t N>
CELER_CONSTEXPR_FUNCTION Span<T, N> make_span(T (&arr)[N])
{
    return {arr};
}

//---------------------------------------------------------------------------//
//! Get a mutable view to a generic container
template<class T>
CELER_CONSTEXPR_FUNCTION Span<typename T::value_type> make_span(T& cont)
{
    return {cont.data(), cont.size()};
}

//---------------------------------------------------------------------------//
//! Get a const view to a generic container
template<class T>
CELER_CONSTEXPR_FUNCTION Span<typename T::value_type const>
make_span(T const& cont)
{
    return {cont.data(), cont.size()};
}

//---------------------------------------------------------------------------//
//! Construct an array from a fixed-size span
template<class T, std::size_t N>
CELER_CONSTEXPR_FUNCTION auto to_array(Span<T, N> s)
{
    Array<std::remove_cv_t<T>, N> result{};
    for (std::size_t i = 0; i < N; ++i)
    {
        result[i] = s[i];
    }
    return result;
}

// DEPRECATED: remove in v1.0
template<class T, std::size_t N>
[[deprecated("use to_array")]] CELER_CONSTEXPR_FUNCTION auto
make_array(Span<T, N> s)
{
    return to_array(s);
}

#if !CELER_DEVICE_COMPILE
//---------------------------------------------------------------------------//
/*!
 * Write the elements of array \a a to stream \a os.
 */
template<class T, std::size_t N>
CELER_FORCEINLINE std::ostream&
operator<<(std::ostream& os, Span<T, N> const& s)
{
    os << StreamableContainer{s.data(), s.size()};
    return os;
}
#endif

//---------------------------------------------------------------------------//
}  // namespace celeritas

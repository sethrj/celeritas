//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file StackAllocation.i.hh
//---------------------------------------------------------------------------//

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct with a reference to an allocation.
 */
template<class T, Ownership W, MemSpace M>
StackAllocation<T, W, M>::StackAllocation(const Pointers& data) : data_(data)
{
    CELER_EXPECT(data);
}

//---------------------------------------------------------------------------//
/*!
 * Access a single element.
 */
template<class T, Ownership W, MemSpace M, class I>
CELER_FUNCTION auto Collection<T, W, M, I>::operator[](ItemIdT i) -> reference
{
    CELER_EXPECT(i < this->size());
    return this->storage()[i.get()];
}

//---------------------------------------------------------------------------//
/*!
 * Access a single element (const).
 */
template<class T, Ownership W, MemSpace M, class I>
CELER_FUNCTION auto Collection<T, W, M, I>::operator[](ItemIdT i) const
    -> const_reference
{
    CELER_EXPECT(i < this->size());
    return this->storage()[i.get()];
}

//---------------------------------------------------------------------------//
/*!
 * Access a subset of the data as a Span.
 */
template<class T, Ownership W, MemSpace M, class I>
CELER_FUNCTION auto Collection<T, W, M, I>::operator[](ItemRangeT ir) -> SpanT
{
    CELER_EXPECT(*ir.end() < this->size() + 1);
    pointer data = this->data();
    return {data + ir.begin()->get(), data + ir.end()->get()};
}

//---------------------------------------------------------------------------//
/*!
 * Access a subset of the data as a Span (const).
 */
template<class T, Ownership W, MemSpace M, class I>
CELER_FUNCTION auto Collection<T, W, M, I>::operator[](ItemRangeT ir) const
    -> SpanConstT
{
    CELER_EXPECT(*ir.end() < this->size() + 1);
    const_pointer data = this->data();
    return {data + ir.begin()->get(), data + ir.end()->get()};
}

//---------------------------------------------------------------------------//
/*!
 * Access a subset of the data as a Span.
 */
template<class T, Ownership W, MemSpace M, class I>
CELER_FUNCTION auto Collection<T, W, M, I>::operator[](AllItemsT) -> SpanT
{
    return {this->data(), this->size()};
}

//---------------------------------------------------------------------------//
/*!
 * Access a subset of the data as a Span (const).
 */
template<class T, Ownership W, MemSpace M, class I>
CELER_FUNCTION auto Collection<T, W, M, I>::operator[](AllItemsT) const
    -> SpanConstT
{
    return {this->data(), this->size()};
}

//---------------------------------------------------------------------------//
/*!
 * Get the dynamic size of the allocation.
 */
template<class T, Ownership W, MemSpace M, class I>
CELER_CONSTEXPR_FUNCTION auto Collection<T, W, M, I>::size() const -> size_type
{
    // TODO: cache locally or use __ldg to load
    return data_.size[ItemId<size_type>{0}];
}

//---------------------------------------------------------------------------//
/*!
 * Whether any elements were allocated.
 */
template<class T, Ownership W, MemSpace M, class I>
CELER_CONSTEXPR_FUNCTION bool Collection<T, W, M, I>::empty() const
{
    return this->size() == 0;
}

//---------------------------------------------------------------------------//
} // namespace celeritas

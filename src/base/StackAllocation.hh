//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file StackAllocation.hh
//---------------------------------------------------------------------------//
#pragma once

#include "Collection.hh"
#include "StackAllocatorInterface.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Collection-like access to dynamically allocated data.
 *
 * This can be safely used in a kernel that's not doing any allocating.
 */
template<class T, Ownership W, MemSpace M>
class StackAllocation
{
    using StorageCollection = Collection<T, W, M>;

  public:
    //!@{
    //! Type aliases
    using Pointers = StackAllocatorData<T, W, M>;

    using size_type       = typename StorageCollection::size_type;
    using reference       = typename StorageCollection::reference;
    using const_reference = typename StorageCollection::const_reference;
    using SpanT           = typename StorageCollection::SpanT;
    using SpanConstT      = typename StorageCollection::SpanConstT;
    using ItemIdT         = typename StorageCollection::ItemIdT;
    using ItemRangeT      = typename StorageCollection::Range<ItemIdT>;
    using AllItemsT       = typename StorageCollection::AllItems<T, M>;
    //!@}

  public:
    // Construct from allocator data
    explicit inline CELER_FUNCTION StackAllocation(const Pointers& data);

    // Access a single element
    inline CELER_FUNCTION reference       operator[](ItemIdT i);
    inline CELER_FUNCTION const_reference operator[](ItemIdT i) const;

    // Access a subset of the data with a slice
    inline CELER_FUNCTION SpanT      operator[](ItemRangeT ir);
    inline CELER_FUNCTION SpanConstT operator[](ItemRangeT ir) const;

    // Access all data
    inline CELER_FUNCTION SpanT      operator[](AllItemsT);
    inline CELER_FUNCTION SpanConstT operator[](AllItemsT) const;

    // Access dynamic size
    inline CELER_FUNCTION size_type size() const;
    inline CELER_FUNCTION bool      empty() const;

  private:
    const Pointers& data_;

    CELER_FORCEINLINE_FUNCTION pointer data()
    {
        return &data_.storage[ItemIdT{0}];
    }
    CELER_FORCEINLINE_FUNCTION const_pointer data() const
    {
        return &data_.storage[ItemIdT{0}];
    }
};

//---------------------------------------------------------------------------//
} // namespace celeritas

#include "StackAllocation.i.hh"

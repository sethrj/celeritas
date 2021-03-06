//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file StackAllocator.i.hh
//---------------------------------------------------------------------------//
#include <new>
#include "Atomics.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct with defaults.
 */
template<class T>
CELER_FUNCTION StackAllocator<T>::StackAllocator(const Pointers& shared)
    : data_(shared)
{
    CELER_EXPECT(shared);
}

//---------------------------------------------------------------------------//
/*!
 * Get the maximum number of values that can be allocated.
 */
template<class T>
CELER_FUNCTION auto StackAllocator<T>::capacity() const -> size_type
{
    return data_.storage.size();
}

//---------------------------------------------------------------------------//
/*!
 * Allocate space for a given number of items.
 *
 * Returns an invalid if allocation failed due to out-of-memory. Ensures that
 * the shared size reflects the amount of data allocated.
 */
template<class T>
CELER_FUNCTION auto StackAllocator<T>::operator()(size_type count) -> ItemIdT
{
    CELER_EXPECT(count > 0 && count <= capacity);

    const size_type             capacity = data_.storage.size();
    constexpr ItemId<size_type> size_id{0};

    // Atomic add 'count' to the shared size
    size_type start = atomic_add(&data_.size[size_id], count);
    if (CELER_UNLIKELY(start + count > capacity))
    {
        // Out of memory: restore the old value so that another thread can
        // potentially use it. Multiple threads are likely to exceed the
        // capacity simultaneously. Only one has a "start" value less than or
        // equal to the total capacity: the remainder are (arbitrarily) higher
        // than that.
        if (start <= capacity)
        {
            // We were the first thread to exceed capacity, even though other
            // threads might have failed (and might still be failing) to
            // allocate. Restore the actual allocated size to the start value.
            // This might allow another thread with a smaller allocation to
            // succeed, but it also guarantees that at the end of the kernel,
            // the size reflects the actual capacity.
            data_.size[size_id] = start;
        }

        // TODO Add an "out of memory" flag to make it easier for host code to
        // detect whether a failure occurred, rather than looping through
        // tracks and testing for failure.

        // Return invalid ID, indicating failure to allocate.
        return {};
    }

    // Initialize the data at the newly "allocated" address
    ItemRange

        value_type* result
        = new (&data_.storage[StorageId{start}]) value_type;
    for (size_type i = 1; i < count; ++i)
    {
        // Initialize remaining values
        new (&data_.storage[StorageId{start + i}]) value_type;
    }
    return result;
}

//---------------------------------------------------------------------------//
/*!
 * View all allocated data.
 *
 * This cannot be called while any running kernel could be modifiying the size.
 */
template<class T>
CELER_FUNCTION auto StackAllocator<T>::get() -> Span<value_type>
{
    return data_.storage[ItemRange<T>{StorageId{0}, StorageId{this->size()}}];
}

//---------------------------------------------------------------------------//
/*!
 * View all allocated data (const).
 *
 * This cannot be called while any running kernel could be modifiying the size.
 */
template<class T>
CELER_FUNCTION auto StackAllocator<T>::get() const -> Span<const value_type>
{
    return data_.storage[ItemRange<T>{StorageId{0}, StorageId{this->size()}}];
}

//---------------------------------------------------------------------------//
} // namespace celeritas

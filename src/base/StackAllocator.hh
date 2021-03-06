//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file StackAllocator.hh
//---------------------------------------------------------------------------//
#pragma once

#include "Collection.hh"
#include "StackAllocatorInterface.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Dynamically allocate a range of items in a collection.
 *
 * The stack allocator view acts as a functor and accessor to the allocated
 * data. It enables very fast on-device dynamic allocation of data, such as
 * secondaries or detector hits. As an example, inside a hypothetical physics
 * Interactor class, you could create two particles with the following code:
 * \code
 struct Interactor
 {
    StackAllocator<Secondary> allocate;

    // Sample an interaction
    template<class Engine>
    Interaction operator()(Engine&)
    {
       // Create 2 secondary particles
       SecondaryId allocated = this->allocate(2);
       if (!allocated)
       {
           return Interaction::from_failure();
       }
       Interaction result;
       result.secondaries = {allocated, 4};
       return result;
    };
 };
 \endcode
 *
 * A later kernel could then iterate over the secondaries to apply cutoffs:
 * \code
 using SecondaryRef
     = StackAllocatorData<Secondary, Ownership::reference, MemSpace::device>;

 __global__ apply_cutoff(const SecondaryRef ptrs)
 {
     auto thread_idx = celeritas::KernelParamCalculator::thread_id().get();
     StackAllocation<Secondary> secondaries(ptrs);
     if (thread_idx < secondaries.size())
     {
         Secondary& sec = secondaries[SecondaryId{thread_idx}];
         if (sec.energy < 100 * units::kilo_electron_volts)
         {
             sec.energy = 0;
         }
     }
 }
 * \endcode
 *
 * You *cannot* safely access the current size of the stack in the same kernel
 * that's modifying it, so you should not have a \c StackAllocator in the same
 * kernel as a \c StackAllocation (if they point to the same data).
 */
template<class T>
class StackAllocator
{
  public:
    //!@{
    //! Type aliases
    using Pointers
        = StackAllocatorData<T, Ownership::reference, MemSpace::native>;
    using ItemSpanT = ItemSpan<T>;
    //!@}

  public:
    // Construct with shared data
    explicit inline CELER_FUNCTION StackAllocator(const Pointers& data);

    // Allocate space for this many items
    inline CELER_FUNCTION ItemIdT operator()(size_type count);

  private:
    const Pointers& data_;
};

//---------------------------------------------------------------------------//
} // namespace celeritas

#include "StackAllocator.i.hh"

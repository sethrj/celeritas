//----------------------------------*-C++-*----------------------------------//
// Copyright 2022 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/random/XorwowRngData.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Macros.hh"
#include "corecel/Types.hh"
#include "corecel/cont/Array.hh"
#include "corecel/data/Collection.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
//! Element index into the XORWOW state
enum class XorwowElement
{
    x,
    y,
    z,
    w,
    v,
    d, //!< Weyl state
    size_
};

//---------------------------------------------------------------------------//
/*!
 * Persistent data for XORWOW generator.
 *
 * If we want to add the "discard" operation or support initialization with a
 * subsequence or offset, we can add the precomputed XORWOW jump matrices here.
 */
template<Ownership W, MemSpace M>
struct XorwowRngParamsData
{
    // TODO: 256-bit seed used to generate initial states for the RNGs
    // For now, just 4 bytes (same as our existing cuda/hip interface)
    Array<unsigned int, 1> seed;

    //// METHODS ////

    //! Whether the data is assigned
    explicit CELER_FUNCTION operator bool() const { return true; }

    //! Assign from another set of data
    template<Ownership W2, MemSpace M2>
    XorwowRngParamsData& operator=(const XorwowRngParamsData<W2, M2>& other)
    {
        CELER_EXPECT(other);
        seed = other.seed;
        return *this;
    }
};

//---------------------------------------------------------------------------//
/*!
 * XORWOW generator states for all threads.
 */
template<Ownership W, MemSpace M>
struct XorwowRngStateData
{
    //// TYPES ////

    using uint_t = unsigned int;
    template<class T>
    using Items = Collection<T, W, M>;

    static_assert(sizeof(uint_t) == 4, "Expected 32-bit int");

    //// DATA ////

    Items<uint_t> state; //!< [x, y, z, w, v, d][thread/pitch]
    size_type     pitch{};
    size_type     num_threads{};

    //// METHODS ////

    //! Number of threads per state
    CELER_FUNCTION size_type size() const { return num_threads; }

    //! True if assigned
    explicit CELER_FUNCTION operator bool() const
    {
        return num_threads > 0 && pitch >= num_threads
               && state.size()
                      == pitch * static_cast<size_type>(XorwowElement::size_);
    }

    //! Assign from another set of states
    template<Ownership W2, MemSpace M2>
    XorwowRngStateData& operator=(XorwowRngStateData<W2, M2>& other)
    {
        CELER_EXPECT(other);
        state = other.state;
        pitch       = other.pitch;
        num_threads = other.num_threads;
        return *this;
    }
};

//---------------------------------------------------------------------------//
/*!
 * Resize and seed the RNG states.
 *
 * cuRAND and hipRAND implement functions that permute the original (but
 * seemingly arbitrary) seeds given in Marsaglia with 64 bits of seed data. We
 * simply generate pseudorandom, independent starting states for all data in
 * all threads using MT19937.
 */
template<MemSpace M>
void resize(XorwowRngStateData<Ownership::value, M>* state,
            const HostCRef<XorwowRngParamsData>&     params,
            size_type                                size);

//---------------------------------------------------------------------------//
} // namespace celeritas

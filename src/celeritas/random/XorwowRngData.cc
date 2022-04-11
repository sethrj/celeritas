//----------------------------------*-C++-*----------------------------------//
// Copyright 2022 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/random/XorwowRngData.cc
//---------------------------------------------------------------------------//
#include "XorwowRngData.hh"

#include <random>

#include "corecel/Assert.hh"
#include "corecel/data/Collection.hh"
#include "corecel/data/CollectionBuilder.hh"
#include "corecel/sys/Device.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Resize and initialize with the seed stored in params.
 */
template<MemSpace M>
void resize(XorwowRngStateData<Ownership::value, M>* state,
            const HostCRef<XorwowRngParamsData>&     params,
            size_type                                size)
{
    CELER_EXPECT(size > 0);
    CELER_EXPECT(params);

    XorwowRngStateData<Ownership::value, MemSpace::host> host_state;
    host_state.num_threads = size;
    host_state.pitch       = size;
    if (M == MemSpace::device)
    {
        // Round up size to the nearest multiple of warp size for coalescing
        size_type divisor   = celeritas::device().threads_per_warp();
        size_type remainder = size % divisor;
        host_state.pitch    = size + (remainder ? divisor - remainder : 0);
        CELER_ASSERT(host_state.pitch >= size);
    }

    // Seed sequence to generate well-distributed seed numbers
    std::seed_seq seeds(params.seed.begin(), params.seed.end());
    // 32-bit generator to fill initial states
    std::mt19937                          rng(seeds);
    std::uniform_int_distribution<unsigned int> sample_uniform_int;

    // Resize initial state on host
    make_builder(&host_state.state)
        .resize(host_state.pitch
                * static_cast<size_type>(XorwowElement::size_));

    // Fill all seeds with random data. The xorstate is never all zeros.
    for (unsigned int& u : host_state.state[AllItems<unsigned int>{}])
    {
        u = sample_uniform_int(rng);
    }

    // Copy to input
    *state = host_state;

    CELER_ENSURE(*state);
    CELER_ENSURE(state->size() == size);
}

//---------------------------------------------------------------------------//
// Explicit instantiations
template void resize(HostVal<XorwowRngStateData>*,
                     const HostCRef<XorwowRngParamsData>&,
                     size_type);

template void resize(XorwowRngStateData<Ownership::value, MemSpace::device>*,
                     const HostCRef<XorwowRngParamsData>&,
                     size_type);

//---------------------------------------------------------------------------//
} // namespace celeritas

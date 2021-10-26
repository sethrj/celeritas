//---------------------------------*-C++-*-----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file RngEngine.i.hh
//---------------------------------------------------------------------------//

#include "celeritas_config.h"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct from state.
 */
CELER_FUNCTION
RngEngine::RngEngine(const StateRef& state, const ThreadId& id)
{
    CELER_EXPECT(id < state.rng.size());
    state_ = &state.rng[id].state;
    local_state_ = *state_;
}

//---------------------------------------------------------------------------//
/*!
 * Store to global memory on destruct.
 */
CELER_FUNCTION
RngEngine::~RngEngine()
{
    *state_ = local_state_;
}

//---------------------------------------------------------------------------//
/*!
 * Initialize the RNG engine with a seed value.
 */
CELER_FUNCTION RngEngine& RngEngine::operator=(const Initializer_t& s)
{
    curand_init(s.seed, 0, 0, &local_state_);
    return *this;
}

//---------------------------------------------------------------------------//
/*!
 * Sample a random number.
 */
CELER_FUNCTION auto RngEngine::operator()() -> result_type
{
    return curand(&local_state_);
}

//---------------------------------------------------------------------------//
// Specializations for GenerateCanonical
//---------------------------------------------------------------------------//
/*!
 * Specialization for RngEngine (float).
 */
CELER_FUNCTION float
GenerateCanonical<RngEngine, float>::operator()(RngEngine& rng)
{
    return curand_uniform(&rng.local_state_);
}

//---------------------------------------------------------------------------//
/*!
 * Specialization for RngEngine (double).
 */
CELER_FUNCTION double
GenerateCanonical<RngEngine, double>::operator()(RngEngine& rng)
{
    return curand_uniform_double(&rng.local_state_);
}

//---------------------------------------------------------------------------//
} // namespace celeritas

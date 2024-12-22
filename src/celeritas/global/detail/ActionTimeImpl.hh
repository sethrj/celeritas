//----------------------------------*-C++-*----------------------------------//
// Copyright 2024 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/global/detail/ActionTimeImpl.hh
//---------------------------------------------------------------------------//
#pragma once

#include <vector>

#include "corecel/Assert.hh"
#include "corecel/data/AuxInterface.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Store times for each step action.
 *
 * Note that the size of this vector is the number of \em step actions, not
 * based on the action ID.
 */
struct ActionTimeState final : public AuxStateInterface
{
    std::vector<double> time;

    //! True if initialized
    explicit operator bool() const { return !time.empty(); }
};

//---------------------------------------------------------------------------//
/*!
 * Accumulate into an action time state.
 *
 * This is to be used by \c ActionSequence to record into an \c ActionTimes
 * class.
 */
class ActionTimeAccumulator
{
  public:
    // Construct with pointer to action time state
    inline ActionTimeAccumulator(ActionTimeState* state);

    // Accumulate time into the given step index
    inline void operator()(std::size_t i, double elapsed);

  private:
    ActionTimeState* state_;
};

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Construct with pointer to action time state.
 */
ActionTimeAccumulator::ActionTimeAccumulator(ActionTimeState* state)
    : state_{state}
{
    CELER_EXPECT(state_ && *state_);
}

//---------------------------------------------------------------------------//
/*!
 * Accumulate time into the given step index.
 */
void ActionTimeAccumulator::operator()(std::size_t i, double elapsed)
{
    CELER_EXPECT(i < state_->time.size());
    CELER_EXPECT(elapsed > 0);
    state_->time[i] += elapsed;
}

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

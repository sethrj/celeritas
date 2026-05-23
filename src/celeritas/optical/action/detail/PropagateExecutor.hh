//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/optical/action/detail/PropagateExecutor.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Assert.hh"
#include "corecel/Macros.hh"
#include "celeritas/Types.hh"
#include "celeritas/optical/CoreTrackView.hh"
#include "celeritas/optical/SimTrackView.hh"

namespace celeritas
{
namespace optical
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Move a track to the next interaction or geometry boundary.
 *
 * This should only apply to alive tracks.
 */
struct PropagateExecutor
{
    inline CELER_FUNCTION void operator()(CoreTrackView& track);
};

//---------------------------------------------------------------------------//
CELER_FUNCTION void PropagateExecutor::operator()(CoreTrackView& track)
{
    auto&& sim = track.sim();
    CELER_ASSERT(sim.status() == TrackStatus::alive);

    // Propagate up to the physics distance
    real_type step = sim.step_length();
    CELER_ASSERT(step > 0);

    auto&& geo = track.geometry();
    Propagation p = geo.find_next_step(step);
    if (p.boundary)
    {
        geo.move_to_boundary();
        sim.step_length(p.distance);
        sim.post_step_action(
            track.surface_physics().scalars().init_boundary_action);
    }
    else
    {
        CELER_ASSERT(step == p.distance);
        geo.move_internal(step);
    }
}

class PropagateThreadExecutor
{
  public:
    //!@{
    //! \name Type aliases
    using ParamsPtr = CoreParamsPtr<MemSpace::native>;
    using StatePtr = CoreStatePtr<MemSpace::native>;
    using Applier = AppliesValidVolumetric;
    //!@}

  public:
    //! Construct with condition and operator
    CELER_FUNCTION
    PropagateThreadExecutor(ParamsPtr params,
                            StatePtr state,
                            Applier&& applies,
                            PropagateExecutor&& execute_track)
        : params_{params}
        , state_{state}
        , applies_{celeritas::move(applies)}
        , execute_track_{celeritas::move(execute_track)}
    {
    }

    //! Launch the given thread if the track meets the condition
    CELER_FUNCTION void operator()(TrackSlotId ts)
    {
        CELER_EXPECT(ts < state_->size());
        CoreTrackView track(*params_, *state_, ts);
        if (!applies_(track))
        {
            return;
        }

        return execute_track_(track);
    }

    //! Call the underlying function using the thread index
    CELER_FORCEINLINE_FUNCTION void operator()(ThreadId thread)
    {
        // For optical photons, thread index maps exactly to
        return (*this)(TrackSlotId{thread.unchecked_get()});
    }

  private:
    ParamsPtr const params_;
    StatePtr const state_;
    Applier applies_;
    PropagateExecutor execute_track_;
};

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace optical
}  // namespace celeritas

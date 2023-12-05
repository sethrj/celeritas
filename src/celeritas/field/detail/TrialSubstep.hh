//----------------------------------*-C++-*----------------------------------//
// Copyright 2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/field/detail/TrialSubstep.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Types.hh"
#include "celeritas/field/FieldPropagatorData.hh"
#include "celeritas/field/Types.hh"

#include "FieldUtils.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Result of a trial substep.
 *
 * This is an abstraction for querying the result of a trial substep as it
 * relates to the geometry boundary.
 */
class TrialSubstep
{
  public:
    // Find the intercept distance during construction
    template<class F>
    CELER_FUNCTION TrialSubstep(FieldPropagatorOptions const& options,
                                F&& find_next_step,
                                Real3 const& start_pos,
                                bool start_boundary,
                                DriverResult const& end_substep);

    //// ACCESSORS ////

    //! Get the ODE state at the end of the trial
    CELER_FUNCTION OdeState const& end_state() const { return substep_.state; }

    //! Exact distance of the integrated substep
    CELER_FUNCTION real_type substep() const { return substep_.step; }

    //! Substep length scaled by the intercept/chord length fraction
    CELER_FUNCTION real_type scaled_substep() const { return scaled_substep_; }

    //// QUERIES ////

    // The boundary is truly no further than the end of the step
    inline CELER_FUNCTION bool true_boundary() const;

    // No boundary found even after searching a bit beyond the chord length
    inline CELER_FUNCTION bool no_boundary() const;

    // The particle appears stuck on a boundary.
    inline CELER_FUNCTION bool stuck() const;

    // The distance to the boundary is almost the full substep.
    inline CELER_FUNCTION bool length_almost_boundary() const;

    // The intercept point is spatially close to the substep end point.
    inline CELER_FUNCTION bool endpoint_near_boundary() const;

    // The substep length is so small that the chord length is zero.
    inline CELER_FUNCTION bool degenerate_chord() const;

  private:
    // TODO: unfold driver result, linear step

    FieldPropagatorOptions const& options_;
    Real3 const& start_pos_;
    bool start_boundary_;
    DriverResult substep_;

    Chord chord_;  // {length, dir}
    Propagation linear_step_;  // {distance, flags}
    real_type scaled_substep_;
};

//---------------------------------------------------------------------------//
/*!
 * Find the interception distance during construction.
 */
template<class F>
CELER_FUNCTION TrialSubstep::TrialSubstep(FieldPropagatorOptions const& options,
                                          F&& find_next_step,
                                          Real3 const& start_pos,
                                          bool start_boundary,
                                          DriverResult const& end_substep)
    : options_{options}
    , start_pos_{start_pos}
    , start_boundary_{start_boundary}
    , substep_{end_substep}
{
    CELER_ASSERT(substep_.step > 0);

    // Calculate the straight-line distance between the start and the end
    // of the substep
    chord_ = detail::make_chord(start_pos_, end_substep.state.pos);
    // Calculate the distance to the end point, searching a bit beyond
    // because of the allowable tolerance
    linear_step_ = find_next_step(chord_);
    CELER_ASSERT(linear_step_.distance
                 <= chord_.length + options_.delta_intersection());

    // Scale the effective substep length to travel by the fraction along
    // the chord to the boundary. This fraction can be slightly larger than 1
    // because we might search up a little past the endpoint (thanks to the
    // delta intersection). It *might* be NaN if the chord
    // length is degenerate.
    // NOTE: this will be unused if no intersection is found
    scaled_substep_ = (linear_step_.distance / chord_.length) * substep_.step;
}

//---------------------------------------------------------------------------//
/*!
 * The boundary is truly no further than the end of the step.
 *
 * This is used to guarantee that moving to the boundary won't exceed the
 * physical path length.
 * (TODO: since the substep can be slightly longer than requested step, could
 * this result in a coincident boundary?)
 */
CELER_FORCEINLINE_FUNCTION bool TrialSubstep::true_boundary() const
{
    return linear_step_.distance <= chord_.length;
}

//---------------------------------------------------------------------------//
/*!
 * No boundary was found even after searching a bit beyond the chord length.
 */
CELER_FORCEINLINE_FUNCTION bool TrialSubstep::no_boundary() const
{
    return !linear_step_.boundary;
}

//---------------------------------------------------------------------------//
/*!
 * The particle appears stuck on a boundary.
 *
 * This happens when the particle is on a boundary but the next boundary
 * reported is less than the bump distance.
 */
CELER_FORCEINLINE_FUNCTION bool TrialSubstep::stuck() const
{
    return start_boundary_ && linear_step_.distance < options_.bump_distance();
}

//---------------------------------------------------------------------------//
/*!
 * The distance to the boundary is almost the full substep.
 *
 * The intercept point is close enough to the trial substep end point that
 * the next trial step would be less than the minimum substep.
 */
CELER_FUNCTION bool TrialSubstep::length_almost_boundary() const
{
    return linear_step_.boundary
           && scaled_substep_ <= options_.minimum_substep();
}

//---------------------------------------------------------------------------//
/*!
 * The intercept point is spatially close to the substep end point.
 *
 * The straight-line intersection point is a distance less than
 * `delta_intersection` from the substep's end position.
 */
CELER_FUNCTION bool TrialSubstep::endpoint_near_boundary() const
{
    return linear_step_.boundary
           && detail::is_intercept_close(start_pos_,
                                         chord_.dir,
                                         linear_step_.distance,
                                         substep_.state.pos,
                                         options_.delta_intersection());
}

//---------------------------------------------------------------------------//
/*!
 * The substep length is so small that the chord length is zero.
 *
 * The end of the substep is too close to the beginning (which could
 * happen for very small initial "step" requests, especially if
 * using single precision arithmetic).
 */
bool TrialSubstep::degenerate_chord() const
{
    return chord_.length == 0;
}

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/field/FieldPropagator.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Macros.hh"
#include "corecel/Types.hh"
#include "corecel/math/Algorithms.hh"
#include "corecel/math/ArrayOperators.hh"
#include "corecel/math/NumericLimits.hh"
#include "orange/Types.hh"
#include "celeritas/phys/ParticleTrackView.hh"

#include "FieldSubstepper.hh"
#include "Types.hh"
#include "detail/FieldUtils.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Propagate a charged particle in a field.
 *
 * For a given initial state (position, momentum), it propagates a charged
 * particle along a curved trajectory up to an interaction length proposed by
 * a chosen physics process for the step, possibly integrating sub-steps by
 * an adaptive step control with a required accuracy of tracking in a
 * field. It updates the final state (position, momentum, boundary) along with
 * the step actually taken.  If the final position is outside the current
 * volume, it returns a geometry limited step and the state at the
 * intersection between the curve trajectory and the first volume boundary
 * using an iterative step control method within a tolerance error imposed on
 * the closest distance between two positions by the field stepper and the
 * linear projection to the volume boundary.
 *
 * \note This follows similar methods as in Geant4's G4PropagatorInField class.
 */
template<class DriverT, class GTV>
class FieldPropagator
{
  public:
    //!@{
    //! \name Type aliases
    using result_type = Propagation;
    //!@}

  public:
    // Construct with parameters, states, and the driver type.
    inline CELER_FUNCTION FieldPropagator(FieldPropagatorOptions const& options,
                                          DriverT&& driver,
                                          ParticleTrackView const& particle,
                                          GTV&& geo);

    //! Compatibility construction to get field options
    CELER_FUNCTION FieldPropagator(DriverT&& driver,
                                   ParticleTrackView const& particle,
                                   GTV&& geo)
        : FieldPropagator{driver.driver_options(),
                          ::celeritas::forward<DriverT>(driver),
                          ::celeritas::forward<GTV>(geo)}
    {
    }

    // Move track to next volume boundary.
    inline CELER_FUNCTION result_type operator()();

    // Move track up to a user-provided distance, or to the next boundary
    inline CELER_FUNCTION result_type operator()(real_type dist);

    //! Whether it's possible to have tracks that are looping
    static CELER_CONSTEXPR_FUNCTION bool tracks_can_loop() { return true; }

  private:
    //// DATA ////

    FieldPropagatorOptions const& options_;
    DriverT driver_;
    GeoFieldState<GTV> state_;
};

//---------------------------------------------------------------------------//
// DEDUCTION GUIDES
//---------------------------------------------------------------------------//
template<class DriverT, class GTV>
CELER_FUNCTION FieldPropagator(DriverT&&, ParticleTrackView const&, GTV&&)
    ->FieldPropagator<DriverT, GTV>;

//---------------------------------------------------------------------------//
template<class DriverT, class GTV>
CELER_FUNCTION FieldPropagator(FieldPropagatorOptions const&,
                               DriverT&&,
                               ParticleTrackView const&,
                               GTV&&) -> FieldPropagator<DriverT, GTV>;

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Construct with shared field parameters and the field driver.
 */
template<class DriverT, class GTV>
CELER_FUNCTION FieldPropagator<DriverT, GTV>::FieldPropagator(
    DriverT&& driver, ParticleTrackView const& particle, GTV&& geo)
    : driver_{::celeritas::forward<DriverT>(driver)}
    , state_{::celeritas::forward<GTV>(geo), geo.pos(), geo.dir()}
{
    using MomentumUnits = OdeState::MomentumUnits;

    state_.pos = geo_.pos();
    state_.mom = value_as<MomentumUnits>(particle.momentum()) * geo_.dir();
}

//---------------------------------------------------------------------------//
/*!
 * Propagate a charged particle until it hits a boundary.
 */
template<class DriverT, class GTV>
CELER_FUNCTION auto FieldPropagator<DriverT, GTV>::operator()() -> result_type
{
    return (*this)(numeric_limits<real_type>::infinity());
}

//---------------------------------------------------------------------------//
/*!
 * Propagate a charged particle in a field.
 *
 * It utilises a field driver (based on an adaptive step control to limit the
 * length traveled based on the magnetic field behavior and geometric
 * tolerances) to track a charged particle along a curved trajectory for a
 * given step length within a required accuracy or intersects
 * with a new volume (geometry limited step).
 *
 * The position of the internal OdeState `state_` should be consistent with the
 * geometry `geo_`'s position, but the geometry's direction will be a series
 * of "trial" directions that are the chords between the start and end points
 * of a curved substep through the field. At the end of the propagation step,
 * the geometry state's direction is updated based on the actual value of the
 * calculated momentum.
 *
 * Caveats:
 * - The physical (geometry track state) position may deviate from the exact
 *   curved propagation position up to a driver-based tolerance at every
 *   boundary crossing. The momentum will always be conserved, though.
 * - In some unusual cases (e.g. a very small caller-requested step, or an
 *   unusual accumulation in the driver's substeps) the distance returned may
 *   be slightly higher (again, up to a driver-based tolerance) than the
 *   physical distance travelled.
 */
template<class DriverT, class GTV>
CELER_FUNCTION auto FieldPropagator<DriverT, GTV>::operator()(real_type step)
    -> result_type
{
    CELER_EXPECT(step > 0);

    StepperImpl stepper{step, geo_};
    NextStepFinder find_next_step{geo_}

    // Break the curved steps into substeps as determined by the driver *and*
    // by the proximity of geometry boundaries. Test for intersection with the
    // geometry boundary in each substep. Accept the substep and move
    // internally if no boundary is nearby.
    // This loop is guaranteed to converge since the trial step always
    // decreases *or* the actual position advances.
    SubstepStatus status{SubstepStatus::iterating};
    while (status == SubstepStatus::iterating)
    {
        CELER_ASSERT(soft_zero(distance(state_.pos, geo_.pos())));
        CELER_ASSERT(stepper.boundary == geo_.is_on_boundary());

        // Advance up to (but probably less than) the trial step length
        TrialState trial{options_,
                         driver_.advance(stepper.trial_step, state_),
                         state_.pos,
                         find_next_step};
        CELER_ASSERT(trial.substep() <= stepper.trial_step)

        if (trial.no_boundary())
        {
            substepper.accept_internal(trial);
        }
        else if (CELER_UNLIKELY(trial.stuck()))
        {
            substepper.retry_stuck(trial);
        }
        else if (trial.length_almost_boundary()
                 || trial.endpoint_near_boundary()
                 || CELER_UNLIKELY(trial.degenerate_chord()))
        {
            // Commit the proposed state's momentum, use the
            // post-boundary-crossing track position for consistency, and
            // conservatively reduce the *reported* traveled distance to avoid
            // coincident boundary crossings.
            substepper.accept_likely_boundary(trial);
        }
        else
        {
            // A boundary was hit but the straight-line intercept is too far
            // from substep's end state.
            // Decrease the allowed substep (curved path distance) by the
            // fraction along the chord, and retry the driver step.
            substepper.update_trial_step(trial);
        }
        status = substepper.status();
    }

    if (status == Status::move_to_boundary)
    {
        // We moved to a new boundary. Update the position to reflect the
        // geometry's state (and possibly "bump" the ODE state's position
        // because of the tolerance in the intercept checks above).
        substepper.cross_boundary();
    }
    else if (status == Status::moved_internal)
    {
        // Make sure the distance travelled is exactly the input step length
        substepper.fixup_internal_step();
    }

    substepper.bump();

    if (status == Status::stuck)
    {
        substepper.unstick();
    }

    // Convert the substepper inteals to the result
    Propagation result;
    result.distance = substepper.travelled();
    result.boundary = state_.boundary;
    result.looping = (status == Status::looping);

    // Due to accumulation errors from multiple substeps or chord-finding
    // within the driver, the distance may be very slightly beyond the
    // requested step.
    CELER_ENSURE(
        result.distance > 0
        && (result.distance <= step || soft_equal(result.distance, step)));
    CELER_ENSURE(result.boundary == geo_.is_on_boundary()
                 || status == Status::stuck);
    return result;
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

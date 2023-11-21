//----------------------------------*-C++-*----------------------------------//
// Copyright 2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/field/FieldSubstepper.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Types.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Result of a substep iteration.
 */
enum class SubstepStatus
{
    iterating = 0,  //!< Still performing substeps (or trial substeps)
    boundary,  //!< Intersected a boundary
    moved_internal,  //!< Reached the end of the step length
    stuck = -1,  //!< The particle won't move from the boundary
    looping = stuck - 1,  //!< No boundary is found after numerous substeps
};

//---------------------------------------------------------------------------//
/*!
 * State local to the propagator and modified by the substepper.
 *
 */
template<class GTV>
struct GeoFieldState
{
    GTV geo;
    OdeState state;
    bool boundary;
};

template<class GTV>
struct FieldSubstepper
{
    // Input: propagation step
    real_type const propagation_step_;
    GeoFieldState* state_;

    //! Cumulative distance propagated
    real_type travelled_;
    //! Distance to try to travel in the next substep
    real_type trial_substep;
    //! Number of substeps until we declare "looping"
    auto remaining_substeps;

    FieldSubstepper(real_type step, GeoFieldState* gfstate)
        : propagation_step_{step}
        , travelled_{0}
        , trial_substep{propagation_step_}
        , remaining_substeps{FieldPropagatorOptions::max_substeps}
    {
    }

    Status status() const
    {
        if (trial_substep > this->minimum_substep() && remaining_substeps > 0)
            return Status::iterating;
        if (remaining_substeps == 0 && travelled_ < propagation_step_)
            return Status::looping;
        if (travelled_ > 0)
        {
            if (state_->boundary)
            {
                return Status::boundary;
            }
            return Status::moved_internal;
        }
        // No movement no matter the step size
        return Status::stuck;
    }

    void accept_internal(TrialSubstep const& trial)
    {
        // No boundary intersection along the chord: accept substep
        // movement inside the current volume and reset the remaining
        // distance so we can continue toward the next boundary or end of
        // caller-requested step. Reset the boundary flag to "false" only
        // in the unlikely case that we successfully shortened the substep
        // on a reentrant boundary crossing below.

        state_ = trial.end_state();
        boundary = false;
        travelled_ += trial.substep();
        trial_substep = propagation_step_ - travelled_;
        geo_.move_internal(state_.pos);
        --remaining_substeps;
    }

    bool accept_likely_boundary(TrialSubstep const& trial)
    {
        // Commit the proposed state's momentum, use the
        // post-boundary-crossing track position for consistency, and
        // conservatively reduce the *reported* traveled distance to avoid
        // coincident boundary crossings.

        // Only cross the boundary if at least one is true:
        // 1. the intersect point is less than or exactly on the substep end
        //    point, or
        // 2. crossing doesn't put us past the end of the remaining distance to
        //    be travelled_ (i.e. geo step truly is shorter than physics)
        // 3. the substep is effectively zero and we still "hit" because of the
        //    extra delta_intersection search length
        bool hit_boundary = trial.true_boundary()
                            || travelled_ + trial.scaled_substep() <= substep()
                            || trial.degenerate_chord();
        if (!hit_boundary)
        {
            state_.pos = trial.end_state().pos;
            geo_.move_internal(state_.pos);
        }

        // The update length can be slightly greater than the substep due
        // to the extra delta_intersection boost when searching. The
        // trial substep itself can be slightly more than the requested
        // substep.
        travelled_ += celeritas::min(trial.scaled_substep(), trial.substep());
        state_.mom = trial.end_state().mom;
        // Mark end of search
        trial_substep = 0;
    }

    void retry_stuck(TrialSubstep const& trial)
    {
        // Likely heading back into the old volume when starting on a
        // surface (this can happen when tracking through a volume at a
        // near tangent). Reduce substep size and try again.
        trial_substep = trial.substep() / 2;
    }

    void retry_hit(TrialSubstep const& trial)
    {
        CELER_ASSERT(trial.scaled_substep() < trial_substep);
        trial_substep = trial.scaled_substep();
    }

    void cross_boundary()
    {
        geo_.move_to_boundary();
        state_.pos = geo_.pos();
        state_.boundary = true;
    }

    void restore_direction()
    {
        // Even though the along-substep movement was through chord lengths,
        // conserve momentum through the field change by updating the final
        // *direction* based on the state's momentum.
        geo_.set_dir(make_unit_vector(state_.mom));
    }

    void bump()
    {
        // PRECONDITION: geo direction is momentum direction which is the
        // *original* direction
        // We failed to move at all, which means we hit a boundary no matter
        // what step length we took, which means we're stuck.
        // Using the just-reapplied direction, hope that we're pointing deeper
        // into the current volume and bump the particle.
        travelled_
            = celeritas::min(options_.bump_distance(), propagation_step_);
        axpy(travelled_, geo_.dir(), &state_.pos);
        geo_.move_internal(state_.pos);
        state_.boundary = false;
    }

    void fixup_internal_step()
    {
        if (CELER_UNLIKELY(travelled_ < propagation_step_))
        {
            // TODO: this is more likely to happen due to the 'minimum substep'
            // cutoff

            // Even though the track traveled the full step length, the
            // distance might be slightly less than the step due to roundoff
            // error (TODO: OR ENDING BEFORE THE LAST 'too small'
            // SUBSTEP). Reset the distance so the track's action isn't
            // erroneously set as propagation-limited.
            CELER_ASSERT(soft_equal(travelled_, propagation_step_));
            travelled_ = step;
        }
    }
};

// Find the next step
struct NextStepFinder
{
    Propagation operator()(Chord const& chord);
    {
        // Do a detailed check boundary check from the start position toward
        // the substep end point. Travel to the end of the chord, plus a little
        // extra.
        if (chord.length >= this->minimum_substep())
        {
            // Only update the direction if the chord length is nontrivial.
            // This is usually the case but might be skipped in two cases:
            // - if the initial step is very small compared to the
            //   magnitude of the position (which can result in a zero length
            //   for the chord and NaNs for the direction)
            // - in a high-curvature track where the trial distance is just
            //   barely above the minimum step (in which case our
            //   boundary test does lose some accuracy)
            geo_.set_dir(chord.dir);
        }

        return geo_.find_next_step(chord.length
                                   + options_.delta_intersection());
    }
};

// Find next step, using safety to skip unneeded distance calls
struct NextStepSafetyFinder
{
    Propagation operator()(Chord const& chord)
    {
        Propagation result;
        safety -= (chord.length + options_.delta_intersection());
        if (safety < 0 && !geo_.is_on_boundary())
        {
            // Calculate the nearest boundary distance, just past the possible
            // intersection length
            safety = geo_.find_safety(chord.length
                                      + 2 * options_.delta_intersection())
                     - search_dist;
        }

        if (safety > 0)
        {
            boundary = false;
        }
        else
        {
            // XXX we might not have updated the geo direction after several
            // "in safety" substeps, so we *have* to update it here.
            CELER_ASSERT(chord.length > 0);
            geo_.set_dir(chord.dir);
            result = geo_.find_next_step(chord.length
                                         + options_.delta_intersection());
        }
        return result;
    }

    real_type safety{0};
};

//---------------------------------------------------------------------------//
/*!
 * Manage temporary data during field propagation.
 *
 * Optional detailed class description, and possibly example usage:
 * \code
    FieldSubstepper ...;
   \endcode
 */
template<class DriverT, class GTV>
class FieldSubstepper
{
  public:
    //!@{
    //! \name Type aliases

    //!@}

  public:
    // Construct with defaults
    inline FieldSubstepper();

  private:
};

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Construct with defaults.
 */
FieldSubstepper::FieldSubstepper() {}

//---------------------------------------------------------------------------//
}  // namespace celeritas

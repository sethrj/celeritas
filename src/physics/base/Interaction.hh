//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file Interaction.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Array.hh"
#include "base/Span.hh"
#include "base/Types.hh"
#include "sim/Action.hh"
#include "Secondary.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Change in state due to an interaction.
 *
 * The interaction results in an "empty" secondary by default (i.e. no
 * secondaries produced). The first secondary must be saved to the `secondary`
 * field; additional secondaries should be allocated and saved to
 * `secondaries`.
 */
struct Interaction
{
    Action           action;            //!< Failure, scatter, absorption, ...
    units::MevEnergy energy;            //!< Post-interaction energy
    Real3            direction;         //!< Post-interaction direction
    Secondary        secondary;         //!< First emitted secondary
    Span<Secondary>  secondaries;       //!< Additional emitted secondaries
    units::MevEnergy energy_deposition; //!< Energy loss locally to material

    // Return an interaction representing a recoverable error
    static inline CELER_FUNCTION Interaction from_failure();

    // Return an interaction representing an absorbed process
    static inline CELER_FUNCTION Interaction from_absorption();

    // Whether the interaction succeeded
    explicit inline CELER_FUNCTION operator bool() const;

    // Number of secondaries produced
    inline CELER_FUNCTION size_type num_secondaries() const;
};

//---------------------------------------------------------------------------//
// INLINE FUNCTIONS
//---------------------------------------------------------------------------//
/*!
 * Construct with defaults.
 */
CELER_FUNCTION Interaction Interaction::from_failure()
{
    Interaction result;
    result.action = Action::failed;
    return result;
}

//---------------------------------------------------------------------------//
/*!
 * Construct an interaction from a particle that was totally absorbed.
 */
CELER_FUNCTION Interaction Interaction::from_absorption()
{
    Interaction result;
    result.action    = Action::absorbed;
    result.energy    = zero_quantity();
    result.direction = {0, 0, 0};
    return result;
}

//---------------------------------------------------------------------------//
/*!
 * Whether the interaction succeeded.
 */
CELER_FUNCTION Interaction::operator bool() const
{
    return action_completed(this->action);
}

//---------------------------------------------------------------------------//
/*!
 * Number of secondaries produced.
 */
CELER_FUNCTION size_type Interaction::num_secondaries() const
{
    if (!this->secondary)
    {
        CELER_ENSURE(this->secondaries.empty());
        return 0;
    }

    return 1u + this->secondaries.size();
}

//---------------------------------------------------------------------------//
} // namespace celeritas

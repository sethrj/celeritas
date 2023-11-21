//----------------------------------*-C++-*----------------------------------//
// Copyright 2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/field/FieldPropagatorData.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Types.hh"

#include "FieldDriverOptions.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Result of the substep iteration.
 */
struct FieldPropagatorOptions
{
    // TODO: some propagator options are in driver
    FieldDriverOptions const& driver_options;

    //! Limit on substeps
    static constexpr short int max_substeps = 100;

    //! Distance close enough to a boundary to mark as being on the boundary
    CELER_FORCEINLINE_FUNCTION real_type delta_intersection() const
    {
        return driver_options.delta_intersection;
    }

    //! Distance to bump or to consider a "zero" movement
    CELER_FORCEINLINE_FUNCTION real_type bump_distance() const
    {
        return this->delta_intersection() * real_type(0.1);
    }

    //! Smallest allowable inner loop distance to take
    CELER_FORCEINLINE_FUNCTION real_type minimum_substep() const
    {
        return driver_options.minimum_step;
    }
};

//---------------------------------------------------------------------------//
}  // namespace celeritas

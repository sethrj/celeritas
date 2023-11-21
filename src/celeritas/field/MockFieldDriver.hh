//----------------------------------*-C++-*----------------------------------//
// Copyright 2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/field/MockFieldDriver.hh
//---------------------------------------------------------------------------//
#pragma once

#include "FieldDriverOptions.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Mock field driver.
 *
 * This declaration-only class can be used with the field propagator to
 * generate assembly output for instructive purposes.
 *
 * TODO: rename driver -> substepper
 * TODO: rename advance -> operator()
 */
class MockFieldDriver
{
  public:
    //! Construct with options
    explicit CELER_FUNCTION MockFieldDriver(FieldDriverOptions const& options)
        : options_{options}
    {
    }

    //! For a given trial step, advance by a sub_step within a tolerance error
    CELER_FUNCTION DriverResult advance(real_type step, OdeState const& state);

  private:
    //// DATA ////

    // Driver configuration
    FieldDriverOptions const& options_;
};

//---------------------------------------------------------------------------//
}  // namespace celeritas

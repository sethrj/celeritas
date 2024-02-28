//----------------------------------*-C++-*----------------------------------//
// Copyright 2022-2024 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/phys/PrimaryGeneratorOptions.cc
//---------------------------------------------------------------------------//
#include "PrimaryGeneratorOptions.hh"

#include "corecel/io/EnumStringMapper.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Get a string corresponding to the distribution type.
 */
char const* to_cstring(DistributionSelection value)
{
    static EnumStringMapper<DistributionSelection> const to_cstring_impl{
        "delta",
        "isotropic",
        "box",
    };
    return to_cstring_impl(value);
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

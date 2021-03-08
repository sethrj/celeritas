//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file ImportFile.hh
//---------------------------------------------------------------------------//
#pragma once

#include <vector>
#include "ImportMaterial.hh"
#include "ImportParticle.hh"
#include "ImportProcess.hh"
#include "ImportVolume.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Collection of data that defines a problem input.
 */
struct ImportFile
{
    std::vector<ImportElement>  elements;
    std::vector<ImportMaterial> materials;
    std::vector<ImportParticle> particles;
    std::vector<ImportVolume>   geometry;
    std::vector<ImportProcess>  processes;
    // TODO: cutoffs
};

//---------------------------------------------------------------------------//
} // namespace celeritas

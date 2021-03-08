//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file ImportMaterial.hh
//---------------------------------------------------------------------------//
#pragma once

#include <string>
#include <vector>

#include "ImportElement.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Enum for storing G4State enumerators.
 * [See G4Material.hh]
 */
enum class ImportMaterialState
{
    not_defined,
    solid,
    liquid,
    gas
};

//---------------------------------------------------------------------------//
/*!
 * Store data of a given material and its elements.
 *
 * Used by the GdmlGeometryMap class.
 *
 * The data is exported via the app/geant-exporter. For further expanding
 * this struct, add the appropriate variables here and fetch the new values in
 * \c app/geant-exporter.cc : store_geometry(...).
 *
 * Units are defined at export time in the aforementioned function.
 */
struct ImportMaterial
{
    using elem_id = unsigned int;

    std::string                  name;
    ImportMaterialState          state;
    double                       temperature;        // [K]
    double                       density;            // [g/cm^3]
    double                       electron_density;   // [1/cm^3]
    double                       number_density;     // [1/cm^3]
    double                       radiation_length;   // [cm]
    double                       nuclear_int_length; // [cm]
    // TODO: use vector of ImportMatElementComponent below
    std::map<elem_id, double> elements_fractions;     // Mass fractions
    std::map<elem_id, double> elements_num_fractions; // Number fractions
};

//---------------------------------------------------------------------------//
} // namespace celeritas

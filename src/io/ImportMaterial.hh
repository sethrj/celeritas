//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file ImportMaterial.hh
//---------------------------------------------------------------------------//
#pragma once

#include <map>
#include <string>
#include <vector>

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
 * Store element data.
 *
 * Used by ImportMaterial and GdmlGeometryMap.
 *
 * The data is exported via the app/geant-exporter. For further expanding this
 * struct, add the appropriate variables here and fetch the new values in
 * \c app/geant-exporter.cc : store_geometry(...).
 *
 * Units are defined at export time in the aforementioned function.
 */
struct ImportElement
{
    std::string name;
    int         atomic_number;
    double      atomic_mass;           // [atomic mass unit]
    double      radiation_length_tsai; // [g/cm^2]
    double      coulomb_factor;
};

//---------------------------------------------------------------------------//
/*!
 * Components of an element.
 *
 * The atom and mass fractions should be redundant.
 *
 * \todo CURRENTLY UNUSED: replace elements_fractions and
 * elements_num_fractions below.
 */
struct ImportMatElementComponent
{
    unsigned int element_id;
    double       mass_frac;
    double       atom_frac;
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

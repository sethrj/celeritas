//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file geocel/VolumeParamsIO.json.hh
//---------------------------------------------------------------------------//
#pragma once

#include <iosfwd>
#include <nlohmann/json.hpp>

#include "VolumeParams.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
// Read volume hierarchy from JSON
void from_json(nlohmann::json const& j, VolumeParams& vp);

// Write volume hierarchy to JSON
void to_json(nlohmann::json& j, VolumeParams const& vp);

// Read volume hierarchy from a stream
std::istream& operator>>(std::istream& is, VolumeParams& vp);

// Write volume hierarchy to a stream
std::ostream& operator<<(std::ostream& os, VolumeParams const& vp);

//---------------------------------------------------------------------------//
}  // namespace celeritas

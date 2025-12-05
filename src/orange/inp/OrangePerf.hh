//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/inp/OrangePerf.hh
//---------------------------------------------------------------------------//
#pragma once

#include <iosfwd>

namespace celeritas
{
namespace inp
{
//---------------------------------------------------------------------------//
/*!
 * Performance knobs for runtime setup, including BIH construction.
 */
struct OrangePerf
{
    //! Maximum number of intersections per step (zero for unlimited)
    unsigned int max_intersect{};
};

// Helper to read from a file or stream
std::istream& operator>>(std::istream& is, OrangePerf&);
// Helper to write to a file or stream
std::ostream& operator<<(std::ostream& os, OrangePerf const&);

//---------------------------------------------------------------------------//
}  // namespace inp
}  // namespace celeritas

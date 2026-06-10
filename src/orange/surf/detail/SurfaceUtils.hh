//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/surf/detail/SurfaceUtils.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/cont/Array.hh"
#include "corecel/math/Algorithms.hh"
#include "orange/OrangeTypes.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//

//! Intersection array from a plane
using PlaneIntersections = Array<real_type, 1>;

/*!
 * Calculate all possible straight-line intersections with this surface.
 */
inline CELER_FUNCTION PlaneIntersections
calc_plane_intersections(real_type n_pos,
                         real_type n_dir,
                         real_type displacement,
                         SurfaceState on_surface)
{
    CELER_EXPECT(std::fabs(n_dir) <= 1.000001);
    real_type const dist = (displacement - n_pos) / n_dir;

    bool valid = celeritas::logical_all(
        (on_surface == SurfaceState::off), (n_dir != 0), (dist > 0));

    return {valid ? dist : no_intersection()};
}

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

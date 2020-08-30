//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file ImageInterface.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Span.hh"
#include "base/Types.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construction arguments for an on-device image view.
 */
struct ImageInterface
{
    Real3     origin;   //!< Upper left corner
    Real3     down_ax;  //!< Downward axis (increasing j, track initialization)
    Real3     right_ax; //!< Rightward axis (increasing i, track movement)
    real_type pixel_width; //!< Width of a pixel
    array<int, 2> dims;    //!< Image dimensions (j, i)
    span<int>     image;   //!< Stored image [j][i]
};

//---------------------------------------------------------------------------//
} // namespace celeritas

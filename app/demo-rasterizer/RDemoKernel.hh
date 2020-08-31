//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file RDemoKernel.hh
//---------------------------------------------------------------------------//
#pragma once

#include "geometry/GeoParamsPointers.hh"
#include "geometry/GeoStatePointers.hh"
#include "ImageInterface.hh"

namespace demo_rasterizer
{
//---------------------------------------------------------------------------//

void trace(const GeoParamsPointers& geo_params,
           const GeoStatePointers&  geo_state,
           const ImageInterface&    image);

//---------------------------------------------------------------------------//
} // namespace demo_rasterizer

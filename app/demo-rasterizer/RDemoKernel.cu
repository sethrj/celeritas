//---------------------------------*-CUDA-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file RDemoKernel.cu
//---------------------------------------------------------------------------//
#include "RDemoKernel.cuh"

#include "base/Assert.cuda.hh"
#include "base/KernelParamCalculator.cuda.hh"
#include "geometry/GeoTrackView.hh"
#include "ImageTrackView.hh"

using namespace celeritas;
using namespace demo_rasterizer;

namespace
{
__device__ int geo_id(const GeoTrackView& geo)
{
    if (geo.boundary() == Boundary::outside)
        return -1;
    return geo.volume_id().get();
}

__global__ void trace_impl(const GeoParamsPointers geo_params,
                           const GeoStatePointers  geo_state,
                           const ImageInterface    image_state)
{
    auto tid = celeritas::KernelParamCalculator::thread_id();
    if (tid.get() >= size)
        return;

    ImageTrackView image(image_state, tid);
    GeoTrackView   geo(geo_params, geo_state, tid);

    int cur_id = geo_id(geo);
    geo.find_next_step();
    real_type geo_dist = geo.next_step();

    // Track along each pixel
    for (unsigned int i = 0; i < image_state.dims()[1]; ++i)
    {
        real_type pix_dist = image_state.pixel_width();
        while (geo_dist <= pix_dist)
        {
            // Move to geometry boundary
            pix_dist -= geo_dist;

            // Cross surface
            geo.move_next_step();
            cur_id = geo_id(geo);
            geo.find_next_step();
            geo_dist = geo.next_step();
        }

        // Move to pixel boundary
        geo_dist -= pix_dist;
        image.set_pixel(i, cur_id);
    }
}
} // namespace

namespace demo_rasterizer
{
//---------------------------------------------------------------------------//
void trace(const GeoParamsPointers& geo_params,
           const GeoStatePointers&  geo_state,
           const ImageInterface&    image)
{
    REQUIRE(image);

    KernelParamCalculator calc_kernel_params;
    auto                  params = calc_kernel_params(states.size());
    trace_impl(geo_params, geo_state, image);
    CELER_CUDA_CALL(cudaDeviceSynchronize());
}

//---------------------------------------------------------------------------//
} // namespace demo_rasterizer

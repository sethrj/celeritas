//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file ImageView.i.hh
//---------------------------------------------------------------------------//

#include "base/Assert.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct with defaults.
 */
ImageView::ImageView(const ImageInterface& shared, ThreadId tid)
    : shared_(shared), j_index_(tid.get())
{
    REQUIRE(j_index_ < shared_.dims[0]);
}

//---------------------------------------------------------------------------//
/*!
 * Calculate starting position.
 */
CELER_FUNCTION Real3 ImageView::start_pos() const
{
    Real3 result;
    for (int i = 0; i < 3; ++i)
    {
        result[i] = shared_.origin[i] + shared_.down_ax[i] * j_index_;
    }
    return result;
}

//---------------------------------------------------------------------------//
/*!
 * Set the value for a pixel.
 */
CELER_FUNCTION void ImageView::set_pixel(unsigned int i, int value)
{
    REQUIRE(i >= 0 && i < shared_.dims[1]);
    unsigned int idx = j_index_ * shared_.dims[1] + i;

    CHECK(idx < shared_.image.size());
    shared_.image[idx] = value;
}

//---------------------------------------------------------------------------//
} // namespace celeritas

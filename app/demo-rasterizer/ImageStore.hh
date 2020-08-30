//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file ImageStore.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Array.hh"
#include "base/DeviceVector.hh"
#include "base/Span.hh"
#include "base/Types.hh"
#include "ImageInterface.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Initialization and storage for a raster image.
 */
class ImageStore
{
  public:
    //@{
    //! Type aliases
    using Int2    = array<int, 2>;
    using SpanInt = span<int>;
    //@}

    //! Construction arguments
    struct Params
    {
        Real3 lower_left;
        Real3 upper_right;
        Real3 rightward_ax;
        int   vertical_pixels;
    };

  public:
    // Construct with defaults
    explicit ImageStore(Params);

    //! Access image on device for writing
    ImageInterface device_interface();

    // >>> HOST ACCESSORS

    //! Upper left corner of the image
    const Real3& origin() const { return origin_; }

    //! Downward axis (increasing j)
    const Real3& down_ax() const { return down_ax_; }

    //! Rightward axis (increasing i)
    const Real3& right_ax() const { return right_ax_; }

    //! Width of a pixel
    real_type pixel_width() const { return pixel_width_; }

    //! Dimensions {j, i} of the image
    const Int2& dims() const { return dims_; }

    // Copy out the image to the host
    void image_to_host(SpanInt host_data) const;

  private:
    Real3             origin_;
    Real3             down_ax_;
    Real3             right_ax_;
    real_type         pixel_width_;
    Int2              dims_;
    DeviceVector<int> image_;
};

//---------------------------------------------------------------------------//
} // namespace celeritas

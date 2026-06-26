//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/surf/PlaneAligned.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Macros.hh"
#include "corecel/Types.hh"
#include "corecel/cont/Array.hh"
#include "corecel/cont/Span.hh"
#include "geocel/Types.hh"
#include "orange/OrangeTypes.hh"
#include "orange/SenseUtils.hh"

#include "detail/SurfaceUtils.hh"

namespace celeritas
{

//---------------------------------------------------------------------------//
/*!
 * Axis-aligned plane with positive-facing normal.
 */
class PlaneAlignedBase
{
  public:
    //@{
    //! \name Type aliases
    using Intersections = detail::PlaneIntersections;
    using StorageSpan = Span<real_type const, 1>;
    //@}

  public:
    //// CONSTRUCTORS ////

    // Construct from axis intercept
    explicit inline CELER_FUNCTION PlaneAlignedBase(real_type position);

    // Construct from raw data
    template<class R>
    explicit inline CELER_FUNCTION
    PlaneAlignedBase(Span<R, StorageSpan::extent> data)
        : d_{data[0]}
    {
    }

    //! Get a view to the data for type-deleted storage
    CELER_FUNCTION StorageSpan data() const { return {&d_, 1}; }

    //// ACCESSORS ////

    //! Distance from the origin along the normal to the plane
    CELER_CONSTEXPR_FUNCTION real_type displacement() const { return d_; }

    // Get the dot product with the normal
    CELER_CONSTEXPR_FUNCTION real_type dot_normal(Axis ax,
                                                  Real3 const& pos) const
    {
        return pos[to_int(ax)];
    }

  private:
    //! Intersection with the axis
    real_type d_;
};

//---------------------------------------------------------------------------//
/*!
 * Axis-aligned plane with positive-facing normal.
 */
template<Axis T>
class PlaneAligned : public PlaneAlignedBase
{
    static_assert(T != Axis::size_);

  public:
    //@{
    //! \name Type aliases
    using Intersections = detail::PlaneIntersections;
    using StorageSpan = Span<real_type const, 1>;
    //@}

    //// CLASS ATTRIBUTES ////

    // Surface type identifier
    static CELER_CONSTEXPR_FUNCTION SurfaceType surface_type();

    //! Safety is intersection along surface normal
    static CELER_CONSTEXPR_FUNCTION bool simple_safety() { return true; }

  public:
    //// CONSTRUCTORS ////

    // Construct from axis intercept
    explicit inline CELER_FUNCTION PlaneAligned(real_type position)
        : PlaneAlignedBase{position}
    {
    }

    // Construct from raw data
    template<class R>
    explicit inline CELER_FUNCTION PlaneAligned(Span<R, StorageSpan::extent> s)
        : PlaneAlignedBase{s}
    {
    }

    //// ACCESSORS ////

    //! Distance from the origin along the normal to the plane
    using PlaneAlignedBase::displacement;

    //! Get a view to the data for type-deleted storage
    using PlaneAlignedBase::data;

    // Construct outward normal vector
    inline CELER_FUNCTION Real3 normal() const;

    //// CALCULATION ////

    // Get the dot product with the normal
    CELER_CONSTEXPR_FUNCTION real_type dot_normal(Real3 const& pos) const;

    // Determine the sense of the position relative to this surface
    CELER_CONSTEXPR_FUNCTION SignedSense calc_sense(Real3 const& pos) const;

    // Calculate all possible straight-line intersections with this surface
    CELER_CONSTEXPR_FUNCTION Intersections calc_intersections(
        Real3 const& pos, Real3 const& dir, SurfaceState on_surface) const;

    // Calculate outward normal at a position
    inline CELER_FUNCTION Real3 calc_normal(Real3 const& pos) const;

  private:
    //! Intersection with the axis
    real_type d_;
};

//---------------------------------------------------------------------------//
// TYPE ALIASES
//---------------------------------------------------------------------------//

using PlaneX = PlaneAligned<Axis::x>;
using PlaneY = PlaneAligned<Axis::y>;
using PlaneZ = PlaneAligned<Axis::z>;

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Surface type identifier.
 */
template<Axis T>
CELER_CONSTEXPR_FUNCTION SurfaceType PlaneAligned<T>::surface_type()
{
    return static_cast<SurfaceType>(static_cast<int>(SurfaceType::px)
                                    + to_int(T));
}

//---------------------------------------------------------------------------//
/*!
 * Calculate outward normal.
 */
template<Axis T>
CELER_FUNCTION Real3 PlaneAligned<T>::normal() const
{
    Real3 norm{0, 0, 0};

    norm[to_int(T)] = 1.;
    return norm;
}

//---------------------------------------------------------------------------//
/*!
 * Get the dot product with the normal.
 *
 * This is used for intersection, sense, and combined-plane methods.
 */
template<Axis T>
CELER_CONSTEXPR_FUNCTION real_type PlaneAligned<T>::dot_normal(Real3 const& x) const
{
    return PlaneAlignedBase::dot_normal(T, x);
}

//---------------------------------------------------------------------------//
/*!
 * Determine the sense of the position relative to this surface.
 */
template<Axis T>
CELER_CONSTEXPR_FUNCTION SignedSense
PlaneAligned<T>::calc_sense(Real3 const& pos) const
{
    return real_to_sense(this->dot_normal(pos) - this->displacement());
}

//---------------------------------------------------------------------------//
/*!
 * Calculate all possible straight-line intersections with this surface.
 */
template<Axis T>
CELER_CONSTEXPR_FUNCTION auto
PlaneAligned<T>::calc_intersections(Real3 const& pos,
                                    Real3 const& dir,
                                    SurfaceState on_surface) const
    -> Intersections
{
    return detail::calc_plane_intersections(this->dot_normal(pos),
                                            this->dot_normal(dir),
                                            this->displacement(),
                                            on_surface);
}

//---------------------------------------------------------------------------//
/*!
 * Calculate outward normal at a position on the surface.
 */
template<Axis T>
CELER_FORCEINLINE_FUNCTION Real3 PlaneAligned<T>::calc_normal(Real3 const&) const
{
    return this->normal();
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

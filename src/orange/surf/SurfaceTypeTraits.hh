//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/surf/SurfaceTypeTraits.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/cont/EnumClassUtils.hh"
#include "corecel/math/Algorithms.hh"
#include "orange/OrangeTypes.hh"

#include "SurfaceFwd.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Map surface enumeration to surface type.
 *
 * This class can be passed as a "tag" to functors that can then retrieve its
 * value or the associated class. It can be implicitly converted into a
 * SurfaceType enum for use in template parameters.
 */
template<SurfaceType S>
struct SurfaceTypeTraits;

#define ORANGE_SURFACE_TRAITS(ENUM_VALUE, CLS)                          \
    template<>                                                          \
    struct SurfaceTypeTraits<SurfaceType::ENUM_VALUE>                   \
        : public EnumToClass<SurfaceType, SurfaceType::ENUM_VALUE, CLS> \
    {                                                                   \
    }

// clang-format off
ORANGE_SURFACE_TRAITS(px,  PlaneAligned<Axis::x>);
ORANGE_SURFACE_TRAITS(py,  PlaneAligned<Axis::y>);
ORANGE_SURFACE_TRAITS(pz,  PlaneAligned<Axis::z>);
ORANGE_SURFACE_TRAITS(p,   Plane);
ORANGE_SURFACE_TRAITS(sc,  SphereCentered);
ORANGE_SURFACE_TRAITS(s,   Sphere);
ORANGE_SURFACE_TRAITS(cxc, CylCentered<Axis::x>);
ORANGE_SURFACE_TRAITS(cyc, CylCentered<Axis::y>);
ORANGE_SURFACE_TRAITS(czc, CylCentered<Axis::z>);
ORANGE_SURFACE_TRAITS(cx,  CylAligned<Axis::x>);
ORANGE_SURFACE_TRAITS(cy,  CylAligned<Axis::y>);
ORANGE_SURFACE_TRAITS(cz,  CylAligned<Axis::z>);
ORANGE_SURFACE_TRAITS(kx,  ConeAligned<Axis::x>);
ORANGE_SURFACE_TRAITS(ky,  ConeAligned<Axis::y>);
ORANGE_SURFACE_TRAITS(kz,  ConeAligned<Axis::z>);
ORANGE_SURFACE_TRAITS(sq,  SimpleQuadric);
ORANGE_SURFACE_TRAITS(gq,  GeneralQuadric);
ORANGE_SURFACE_TRAITS(inv, Involute);
// clang-format on

#undef ORANGE_SURFACE_TRAITS

#define ORANGE_ST_VISIT_CASE(F, FUNC, TYPE) \
    case SurfaceType::TYPE:                 \
        return celeritas::forward<F>(FUNC)( \
            SurfaceTypeTraits<SurfaceType::TYPE>{})

//---------------------------------------------------------------------------//
/*!
 * Expand a macro to a switch statement over all possible surface types.
 *
 * The \c func argument should be a functor that takes a single argument which
 * is a SurfaceTypeTraits instance.
 */
template<class F>
CELER_CONSTEXPR_FUNCTION decltype(auto)
visit_surface_type(F&& func, SurfaceType st)
{
    switch (st)
    {
        ORANGE_ST_VISIT_CASE(F, func, px);
        ORANGE_ST_VISIT_CASE(F, func, py);
        ORANGE_ST_VISIT_CASE(F, func, pz);
        ORANGE_ST_VISIT_CASE(F, func, p);
        ORANGE_ST_VISIT_CASE(F, func, sc);
        ORANGE_ST_VISIT_CASE(F, func, s);
        ORANGE_ST_VISIT_CASE(F, func, cxc);
        ORANGE_ST_VISIT_CASE(F, func, cyc);
        ORANGE_ST_VISIT_CASE(F, func, czc);
        ORANGE_ST_VISIT_CASE(F, func, cx);
        ORANGE_ST_VISIT_CASE(F, func, cy);
        ORANGE_ST_VISIT_CASE(F, func, cz);
        ORANGE_ST_VISIT_CASE(F, func, kx);
        ORANGE_ST_VISIT_CASE(F, func, ky);
        ORANGE_ST_VISIT_CASE(F, func, kz);
        ORANGE_ST_VISIT_CASE(F, func, sq);
        ORANGE_ST_VISIT_CASE(F, func, gq);
        case SurfaceType::inv:
            // Prevented by input reader: see
            // orange/detail/SurfacesRecordBuilder.cc
            CELER_ASSERT_UNREACHABLE();
        default:
            CELER_ASSERT_UNREACHABLE();
    }
}

//---------------------------------------------------------------------------//
/*!
 * Expand a macro to a switch statement over all possible surface types.
 *
 * The \c func argument should be a functor that takes a single argument which
 * is a SurfaceTypeTraits instance.
 */
template<class F>
CELER_CONSTEXPR_FUNCTION decltype(auto)
visit_planar_surface_type(F&& func, SurfaceType st)
{
    switch (st)
    {
        ORANGE_ST_VISIT_CASE(F, func, px);
        ORANGE_ST_VISIT_CASE(F, func, py);
        ORANGE_ST_VISIT_CASE(F, func, pz);
        ORANGE_ST_VISIT_CASE(F, func, p);
        default:
            CELER_ASSERT_UNREACHABLE();
    }
}

#undef ORANGE_ST_VISIT_CASE

//---------------------------------------------------------------------------//
}  // namespace celeritas

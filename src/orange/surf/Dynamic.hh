//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/surf/Dynamic.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/cont/Array.hh"
#include "corecel/cont/LdgSpan.hh"
#include "orange/OrangeTypes.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Provide type-deleted access to planar data.
 */
class DynamicPlane
{
  public:
    //@{
    //! \name Type aliases
    using Intersections = Array<real_type, 1>;
    using StorageSpan = LdgSpan<real_type const>;  //!< Dynamic storage
    //@}

    //! Safety is intersection along surface normal
    static CELER_CONSTEXPR_FUNCTION bool simple_safety() { return true; }

    // Surface type identifier
    CELER_CONSTEXPR_FUNCTION SurfaceType surface_type() const { return type_; }

  public:
    //// CONSTRUCTORS ////

    // Construct from type and data
    CELER_CONSTEXPR_FUNCTION DynamicPlane(SurfaceType st, StorageSpan data)
        : type_{st}, data_{data}
    {
    }

    //! Construct surface
    template<class T>
    CELER_FUNCTION T surface() const
    {
        CELER_ASSUME(T::surface_type() == type_);
        constexpr size_type size{T::StorageSpan::extent};

        CELER_ASSUME(data_.size() == size);
        using LdgSpanT = LdgSpan<real_type const, size>;
        return T{LdgSpanT{data_}};
    }

  private:
    SurfaceType type_;
    StorageSpan data_;
};

//---------------------------------------------------------------------------//
}  // namespace celeritas

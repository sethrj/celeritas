//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/univ/detail/CompressedFaceVisitor.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Types.hh"
#include "orange/OrangeData.hh"
#include "orange/surf/SurfaceTypeTraits.hh"
#include "orange/surf/detail/AllSurfaces.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Apply a functor to all type-deleted local compressed surfaces.
 *
 * Example: \code
 CompressedFaceVisitor visit_all_faces{params_, vol.};
 visit_all_faces( [&](auto const& s) { calc_intersection(s)});
 \endcode
 */
class CompressedFaceVisitor
{
  public:
    //!@{
    //! \name Type aliases
    using ParamsRef = NativeCRef<OrangeParamsData>;
    //!@}

  public:
    // Construct from ORANGE params and volume faces
    inline CELER_FUNCTION
    CompressedFaceVisitor(ParamsRef const& params,
                          CompressedFacesRecord const& faces);

    // Apply the function to all faces
    template<class F>
    inline CELER_FUNCTION void operator()(F&& typed_visitor) const;

  private:
    //// TYPES ////

    template<class T>
    using Items = Collection<T, Ownership::const_reference, MemSpace::native>;
    using SpanSurfaceType = LdgSpan<SurfaceType const>;
    using SpanReal = LdgSpan<real_type const>;

    //// DATA ////

    SpanSurfaceType surface_types_;
    SpanReal reals_;

    //// HELPER FUNCTIONS ////

    // Load data for a surface
    inline CELER_FUNCTION SpanReal surface_data(LocalSurfaceId id,
                                                size_type size) const;

    // Construct a surface from a data offset
    template<class T>
    inline CELER_FUNCTION T make_surface(SpanReal data) const;
};

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Construct from ORANGE data and local faces.
 */
CELER_FORCEINLINE_FUNCTION
CompressedFaceVisitor::CompressedFaceVisitor(
    ParamsRef const& params, CompressedFacesRecord const& local_surfaces)
    : surface_types_{params.surface_types[local_surfaces.types]}
    , reals_{params.reals[local_surfaces.reals]}
{
}

#if !defined(__DOXYGEN__) || __DOXYGEN__ > 0x010908
//---------------------------------------------------------------------------//
/*!
 * Apply the function to all faces.
 */
template<class F>
CELER_FUNCTION void CompressedFaceVisitor::operator()(F&& func) const
{
    size_type data_offset{0};
    auto surface_visitor = [&](auto s_traits) -> size_type {
        using S = typename decltype(s_traits)::type;
        constexpr auto size = S::StorageSpan::extent;
        using LdgSpanT = LdgSpan<real_type const, size>;

        CELER_ASSERT(data_offset + size <= reals_.size());
        auto data = reals_.subspan(data_offset, size);
        func(S{LdgSpanT{data}});
        return size;
    };

    for (size_type i : range(surface_types_.size()))
    {
        data_offset += visit_surface_type(surface_visitor, surface_types_[i]);
    }
    CELER_ENSURE(data_offset == reals_.size());
}
#endif

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

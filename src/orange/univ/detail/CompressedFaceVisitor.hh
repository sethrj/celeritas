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
 CompressedFaceVisitor visit_faces{params_, vol.compressed_faces()};
 visit_faces([&](auto const& s) { calc_intersection(s)});
 visit_faces(calc_intersection, FaceId{1});
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
    inline CELER_FUNCTION void operator()(F&& func) const;

    // Visit a single face (less efficient for multiple lookups)
    template<class F>
    inline CELER_FUNCTION decltype(auto) operator()(F&& func, FaceId) const;

  private:
    //// TYPES ////

    using SpanSurfaceType = LdgSpan<SurfaceType const>;
    using SpanReal = LdgSpan<real_type const>;
    using SpanSize = LdgSpan<size_type const>;

    //// DATA ////

    SpanSurfaceType surface_types_;
    SpanReal reals_;
    SpanSize offsets_;

    //// HELPER FUNCTIONS ////

    template<class T>
    inline CELER_FUNCTION T make_surface(size_type data_offset) const;
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
    , offsets_{params.sizes[local_surfaces.offsets]}
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
        func(this->make_surface<S>(data_offset));
        return S::StorageSpan::extent;
    };

    for (size_type i : range(surface_types_.size()))
    {
        data_offset += visit_surface_type(surface_visitor, surface_types_[i]);
    }
    CELER_ENSURE(data_offset == reals_.size());
}

//---------------------------------------------------------------------------//
/*!
 * Apply a function to a single face.
 *
 * This is the same signature as \c LocalSurfaceVisitor .
 */
template<class F>
CELER_FUNCTION decltype(auto)
CompressedFaceVisitor::operator()(F&& func, FaceId fid) const
{
    CELER_EXPECT(fid < surface_types_.size());

    auto st = surface_types_[*fid];
    auto data_offset = offsets_[*fid];

    return visit_surface_type(
        [&](auto s_traits) {
            using S = typename decltype(s_traits)::type;
            return func(this->make_surface<S>(data_offset));
        },
        st);
}

#endif

//---------------------------------------------------------------------------//
/*!
 * Create the surface at this offset.
 */
template<class T>
CELER_FUNCTION T CompressedFaceVisitor::make_surface(size_type data_offset) const
{
    constexpr auto size{T::StorageSpan::extent};
    using LdgSpanT = LdgSpan<real_type const, size>;

    CELER_ASSERT(data_offset + size <= reals_.size());
    auto data = reals_.subspan(data_offset, size);
    return T{LdgSpanT{data}};
}

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/univ/detail/CompressedFaceVisitor.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Macros.hh"
#include "corecel/Types.hh"
#include "orange/OrangeData.hh"
#include "orange/OrangeTypes.hh"
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

    using RealId = ItemId<real_type>;

    //// DATA ////
    ParamsRef const& params_;
    CompressedFacesRecord const& faces_;
    SurfaceType const* types_;
    real_type const* reals_;

    //// HELPER FUNCTIONS ////

    inline CELER_FUNCTION SurfaceType surface_type(FaceId) const;

    template<class T>
    inline CELER_FUNCTION T make_surface(size_type offset) const;
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
    : params_{params}
    , faces_{local_surfaces}
    , types_{params_.surface_types.data().get() + **faces_.types.begin()}
    , reals_{params_.reals.data().get() + **faces_.reals.begin()}
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
    size_type offset{0};
    auto surface_visitor = [&](auto s_traits) -> size_type {
        using S = typename decltype(s_traits)::type;
        func(this->make_surface<S>(offset));
        return S::StorageSpan::extent;
    };

    for (MakeSize_t<FaceId> face_idx = 0, end_idx = faces_.size();
         face_idx != end_idx;
         ++face_idx)
    {
        offset += visit_surface_type(surface_visitor,
                                     this->surface_type(FaceId{face_idx}));
    }
    // CELER_ENSURE(data_begin == **faces_.reals.end());
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
    CELER_EXPECT(fid < faces_.size());

    size_type offset = params_.sizes[faces_.offsets[fid]];

    return visit_surface_type(
        [&](auto s_traits) {
            using S = typename decltype(s_traits)::type;
            return func(this->make_surface<S>(offset));
        },
        this->surface_type(fid));
}

#endif

//---------------------------------------------------------------------------//
/*!
 * Surface type of a face.
 */
CELER_FORCEINLINE_FUNCTION SurfaceType
CompressedFaceVisitor::surface_type(FaceId fid) const
{
    return types_[*fid];
}

//---------------------------------------------------------------------------//
/*!
 * Create the surface at this offset.
 */
template<class T>
CELER_FUNCTION T CompressedFaceVisitor::make_surface(size_type offset) const
{
    constexpr auto size{T::StorageSpan::extent};
    using LdgSpanT = LdgSpan<real_type const, size>;

    // auto data_end = data_begin + size;
    // CELER_ASSERT(data_end <= *faces_.reals.end());
    //  auto data = params_.reals[ItemRange<real_type>{data_begin, data_end}];
    //  return T{LdgSpanT{data}};
    return T{LdgSpanT{reals_ + offset, reals_ + offset + size}};
}

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

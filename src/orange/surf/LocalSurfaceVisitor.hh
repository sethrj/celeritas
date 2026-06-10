//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/surf/LocalSurfaceVisitor.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Types.hh"
#include "corecel/math/Algorithms.hh"
#include "orange/OrangeData.hh"

#include "SurfaceTypeTraits.hh"

#include "detail/AllSurfaces.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Apply a functor to a type-deleted local surface.
 *
 * An instance of this class is like \c std::visit but accepting a
 * \c LocalSurfaceId rather than a \c std::variant .
 *
 * Example: \code
 LocalSurfaceVisitor visit_surface{params_};
 auto sense = visit_surface(
    [&pos](auto const& s) { return s.calc_sense(pos); },
    surface_id);
 \endcode
 */
class LocalSurfaceVisitor
{
  public:
    //!@{
    //! \name Type aliases
    using ParamsRef = NativeCRef<OrangeParamsData>;
    //!@}

  public:
    // Construct from ORANGE params and surfaces redord
    inline CELER_FUNCTION
    LocalSurfaceVisitor(ParamsRef const& params,
                        SurfacesRecord const& local_surfaces);

    // Construct from ORANGE params and simple unit ID
    inline CELER_FUNCTION
    LocalSurfaceVisitor(ParamsRef const& params, SimpleUnitId unit);

    // Apply the function to the surface specified by the given ID
    template<class F>
    inline CELER_FUNCTION decltype(auto)
    operator()(F&& typed_visitor, LocalSurfaceId t) const;

  private:
    //// TYPES ////

    template<class T>
    using Items = Collection<T, Ownership::const_reference, MemSpace::native>;
    using SpanReal = LdgSpan<real_type const>;

    //// DATA ////

    ParamsRef const& params_;
    SurfacesRecord const& surfaces_;

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
 * Construct from ORANGE data and local surfaces.
 *
 * This is meant to be called from inside a simple unit tracker.
 */
CELER_FORCEINLINE_FUNCTION
LocalSurfaceVisitor::LocalSurfaceVisitor(ParamsRef const& params,
                                         SurfacesRecord const& local_surfaces)
    : params_{params}, surfaces_{local_surfaces}
{
}

//---------------------------------------------------------------------------//
/*!
 * Construct from ORANGE data with surfaces from a simple unit.
 */
CELER_FORCEINLINE_FUNCTION
LocalSurfaceVisitor::LocalSurfaceVisitor(ParamsRef const& params,
                                         SimpleUnitId unit)
    : LocalSurfaceVisitor{params, params.simple_units[unit].surfaces}
{
}

#if !defined(__DOXYGEN__) || __DOXYGEN__ > 0x010908
//---------------------------------------------------------------------------//
/*!
 * Apply the function to the surface specified by the given ID.
 */
template<class F>
CELER_FUNCTION decltype(auto)
LocalSurfaceVisitor::operator()(F&& func, LocalSurfaceId id) const
{
    CELER_EXPECT(id < surfaces_.size());

    constexpr bool is_specialized_plane
        = std::is_invocable_v<F, SurfaceType, SpanReal>;

    auto st = params_.surface_types[surfaces_.types[id]];
    if constexpr (is_specialized_plane)
    {
        if (is_plane(st))
        {
            auto data = this->surface_data(id, st != SurfaceType::p ? 1 : 4);
            return func(st, data);
        }
    }

    // Apply type-deleted functor based on type
    return visit_surface_type(
        [&](auto s_traits) {
            using S = typename decltype(s_traits)::type;
            using StorageSpan = typename S::StorageSpan;

            auto data = this->surface_data(id, StorageSpan::extent);
            if constexpr (is_specialized_plane && is_plane(s_traits()))
            {
                // Do not emit code
                CELER_ASSERT_UNREACHABLE();
            }

            // Call the user-provided action using the reconstructed surface
            return func(this->make_surface<S>(data));
        },
        st);
}
#endif

//---------------------------------------------------------------------------//
// PRIVATE HELPER FUNCTIONS
//---------------------------------------------------------------------------//
/*!
 * Load the data for a given surface.
 */
CELER_FUNCTION auto
LocalSurfaceVisitor::surface_data(LocalSurfaceId id, size_type size) const
    -> SpanReal
{
    using Reals = Items<real_type>;
    using RealIdT = Reals::ItemIdT;
    using RealRangeT = Reals::ItemRangeT;
    RealIdT offset = params_.real_ids[surfaces_.data_offsets[id]];
    CELER_ASSERT(offset + size <= params_.reals.size());
    return params_.reals[RealRangeT{offset, offset + size}];
}

//---------------------------------------------------------------------------//
/*!
 * Construct a surface of a given type using a span of data.
 */
template<class T>
CELER_FUNCTION T LocalSurfaceVisitor::make_surface(SpanReal storage_span) const
{
    constexpr size_type size{T::StorageSpan::extent};
    CELER_ASSUME(storage_span.size() == size);
    using LdgSpanT = LdgSpan<real_type const, size>;
    return T{LdgSpanT{storage_span}};
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

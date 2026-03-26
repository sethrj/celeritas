//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file geocel/vg/VecGeomView.h
//---------------------------------------------------------------------------//
#pragma once

#include <VecGeom/navigation/NavigationState.h>

#include "corecel/OpaqueId.hh"
#include "corecel/cont/Span.hh"
#include "corecel/math/SpanUtils.hh"
#include "geocel/Types.hh"

#include "VecgeomTypes.hh"

#if CELERITAS_VECGEOM_SURFACE
#    include "detail/SurfNavigator.hh"
#elif CELERITAS_VECGEOM_VERSION < 0x020000
#    include "detail/BVHNavigator.hh"
#else
#    include "detail/SolidsNavigator.hh"
#endif

namespace celeritas
{
//---------------------------------------------------------------------------//
// TYPE ALIASES
//---------------------------------------------------------------------------//

//! Build-time-selected opaque navigation path (NavTuple or NavIndex_t)
using OpaquePath = VgNavStateImpl;

//! VecGeom logical volume ID (maps to vecgeom::LogicalVolume::id())
using VgLogicalVolumeId = ImplVolumeId;

//! VecGeom placed (physical) volume ID (maps to vecgeom::VPlacedVolume::id())
using VgPlacedVolumeId = OpaqueId<struct VgPlacedVolume_, unsigned int>;

//! Result of a view 'find'
enum class VgFoundStatus
{
    error = -1,  //!< Overlap detected
    reentrant = 0,  //!< On a boundary and headed back in
    intersect = 1,  //!< Next boundary is within max distance
    unlimited = 2,  //!< Next boundary is more than max distance
};

//! Result of a call to vecgeom's FindNextBoundary call
struct VgFindResult
{
    VgFoundStatus status{};  //!< Whether a boundary was hit
    real_type distance{0};  //!< Distance traveled, see constraints below
};

//---------------------------------------------------------------------------//
/*!
 * Low-level VecGeom navigation interface for a single thread.
 *
 * \c VecGeomView is an ephemeral wrapper around externally owned VecGeom
 * navigation state.  It provides direct access to VecGeom navigator
 * primitives without Collection-based storage or track-slot management.
 * The caller is responsible for keeping track of the on-boundary flag and the
 * straight-line distance between operations (see the refactor.md design
 * document for the full contract).
 *
 * The view borrows (does not own) references to:
 *  - Current \c VgNavState: encodes the current + previous navigation path
 *    and surface-crossing flag.
 *  - Next \c VgNavState: scratch buffer used for half-relocation.
 *  - Global position as a \c Span<Real,3>.
 *  - Global direction as a \c Span<Real,3>.
 *
 * The \em opaque path (\c OpaquePath = \c VgNavStateImpl) is the low-level,
 * POD-compatible representation of a navigation path that is selected at
 * build time:
 *  - \c VgNavIndex   when \c VECGEOM_USE_NAVINDEX is set.
 *  - \c NavTuple     when \c VECGEOM_USE_NAVTUPLE  is set.
 * G4VG converts between Geant4 touchable paths and \c OpaquePath.
 *
 * Typical straight-tracking usage:
 * \code
 *   VecGeomView view(cur_state, nxt_state, pos_span, dir_span);
 *   view.Initialize(opaque_path, pos_span, dir_span);
 *   Propagation prop = view.FindNextBoundary(max_step);
 *   if (prop.boundary) {
 *     Real3 norm = view.CalcNormal();   // cache before crossing if needed
 *     view.MoveToBoundary(prop.distance);
 *     view.CrossBoundary();
 *   } else {
 *     view.MoveInternal(prop.distance);
 *   }
 * \endcode
 *
 * \sa VecgeomTrackView
 * \sa refactor.md
 */
class VecGeomView
{
  public:
    //!@{
    //! \name Type aliases
    using Real = vg_real_type;
    using SpanReal3 = Span<Real, 3>;
    using ConstSpanReal3 = Span<Real const, 3>;
#if CELERITAS_VECGEOM_SURFACE
    using Navigator = celeritas::detail::SurfNavigator;
#elif CELERITAS_VECGEOM_VERSION < 0x020000
    using Navigator = celeritas::detail::BVHNavigator;
#else
    using Navigator = celeritas::detail::SolidsNavigator;
#endif
    //!@}

    /*!
     * Construct from externally owned navigation state and position/direction.
     *
     * All arguments are borrowed references; the caller must ensure they
     * outlive this view.
     *
     * \param cur_state  Current navigation state (encodes path + boundary
     *                   flag).
     * \param nxt_state  Scratch navigation state used as a "next" buffer for
     *                   half-relocation.
     * \param pos        Mutable span over the global position (3 elements,
     *                   cm).
     * \param dir        Mutable span over the global direction (3 elements,
     *                   unit vector).
     */
    VecGeomView(VgNavState& cur_state,
                VgNavState& nxt_state,
                SpanReal3 pos,
                SpanReal3 dir);

    //// INITIALIZATION ////

    /*!
     * Initialize navigation from an existing opaque path (CPU only).
     *
     * Intended for ADePT-style use where a Geant4 touchable path has already
     * been converted to an \c OpaquePath by G4VG.  Currently supported on CPU
     * only.
     *
     * \pre \p path must be valid and represent a volume inside the world.
     * \pre \p pos must lie inside or on the boundary of the volume encoded by
     *      \p path.
     * \pre \p dir must be normalised to unit length within machine precision.
     *
     * \post Navigation state is fully initialised and all operations are
     * valid.
     */
    void
    Initialize(OpaquePath const& path, ConstSpanReal3 pos, ConstSpanReal3 dir);

    /*!
     * Initialize navigation from position and direction alone.
     *
     * Performs a full geometry location to find the enclosing volume.  Works
     * on both host and device, and is the preferred initialisation path for
     * generating primary particles from spatial distributions (e.g.\ uniform
     * box) on device.
     *
     * \pre \p dir must be normalised to unit length within machine precision.
     *
     * \return Whether the input position is inside the world (valid)
     *
     * \post If the return value is true (success), then the complete
     * navigation path to the deepest enclosing volume is found, and the state
     * is ready for further queries.
     */
    inline bool Initialize(ConstSpanReal3 pos, ConstSpanReal3 dir);

    //// OPERATIONS ////

    /*!
     * Compute a conservative isotropic distance to the nearest boundary.
     *
     * Returns a safety radius \em s satisfying the following constraints for
     * the maximum input distance \em d and the true inscribed-sphere radius
     * \em t:
     *  - \f$ s > 0 \f$, \em except when \f$ t < \varepsilon \f$ (near-boundary
     *    floating-point inaccuracy).
     *  - \f$ s \le d \f$.
     *  - \f$ s \le t \f$ (conservative upper bound on the inscribed sphere).
     *  - Ideally \f$ s \ge C\,t \f$ for a geometry-independent constant \em C
     *    (bounded relative error), though this cannot always be guaranteed.
     *
     * \pre The track is not on a boundary.
     * \pre \p max is strictly positive.
     *
     * \param max Maximum search distance.
     * \return Conservative safety radius.
     *
     * \note May internally cache the nearest geometry facet, but the caller is
     *       responsible for caching the result (position plus radius squared).
     */
    Real FindSafety(Real max) const;

    /*!
     * Find the straight-line distance to the next boundary.
     *
     * Optimised for the common case: interior volume, no overlap, no error.
     * The returned \c Propagation encodes:
     *  - \c boundary=true, \c distance>0: an intersection was found at the
     *    returned distance (\em intersection case).  The next state is cached
     *    internally; call \c MoveToBoundary then \c CrossBoundary.
     *  - \c boundary=false, \c distance=\p max: no boundary within \p max
     *    (\em no-intersection case).  Call \c MoveInternal.
     *  - \c boundary=true, \c distance=0: track is on a boundary headed
     *    outward (\em reentrant case).  Typically handled by the caller as a
     *    degenerate step.
     *  - Negative solid distance signals a geometry overlap (\em error case);
     *    behaviour is implementation-defined.
     *
     * \pre The track position is inside the current volume or on a boundary.
     * \pre \p max is strictly positive.
     *
     * \param max Maximum search distance.
     * \return \c Propagation containing distance and boundary flag.
     *
     * \post Internal next state is cached (current implementation).
     * \post The next distance is \em not cached; the caller must store it if
     *       \c MoveToBoundary will need it.
     */
    VgFindResult FindNextBoundary(Real max);

    /*!
     * Change the current track direction.
     *
     * May be called while on a boundary (e.g.\ before EM boundary crossing or
     * after optical reflection).
     *
     * \pre \p dir is normalised to unit length within machine epsilon.
     *
     * \param dir New direction unit vector.
     *
     * \post The cached next state, next placed-volume, and next surface are
     *       invalidated; \c FindNextBoundary must be called before any move.
     */
    void ChangeDirection(ConstSpanReal3 dir);

    /*!
     * Move to an arbitrary position within the current safety sphere.
     *
     * Used for charged tracks whose position is displaced transversely inside
     * the safety bubble (e.g.\ by magnetic-field or multiple-scattering
     * correction), where the displacement is guaranteed not to cross a
     * boundary.
     *
     * \pre \p pos lies within the current safety sphere.
     *
     * \param pos New global position.
     *
     * \post Internal boundary flag may be reset to false.
     */
    void MoveInternal(ConstSpanReal3 pos);

    /*!
     * Move along the current direction without reaching a boundary.
     *
     * Used for neutral tracks (photons) and for charged tracks near a
     * boundary where the safety is less than the sagitta or miss distance.
     * Unlike Geant4, no re-location call is needed after this move.
     *
     * \pre \p step is strictly positive.
     * \pre \p step is strictly less than the distance to the next boundary.
     *
     * \param step Distance to advance along the current direction.
     *
     * \post Internal boundary flag may be reset to false.
     */
    void MoveInternal(Real step);

    /*!
     * Advance to the boundary at the exact distance returned by
     * \c FindNextBoundary.
     *
     * \pre \c FindNextBoundary was called and returned \c boundary=true.
     * \pre \c ChangeDirection has \em not been called since the last
     *      \c FindNextBoundary.
     *
     * \param step Exact distance returned by the preceding \c FindNextBoundary
     *             call.
     *
     * \post The track is on the boundary (distance from true surface less than
     *       a small tolerance, but potentially more than machine epsilon).
     * \post Internal boundary state is set to "on boundary, heading into
     *       surface".
     */
    void MoveToBoundary(Real step);

    /*!
     * Compute the surface normal at the current boundary.
     *
     * Calculates the outward-facing normal of the solid being exited without
     * requiring extra storage or an expensive search, because both the current
     * and next navigation states are available.  The application must cache
     * the result before calling \c CrossBoundary if the normal is needed
     * afterwards.
     *
     * The sign of the returned vector is \em not guaranteed: it may point out
     * of the current volume or into a daughter, depending on VecGeom's solid
     * implementation.
     *
     * \pre The track is on a boundary and \c FindNextBoundary has been called.
     * \pre \c MoveToBoundary has been called.
     * \pre \c CrossBoundary has \em not yet been called.
     *
     * \return Surface normal direction (sign unspecified).
     */
    SpanReal3 CalcNormal() const;

    /*!
     * Cross the current boundary and update the navigation state.
     *
     * Performs relocation: the "next" half-cooked state becomes the fully
     * correct current state.  The "next" state is then invalidated, so
     * \c FindNextBoundary must be called again before the next
     * \c MoveToBoundary or \c CrossBoundary.
     *
     * \pre The track is on a boundary (i.e.\ \c FindNextBoundary and
     *      \c MoveToBoundary have been called in order).
     *
     * \post The current navigation path reflects the new volume; calls to
     *       \c GetLogicalVolumeId and \c GetPlacedVolumeId will return updated
     *       values.
     * \post The internal next state is invalidated.
     */
    void CrossBoundary();

    //// ACCESSORS ////

    /*!
     * VecGeom logical volume ID of the current volume.
     *
     * On host this can be mapped to \c vecgeom::LogicalVolume* and from there
     * to Celeritas materials, sensitive-detector regions, or Geant4 logical
     * volume pointers.
     */
    VgLogicalVolumeId GetLogicalVolumeId() const;

    /*!
     * VecGeom placed volume ID of the current volume instance.
     *
     * On host this can be mapped to \c vecgeom::VPlacedVolume* and from there
     * to a \c G4VPhysicalVolume*.
     */
    VgPlacedVolumeId GetPlacedVolumeId() const;

    /*!
     * Read-only reference to the current navigation path as an opaque path.
     *
     * The path encodes the full touchable hierarchy and is only meaningful in
     * the context of the VecGeom geometry from which this view was
     * constructed.
     */
    OpaquePath const& GetOpaquePath() const;

    //! Current global position.
    SpanReal3 Position() const;

    //! Current global direction (unit vector).
    SpanReal3 Direction() const;

    /*!
     * Current position in the local coordinate frame of the current volume.
     *
     * Required for Woodcock tracking and other algorithms that need to
     * evaluate solid-specific distance-to-out from the local frame.
     */
    SpanReal3 LocalPosition() const;

    /*!
     * Current direction in the local coordinate frame of the current volume.
     *
     * Required for Woodcock tracking and other algorithms that need to
     * evaluate solid-specific distance-to-out from the local frame.
     */
    SpanReal3 LocalDirection() const;

  private:
    //// DATA ////

    VgNavState& cur_state_;  //!< Current navigation path
    VgNavState& nxt_state_;  //!< Scratch "next" navigation path
    SpanReal3 pos_;  //!< Global position [cm]
    SpanReal3 dir_;  //!< Global direction (unit vector)
};

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Initialize navigation from position and direction alone.
 *
 * Stores the given position and direction into the borrowed spans, clears the
 * current VecGeom navigation state, then performs a full
 * \c Navigator::LocatePointIn call to find the deepest enclosing volume.
 */
CELER_FUNCTION bool
VecGeomView::Initialize(ConstSpanReal3 pos, ConstSpanReal3 dir)
{
    // Copy position and direction into the externally owned storage
    copy(pos, pos_);
    copy(dir, dir_);

    // Set up current state and locate the deepest enclosing volume
    cur_state_.Clear();
#if CELERITAS_VECGEOM_SURFACE
    // Surface navigator identifies worlds by integer ID
    VgPlacedVolumeInt world = vecgeom::NavigationState::WorldId();
#else
    auto const* world = vecgeom::GeoManager::Instance().GetWorld();
#endif
    constexpr bool contains_point = true;
    Navigator::LocatePointIn(
        world, to_vgvector(pos_), cur_state_, contains_point);
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

//----------------------------------*-C++-*----------------------------------//
// Copyright 2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/geo/MockGeoTrackView.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Macros.hh"
#include "corecel/sys/ThreadId.hh"
#include "orange/Types.hh"

namespace celeritas
{
struct MockGeoParamsData;
struct MockGeoStateData;

//---------------------------------------------------------------------------//
/*!
 * Mock track view.
 *
 * This declaration-only class serves as a template for the geometry track view
 * and can be used for generating assembly output for instructive purposes.
 */
class MockGeoTrackView
{
  public:
    //!@{
    //! \name Type aliases
    using ParamsRef = MockGeoParamsData;
    using StateRef = MockGeoStateData;
    using Initializer_t = GeoTrackInitializer;
    //!@}

    //! Helper struct for initializing from an existing geometry state
    struct DetailedInitializer
    {
        MockGeoTrackView const& other;  //!< Existing geometry
        Real3 const& dir;  //!< New direction
    };

  public:
    // Construct from params and state
    CELER_FUNCTION MockGeoTrackView(ParamsRef const& params,
                                    StateRef& states,
                                    TrackSlotId tid);

    // Initialize the state
    CELER_FUNCTION MockGeoTrackView& operator=(Initializer_t const& init);
    // Initialize the state from a parent state and new direction
    CELER_FUNCTION MockGeoTrackView& operator=(DetailedInitializer const& init);

    //// ACCESSORS ////

    // The current position
    CELER_FUNCTION Real3 const& pos() const;
    // The current direction
    CELER_FUNCTION Real3 const& dir() const;
    // The current volume ID (null if outside)
    CELER_FUNCTION VolumeId volume_id() const;
    // The current surface ID
    CELER_FUNCTION SurfaceId surface_id() const;
    // After 'find_next_step', the next straight-line surface
    CELER_FUNCTION SurfaceId next_surface_id() const;
    // Whether the track is outside the valid geometry region
    CELER_FUNCTION bool is_outside() const;
    // Whether the track is exactly on a surface
    CELER_FUNCTION bool is_on_boundary() const;

    //// OPERATIONS ////

    // Find the distance to the next boundary
    CELER_FUNCTION Propagation find_next_step();

    // Find the distance to the next boundary, up to and including a step
    CELER_FUNCTION Propagation find_next_step(real_type max_step);

    // Find the distance to the nearest boundary in any direction
    CELER_FUNCTION real_type find_safety();

    // Find the distance to the nearest nearby boundary in any direction
    CELER_FUNCTION real_type find_safety(real_type max_step);

    // Move to the boundary in preparation for crossing it
    CELER_FUNCTION void move_to_boundary();

    // Move within the volume
    CELER_FUNCTION void move_internal(real_type step);

    // Move within the volume to a specific point
    CELER_FUNCTION void move_internal(Real3 const& pos);

    // Cross from one side of the current surface to the other
    CELER_FUNCTION void cross_boundary();

    // Change direction
    CELER_FUNCTION void set_dir(Real3 const& newdir);

  private:
    MockGeoParamsData const* params_;
    MockGeoStateData* state_;
};

//---------------------------------------------------------------------------//
}  // namespace celeritas

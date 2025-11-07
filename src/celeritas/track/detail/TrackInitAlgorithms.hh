//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/track/detail/TrackInitAlgorithms.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Assert.hh"
#include "corecel/Macros.hh"
#include "corecel/Types.hh"
#include "corecel/data/Collection.hh"
#include "corecel/sys/ThreadId.hh"
#include "celeritas/global/CoreParams.hh"

#include "../CoreStateCounters.hh"
#include "../TrackInitData.hh"
#include "../Utils.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
//! Predicate for sorting charged from neutral tracks with a stencil
struct IsNeutralStencil
{
    using ParamsPtr = CRefPtr<CoreParamsData, MemSpace::native>;

    ParamsPtr params;
    TrackInitializer const* initializers;

    CELER_FUNCTION bool operator()(TrackSlotId::size_type i) const
    {
        CELER_EXPECT(initializers);
        return IsNeutral{params}(initializers[i]);
    }
};

template<Ownership W, MemSpace M>
using TrackSlotCollection = StateCollection<TrackSlotId, W, M>;

//---------------------------------------------------------------------------//
// Remove all elements in the vacancy vector that were flagged as alive
TrackSlotId::size_type
remove_if_alive(HostRef<TrackSlotCollection> const&, StreamId);
TrackSlotId::size_type
remove_if_alive(DeviceRef<TrackSlotCollection> const&, StreamId);

//---------------------------------------------------------------------------//
// Calculate the exclusive prefix sum of the number of surviving secondaries
TrackSlotId::size_type
exclusive_scan_counts(HostRef<TrackSlotCollection> const&, StreamId);
TrackSlotId::size_type
exclusive_scan_counts(DeviceRef<TrackSlotCollection> const&, StreamId);

//---------------------------------------------------------------------------//
// Sort the tracks that will be initialized in this step by charged/neutral
void partition_initializers(CoreParams const&,
                            HostRef<TrackInitStateData> const&,
                            CoreStateCounters const&,
                            TrackSlotId::size_type,
                            StreamId);
void partition_initializers(CoreParams const&,
                            DeviceRef<TrackInitStateData> const&,
                            CoreStateCounters const&,
                            TrackSlotId::size_type,
                            StreamId);

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
#if !CELER_USE_DEVICE
inline TrackSlotId::size_type
remove_if_alive(DeviceRef<TrackSlotCollection> const&, StreamId)
{
    CELER_NOT_CONFIGURED("CUDA or HIP");
}

inline TrackSlotId::size_type
exclusive_scan_counts(DeviceRef<TrackSlotCollection> const&, StreamId)
{
    CELER_NOT_CONFIGURED("CUDA or HIP");
}

inline void partition_initializers(
    CoreParams const&,
    TrackInitStateData<Ownership::reference, MemSpace::device> const&,
    CoreStateCounters const&,
    TrackSlotId::size_type,
    StreamId)
{
    CELER_NOT_CONFIGURED("CUDA or HIP");
}

#endif
//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

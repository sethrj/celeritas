//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/ThreadId.hh
//! \todo Rename to corecel/Id.hh
//---------------------------------------------------------------------------//
#pragma once

#include <cstdint>

#include "corecel/OpaqueId.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
//! Unique ID for multithreading/multitasking
using StreamId = OpaqueId<class Stream_, std::uint16_t>;

//! Index of a thread inside the current kernel
using ThreadId = OpaqueId<struct Thread_, unsigned int>;

//! Index of a state inside the vector of all states
using TrackSlotId = OpaqueId<struct TrackSlot_, unsigned int>;

//! Within-step action to apply to a track
using ActionId = OpaqueId<class ActionInterface, std::uint16_t>;

//---------------------------------------------------------------------------//
}  // namespace celeritas

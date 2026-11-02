//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/Stream.cc
//---------------------------------------------------------------------------//
#include "Stream.hh"

#include "corecel/DeviceRuntimeApi.hh"

#include "corecel/Assert.hh"
#include "corecel/sys/DeviceEvent.hh"
#include "corecel/sys/StreamRuntime.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
void Stream::sync() const
{
    CELER_EXPECT(*this);
    CELER_DEVICE_API_CALL(
        StreamSynchronize(detail::StreamRuntimeAccess::get(*this)));
}

//---------------------------------------------------------------------------//
void Stream::wait(DeviceEvent const& event)
{
    CELER_EXPECT(*this);
    CELER_EXPECT(event);
    CELER_DEVICE_API_CALL(
        StreamWaitEvent(detail::StreamRuntimeAccess::get(*this),
                        detail::DeviceEventRuntimeAccess::get(event),
                        /* flags = */ 0));
}

//---------------------------------------------------------------------------//
void Stream::launch_host_func(HostKernel func, void* data)
{
    CELER_EXPECT(*this);
    CELER_DEVICE_API_CALL(
        LaunchHostFunc(detail::StreamRuntimeAccess::get(*this), func, data));
#if !CELER_USE_DEVICE
    CELER_DISCARD(func);
    CELER_DISCARD(data);
#endif
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

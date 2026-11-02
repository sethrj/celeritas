//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/StreamRuntime.hh
//---------------------------------------------------------------------------//
#pragma once

#include <cstddef>

#include "corecel/DeviceRuntimeApi.hh"

namespace celeritas
{
class DeviceEvent;
class Stream;

namespace detail
{
class AsyncMemoryResource;

#if CELER_USE_DEVICE
using RuntimeStreamT = CELER_DEVICE_API_SYMBOL(Stream_t);
using RuntimeEventT = CELER_DEVICE_API_SYMBOL(Event_t);
#else
using RuntimeStreamT = std::nullptr_t;
using RuntimeEventT = std::nullptr_t;
#endif

void* malloc_async(std::size_t bytes, RuntimeStreamT stream);
void free_async(void* ptr, RuntimeStreamT stream);

//! Runtime-only access to stream and event internals
class StreamRuntimeAccess
{
  public:
    static RuntimeStreamT get(Stream const&);
    static AsyncMemoryResource& memory_resource(Stream&);
    static void* create_memory_resource(RuntimeStreamT);
    static void destroy_memory_resource(void*) noexcept;
};

//! Runtime-only access to an event's native handle
class DeviceEventRuntimeAccess
{
  public:
    static RuntimeEventT get(DeviceEvent const&);
};

}  // namespace detail
}  // namespace celeritas

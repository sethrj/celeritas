//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/Stream.hh
//---------------------------------------------------------------------------//
#pragma once

#include <cstddef>
#include <memory>

#include "corecel/Assert.hh"  // IWYU pragma: keep
#include "corecel/Macros.hh"  // IWYU pragma: keep

namespace celeritas
{
//---------------------------------------------------------------------------//
class Device;
class DeviceEvent;

namespace detail
{
class StreamRuntimeAccess;
}

//---------------------------------------------------------------------------//
/*!
 * Host-facing owner for a CUDA or HIP stream.
 *
 * The native runtime handle and Thrust allocator are available only through
 * the device-specific StreamRuntime API.
 */
class Stream
{
  public:
    using HostKernel = void (*)(void*);

  public:
    Stream();
    explicit Stream(Device const& device);
    CELER_DEFAULT_MOVE_DELETE_COPY(Stream);
    ~Stream();

    inline explicit operator bool() const;

    void* malloc_async(std::size_t bytes);
    void free_async(void* ptr);
    void sync() const;
    void wait(DeviceEvent const& event);
    void launch_host_func(HostKernel func, void* data);

  private:
    struct Impl;
    struct ImplDeleter
    {
        void operator()(Impl*) noexcept;
    };
    std::unique_ptr<Impl, ImplDeleter> impl_;

    friend class detail::StreamRuntimeAccess;
};

//---------------------------------------------------------------------------//
/*!
 * Whether the stream is constructed and not moved from.
 */
Stream::operator bool() const
{
    return static_cast<bool>(impl_);
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

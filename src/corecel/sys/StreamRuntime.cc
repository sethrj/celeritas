//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/StreamRuntime.cc
//---------------------------------------------------------------------------//
#include "StreamRuntime.hh"

#include <iostream>

#include "corecel/Assert.hh"
#include "corecel/io/Logger.hh"
#include "corecel/sys/Device.hh"
#include "corecel/sys/Stream.hh"

namespace celeritas
{
namespace
{
#if CELER_USE_DEVICE
struct StreamableStream
{
    detail::RuntimeStreamT stream;
};

#    if !CELER_DEVICE_COMPILE
std::ostream& operator<<(std::ostream& os, StreamableStream const& value)
{
#        if CUDART_VERSION >= 12000
    unsigned long long stream_id{0};
    CELER_DEVICE_API_CALL(StreamGetId(value.stream, &stream_id));
    os << "id=" << stream_id;
#        else
    os << '@' << static_cast<void*>(value.stream);
#        endif
    return os;
}
#    endif
#endif
}  // namespace

//---------------------------------------------------------------------------//
struct Stream::Impl
{
    detail::RuntimeStreamT stream{};
    void* memory_resource{};
};

//---------------------------------------------------------------------------//
void Stream::ImplDeleter::operator()(Impl* impl) noexcept
{
    if (!impl)
        return;

#if CELER_USE_DEVICE
    try
    {
        CELER_LOG_LOCAL(debug)
            << "Destroying stream " << StreamableStream{impl->stream};
        detail::StreamRuntimeAccess::destroy_memory_resource(
            impl->memory_resource);
        CELER_DEVICE_API_CALL(StreamDestroy(impl->stream));
        delete impl;
    }
    catch (RuntimeError const& e)
    {
        std::clog << "Failed to destroy stream: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::clog << "Failed to destroy stream" << std::endl;
    }
#else
    delete impl;
#endif
}

//---------------------------------------------------------------------------//
Stream::Stream() : Stream(celeritas::device()) {}

//---------------------------------------------------------------------------//
Stream::Stream(Device const& device)
{
#if CELER_USE_DEVICE
    CELER_EXPECT(device);
    detail::RuntimeStreamT stream;
    CELER_DEVICE_API_CALL(StreamCreate(&stream));
    CELER_LOG_LOCAL(debug) << "Created stream " << StreamableStream{stream};
    impl_.reset(new Impl{stream, nullptr});
    impl_->memory_resource
        = detail::StreamRuntimeAccess::create_memory_resource(stream);
#else
    CELER_DISCARD(device);
#endif
}

//---------------------------------------------------------------------------//
Stream::~Stream() = default;

//---------------------------------------------------------------------------//
void* Stream::malloc_async(std::size_t bytes)
{
    CELER_EXPECT(*this);
#if CELER_USE_DEVICE
    return detail::malloc_async(bytes, impl_->stream);
#else
    CELER_DISCARD(bytes);
    CELER_NOT_CONFIGURED("CUDA or HIP");
    return nullptr;
#endif
}

//---------------------------------------------------------------------------//
void Stream::free_async(void* ptr)
{
    CELER_EXPECT(*this);
#if CELER_USE_DEVICE
    detail::free_async(ptr, impl_->stream);
#else
    CELER_DISCARD(ptr);
    CELER_NOT_CONFIGURED("CUDA or HIP");
#endif
}

//---------------------------------------------------------------------------//
detail::RuntimeStreamT detail::StreamRuntimeAccess::get(Stream const& stream)
{
    CELER_EXPECT(stream);
    return stream.impl_->stream;
}

//---------------------------------------------------------------------------//
detail::AsyncMemoryResource& detail::StreamRuntimeAccess::memory_resource(
    Stream& stream)
{
    CELER_EXPECT(stream);
    return *static_cast<detail::AsyncMemoryResource*>(
        stream.impl_->memory_resource);
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

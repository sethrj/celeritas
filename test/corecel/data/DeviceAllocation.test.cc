//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/data/DeviceAllocation.test.cc
//---------------------------------------------------------------------------//
#include "corecel/data/DeviceAllocation.hh"

#include <memory>

#include "corecel/cont/Span.hh"
#include "corecel/sys/Device.hh"
#include "corecel/sys/Stream.hh"

#include "celeritas_test.hh"

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//

// NOTE: don't have 'device' in the name here
TEST(ConstructionTest, should_work_always)
{
    DeviceAllocation alloc;
    EXPECT_EQ(0, alloc.size());
    EXPECT_EQ(nullptr, alloc.data());
    EXPECT_TRUE(alloc.empty());

    DeviceAllocation alloc2;
    swap(alloc, alloc2);
    EXPECT_TRUE(alloc.empty());
    EXPECT_TRUE(alloc.device_ref().empty());
}

#if !CELER_USE_DEVICE
TEST(ConstructionTest, nodevice)
#else
TEST(ConstructionTest, DISABLED_nodevice)
#endif
{
    // Can't allocate
    if (CELERITAS_DEBUG)
    {
        EXPECT_THROW(DeviceAllocation(1234), DebugError);
    }
    else
    {
        EXPECT_THROW(DeviceAllocation(1234), RuntimeError);
    }
}

TEST(DeviceAllocationTest, TEST_IF_CELER_DEVICE(device))
{
    DeviceAllocation sync_alloc{128, std::shared_ptr<Stream>{}};
    EXPECT_FALSE(sync_alloc.stream_owner());
    std::vector<std::byte> sync_data(sync_alloc.size(), std::byte(5));
    sync_alloc.copy_to_device(make_span(sync_data));
    std::vector<std::byte> sync_result(sync_alloc.size());
    sync_alloc.copy_to_host(make_span(sync_result));
    EXPECT_EQ(std::byte(5), sync_result.front());

    DeviceAllocation alloc(1024);
    EXPECT_EQ(1024, alloc.size());
    EXPECT_FALSE(alloc.empty());

    {
        DeviceAllocation other(128);
        std::byte* orig_alloc = alloc.device_ref().data();
        std::byte* orig_other = other.device_ref().data();
        swap(alloc, other);
        EXPECT_EQ(1024, other.size());
        EXPECT_EQ(orig_other, alloc.device_ref().data());
        EXPECT_EQ(orig_alloc, other.device_ref().data());
    }
    std::vector<std::byte> data(alloc.size());
    ASSERT_EQ(128, data.size());

    data.front() = std::byte(1);
    data.back() = std::byte(127);

    alloc.copy_to_device(make_span(data));

    std::vector<std::byte> newdata(alloc.size());
    ASSERT_EQ(128, newdata.size());
    alloc.copy_to_host(make_span(newdata));
    EXPECT_EQ(std::byte(1), newdata.front());
    EXPECT_EQ(std::byte(127), newdata.back());

    // Test move construction/assignment
    {
        std::byte* orig_ptr = alloc.device_ref().data();
        DeviceAllocation other(std::move(alloc));
        EXPECT_EQ(128, other.size());
        EXPECT_EQ(0, alloc.size());
        EXPECT_EQ(orig_ptr, other.device_ref().data());
    }
}

TEST(DeviceAllocationTest, TEST_IF_CELER_DEVICE(empty))
{
    DeviceAllocation alloc{0};
    EXPECT_TRUE(alloc.empty());
    EXPECT_EQ(0, alloc.size());
    EXPECT_EQ(nullptr, alloc.device_ref().data());

    std::vector<std::byte> newdata(alloc.size());
    alloc.copy_to_device(make_span(newdata));
    alloc.copy_to_host(make_span(newdata));
}

TEST(DeviceAllocationTest, TEST_IF_CELER_DEVICE(retain_stream_owner))
{
    device().create_streams(1);
    std::weak_ptr<Stream> weak_stream;
    {
        auto stream = device().stream_owner(StreamId{0});
        weak_stream = stream;
        DeviceAllocation alloc(1024, stream);
        device().destroy_streams();
        stream.reset();
        EXPECT_FALSE(weak_stream.expired());

        std::vector<std::byte> data(alloc.size());
        data.front() = std::byte(7);
        alloc.copy_to_device(make_span(data));
        std::vector<std::byte> result(alloc.size());
        alloc.copy_to_host(make_span(result));
        EXPECT_EQ(std::byte(7), result.front());
    }
    EXPECT_TRUE(weak_stream.expired());
}

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas

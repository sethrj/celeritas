//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/cont/ScopedSwap.test.cc
//---------------------------------------------------------------------------//
#include "corecel/cont/ScopedSwap.hh"

#include "celeritas_test.hh"

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//

TEST(ScopedSwapTest, restore)
{
    int value = 1;
    {
        ScopedSwap swap{&value, 2};
        EXPECT_EQ(2, value);
    }
    EXPECT_EQ(1, value);
}

TEST(ScopedSwapTest, move)
{
    int value = 1;
    {
        ScopedSwap first{&value, 2};
        ScopedSwap second{std::move(first)};
        EXPECT_EQ(2, value);
    }
    EXPECT_EQ(1, value);
}

TEST(ScopedSwapTest, nested)
{
    int value = 1;
    {
        ScopedSwap outer{&value, 2};
        {
            ScopedSwap inner{&value, 3};
            EXPECT_EQ(3, value);
        }
        EXPECT_EQ(2, value);
    }
    EXPECT_EQ(1, value);
}

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas

//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/ScopedLogStorer.test.cc
//---------------------------------------------------------------------------//
#include "corecel/ScopedLogStorer.hh"

#include "corecel/io/Logger.hh"

#include "celeritas_test.hh"

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//

TEST(ScopedLogStorerTest, move)
{
    int original_messages = 0;
    Logger logger{LogHandler{[&](LogProvenance, LogLevel, std::string) {
                      ++original_messages;
                  }},
                  LogLevel::debug};

    {
        ScopedLogStorer source{&logger, LogLevel::debug};
        ScopedLogStorer moved{std::move(source)};
        logger(CELER_CODE_PROVENANCE, LogLevel::warning) << "captured";
        ASSERT_EQ(1u, moved.messages().size());
        EXPECT_EQ("captured", moved.messages().front());
    }

    logger(CELER_CODE_PROVENANCE, LogLevel::warning) << "restored";
    EXPECT_EQ(1, original_messages);
}

TEST(ScopedLogStorerTest, nested)
{
    Logger logger{LogHandler{[](LogProvenance, LogLevel, std::string) {}},
                  LogLevel::debug};
    ScopedLogStorer outer{&logger, LogLevel::debug};
    ScopedLogStorer inner{&logger, LogLevel::debug};

    logger(CELER_CODE_PROVENANCE, LogLevel::warning) << "inner";
    EXPECT_TRUE(outer.empty());
    ASSERT_EQ(1u, inner.messages().size());
    EXPECT_EQ("inner", inner.messages().front());
}

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas

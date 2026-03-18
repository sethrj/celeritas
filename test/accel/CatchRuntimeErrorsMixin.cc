//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file accel/CatchRuntimeErrorsMixin.cc
//---------------------------------------------------------------------------//
#include "CatchRuntimeErrorsMixin.hh"

#include <regex>
#include <string_view>
#include <utility>

using namespace std::string_view_literals;

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//
/*!
 * Return collected exception messages and clear the list.
 */
std::vector<std::string> CatchRuntimeErrorsMixin::release_exceptions()
{
    std::lock_guard scoped_lock{exc_mutex_};
    return std::exchange(exceptions_, {});
}

//---------------------------------------------------------------------------//
/*!
 * Parse and store a RuntimeError's message.
 *
 * The Geant4 exception handler wraps the error in a longer message; this
 * extracts just the original runtime error text.
 */
void CatchRuntimeErrorsMixin::collect_runtime_error(RuntimeError const& e)
{
    CELER_EXPECT(std::string_view(e.details().which) == "Geant4"sv);

    std::lock_guard scoped_lock{exc_mutex_};

    static std::regex extract_error{R"(runtime error:\s*(.+?)(?:\n|$))"};
    std::smatch match;
    std::string what = e.what();
    if (std::regex_search(what, match, extract_error))
    {
        CELER_ASSERT(match.size() > 1);
        exceptions_.push_back(match[1].str());
    }
    else
    {
        exceptions_.push_back(std::move(what));
    }
}

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas

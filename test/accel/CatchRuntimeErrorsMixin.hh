//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file accel/CatchRuntimeErrorsMixin.hh
//---------------------------------------------------------------------------//
#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "corecel/Assert.hh"

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//
/*!
 * Collect caught RuntimeErrors rather than immediately failing.
 *
 * Mix into a test fixture and override \c caught_g4_runtime_error to call
 * \c collect_runtime_error when \c check_runtime_errors_ is set. At the end
 * of the test, call \c release_exceptions to retrieve and clear the list.
 */
class CatchRuntimeErrorsMixin
{
  public:
    //! Enable collection mode instead of immediate test failure
    bool check_runtime_errors_{false};

    //! Return collected exception messages and clear the list
    std::vector<std::string> release_exceptions();

  protected:
    //! Parse and store a RuntimeError's message
    void collect_runtime_error(RuntimeError const& e);

  private:
    std::recursive_mutex exc_mutex_;
    std::vector<std::string> exceptions_;
};

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas

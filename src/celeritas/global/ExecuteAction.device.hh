//----------------------------------*-C++-*----------------------------------//
// Copyright 2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/global/ExecuteAction.device.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/device_runtime_api.h"
#include "corecel/Assert.hh"
#include "corecel/Types.hh"
#include "corecel/sys/Device.hh"
#include "corecel/sys/KernelParamCalculator.device.hh"
#include "corecel/sys/MultiExceptionHandler.hh"
#include "corecel/sys/ThreadId.hh"

#include "ActionInterface.hh"
#include "CoreParams.hh"
#include "CoreState.hh"
#include "KernelContextException.hh"

namespace celeritas
{
namespace
{
//---------------------------------------------------------------------------//
template<class F>
__global__ void execute_action_impl(F launch)
{
    launch(KernelParamCalculator::thread_id());
}

template<class F, class... Ts>
KernelParamCalculator make_kpc(std::string_view label, F* func, Ts... args)
{
    return KernelParamCalculator(label, func, std::forward<Ts>(args));
}

//---------------------------------------------------------------------------//
}  // namespace

template<class F>
class Executor
{
  public:
    explicit Executor(ExplicitActionInterface const& action)
        : entry_(entry)
        , calc_params_{make_kpc(action.label(), execute_action_impl<F>)}
    {
    }

    template<class... Ts>
    void operator()(size_type threads, F const& call_thread)
    {
        auto config = calc_launch_params_(threads);
        execute_action_impl<F>
            <<<config.blocks_per_grid, config.threads_per_block>>>(call_thread);
    }

  private:
    F entry_;
    KernelParamCalculator calc_params_;
};

//---------------------------------------------------------------------------//
/*!
 * Helper struct to run an action in parallel on device.
 */

void execute_action(ExplicitActionInterface const& action)
{
    static Executor<F> launch(*this);
    launch(core_state.size(), Blah{params, states});
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

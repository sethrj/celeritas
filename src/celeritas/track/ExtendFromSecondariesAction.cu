//---------------------------------*-CUDA-*----------------------------------//
// Copyright 2023 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/track/ExtendFromSecondariesAction.cu
//---------------------------------------------------------------------------//
#include "ExtendFromSecondariesAction.hh"

#include "corecel/Types.hh"
#include "corecel/sys/Device.hh"
#include "celeritas/global/ExecuteAction.device.hh"

#include "detail/LocateAliveLauncher.hh"
#include "detail/ProcessSecondariesLauncher.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Launch a kernel to locate alive particles.
 *
 * This fills the TrackInit \c vacancies and \c secondary_counts arrays.
 */
void ExtendFromSecondariesAction::locate_alive(CoreParams const& core_params,
                                               CoreStateDevice& core_state) const
{
    execute_action(*this,
                   core_params,
                   core_state,
                   detail::LocateAliveLauncher{
                       core_params.ptr<MemSpace::native>(), core_state.ptr()});
}

//---------------------------------------------------------------------------//
/*!
 * Launch a kernel to create track initializers from secondary particles.
 */
void ExtendFromSecondariesAction::process_secondaries(
    CoreParams const& core_params, CoreStateDevice& core_state) const
{
    execute_action(
        *this,
        core_params,
        core_state,
        detail::ProcessSecondariesLauncher{core_params.ptr<MemSpace::native>(),
                                           core_state.ptr(),
                                           core_state.counters()});
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

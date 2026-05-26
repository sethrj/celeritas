//------------------------------ -*- cuda -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/optical/action/AlongStepAction.cu
//---------------------------------------------------------------------------//
#include "AlongStepAction.hh"

#include "corecel/sys/KernelParamCalculator.device.hh"
#include "corecel/sys/WarpMask.hh"
#include "celeritas/optical/CoreParams.hh"
#include "celeritas/optical/CoreState.hh"

#include "ActionLauncher.device.hh"

#include "detail/AlongStepExecutor.hh"
#include "detail/PropagateExecutor.hh"

namespace celeritas
{
namespace optical
{
namespace
{
__global__ void __launch_bounds__(CELERITAS_MAX_BLOCK_SIZE)
    launch_along_step(Range<ThreadId> const thread_range,
                      detail::PropagateThreadExecutor execute_thread)
{
    auto tid = KernelParamCalculator::thread_id();
    warp_mask_uint mask
        = ballot_sync(full_warp_mask, tid < thread_range.size());

#if CELER_DEVICE_COMPILE && CELERITAS_DEBUG
    if (mask != full_warp_mask || tid == ThreadId{0})
    {
        printf("launch %04u: 0x%08x\n",
               static_cast<unsigned int>(*tid),
               static_cast<unsigned int>(mask));
    }
#endif

    if (!(tid < thread_range.size()))
        return;

#if CELER_DEVICE_COMPILE && CELERITAS_DEBUG
    if (mask != full_warp_mask || tid == ThreadId{0})
    {
        printf("after early return %04u: 0x%08x\n",
               static_cast<unsigned int>(*tid),
               static_cast<unsigned int>(mask));
    }
#endif

    execute_thread(*(thread_range.cbegin() + tid.get()), mask);
}

}  // namespace
//---------------------------------------------------------------------------//
/*!
 * Launch the along-step action on device.
 */
void AlongStepAction::step(CoreParams const& params,
                           CoreStateDevice& state) const
{
    {
        // Propagate
        static KernelParamCalculator const calc_launch_params_{
            "along-step-propagate", &launch_along_step};
        auto threads = range(ThreadId{state.size()});
        if (!threads.empty())
        {
            ScopedProfiling profile_this_{this->label()};
            using StreamT = CELER_DEVICE_API_SYMBOL(Stream_t);
            StreamT stream
                = celeritas::device().stream(state.stream_id()).get();
            auto config = calc_launch_params_(threads.size());
            launch_along_step<<<config.blocks_per_grid, config.threads_per_block, 0, stream>>>(
                threads,
                detail::PropagateThreadExecutor{params.ptr<MemSpace::native>(),
                                                state.ptr(),
                                                AppliesValidVolumetric{},
                                                detail::PropagateExecutor{}});
        }
    }
    {
        // Update state
        auto execute = make_active_volumetric_thread_executor(
            params.ptr<MemSpace::native>(),
            state.ptr(),
            detail::AlongStepExecutor{});

        static ActionLauncher<decltype(execute)> const launch_kernel(*this);
        launch_kernel(state, execute);
    }
}

//---------------------------------------------------------------------------//
}  // namespace optical
}  // namespace celeritas

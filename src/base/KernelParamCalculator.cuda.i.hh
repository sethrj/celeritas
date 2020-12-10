//---------------------------------*-CUDA-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file KernelParamCalculator.cuda.i.hh
//---------------------------------------------------------------------------//

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Get the linear thread ID.
 */
CELER_FUNCTION auto KernelParamCalculator::thread_id() -> ThreadId
{
#ifdef __CUDA_ARCH__
    return ThreadId{blockIdx.x * blockDim.x + threadIdx.x};
#else
    return ThreadId{0u};
#endif
}

//---------------------------------------------------------------------------//
/*!
 * Get the thread ID if less than the number of threads.
 *
 * This can be used to simplify kernels -- start by calculating thread ID and
 * return if !thread_id. Later to encourage thread cooperation we always
 * initialize track views and have them internally avoid data accesses as
 * needed.
 */
CELER_FUNCTION auto KernelParamCalculator::thread_id(dim_type max_num_threads)
    -> ThreadId
{
    ThreadId result = KernelParamCalculator::thread_id();
    return result < max_num_threads ? result : ThreadId{};
}

//---------------------------------------------------------------------------//
} // namespace celeritas

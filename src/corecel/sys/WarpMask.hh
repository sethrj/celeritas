//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/WarpMask.hh
//---------------------------------------------------------------------------//
#pragma once

#include <cstdint>

#include "corecel/Macros.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
// TODO: as per HIP 7 this should be a kernel template parameter based on
// device properties, or perhaps a configuration compiler definition
constexpr unsigned int warp_size = 32;
using warp_mask_uint = std::uint32_t;
constexpr warp_mask_uint full_warp_mask = ~(warp_mask_uint{0});

#if CELER_DEVICE_COMPILE
CELER_FORCEINLINE __device__ void syncwarp(warp_mask_uint mask = full_warp_mask)
{
    return __syncwarp(mask);
}

CELER_FORCEINLINE __device__ warp_mask_uint ballot_sync(warp_mask_uint mask,
                                                        int predicate)
{
    return __ballot_sync(mask, predicate);
}
#else
CELER_FORCEINLINE void syncwarp(warp_mask_uint = full_warp_mask) {}
CELER_FORCEINLINE warp_mask_uint ballot_sync(warp_mask_uint mask, int predicate)
{
    return mask & static_cast<bool>(predicate);
}
#endif

#if CELERITAS_USE_DEVICE
#    define CELER_DEVICE_IMPL(STMT) STMT
#else
#    define CELER_IF_DEVICE(STMT)
#endif

//---------------------------------------------------------------------------//
}  // namespace celeritas

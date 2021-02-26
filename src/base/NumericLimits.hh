//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file NumericLimits.hh
//---------------------------------------------------------------------------//
#pragma once

#ifdef __CUDA_ARCH__
#    include <cfloat>
#    include <climits>
#    include <math_constants.h>
#else
#    include <limits>
#endif

namespace celeritas
{
#ifdef __CUDA_ARCH__
template<class Numeric>
struct numeric_limits;

template<>
struct numeric_limits<float>
{
    static constexpr __device__ float epsilon() { return FLT_EPSILON; }
    static constexpr __device__ float quiet_NaN() { return CUDART_NAN_F; }
    static constexpr __device__ float infinity() { return CUDART_INF_F; }
    static constexpr __device__ float max() { return FLT_MAX; }
    static constexpr __device__ float min() { return FLT_MIN; }
};

template<>
struct numeric_limits<double>
{
    static constexpr __device__ double epsilon() { return DBL_EPSILON; }
    static constexpr __device__ double quiet_NaN() { return CUDART_NAN; }
    static constexpr __device__ double infinity() { return CUDART_INF; }
    static constexpr __device__ double min() { return DBL_MIN; }
    static constexpr __device__ double max() { return DBL_MAX; }
};

template<>
struct numeric_limits<unsigned int>
{
    static constexpr __device__ unsigned int max() { return UINT_MAX; }
    static constexpr __device__ unsigned int min() { return 0; }
};

template<>
struct numeric_limits<unsigned long>
{
    static constexpr __device__ unsigned long max() { return ULONG_MAX; }
    static constexpr __device__ unsigned long min() { return 0; }
};

template<>
struct numeric_limits<unsigned long long>
{
    static constexpr __device__ unsigned long long max() { return ULLONG_MAX; }
    static constexpr __device__ unsigned long long min() { return 0; }
};

#else // not __CUDA_ARCH__

//! Alias to standard library numeric limits
template<class Numeric>
using numeric_limits = std::numeric_limits<Numeric>;

#endif // __CUDA_ARCH__

} // namespace celeritas

//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file Assert.hh
//---------------------------------------------------------------------------//
#pragma once

#include "base/Assert.hh"

namespace h5
{
//---------------------------------------------------------------------------//
/*!
 * \def CELER_CUDA_CALL
 *
 * Execute the wrapped statement and throw a message if it fails.
 *
 * If it fails, we call \c cudaGetLastError to clear the error code.
 *
 * \code
 *     CELER_H5_CALL(H5Fclose(hid));
 * \endcode
 */
#define CELER_H5_CALL(STATEMENT)         \
    do                                   \
    {                                    \
        herr_t h5_result_ = (STATEMENT); \
        CHECK(h5_result_ >= 0);          \
    } while (0)

//---------------------------------------------------------------------------//
} // namespace h5

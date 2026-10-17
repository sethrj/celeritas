//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/detail/MpiImpl.cc
//---------------------------------------------------------------------------//
#include "MpiImpl.hh"

#include "corecel/Config.hh"

#include "corecel/Assert.hh"
#include "corecel/Macros.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Get an MPI error string.
 *
 * This is used in the implementation of \c CELER_MPI_CALL .
 */
std::string mpi_error_to_string(int errorcode)
{
#if CELERITAS_USE_MPI
    std::string error_string;
    error_string.resize(MPI_MAX_ERROR_STRING);
    int length = 0;
    int new_errorcode = 0;
    new_errorcode = MPI_Error_string(errorcode, &error_string.front(), &length);
    if (new_errorcode != 0)
    {
        // Even the string call failed... just return the code
        return std::tostring(errorcode);
    }
    error_string.resize(length);
    return error_string;
#else
    CELER_DISCARD(errorcode);
    CELER_NOT_CONFIGURED("MPI");
#endif
}

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

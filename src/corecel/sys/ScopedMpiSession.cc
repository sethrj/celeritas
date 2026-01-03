//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/ScopedMpiSession.cc
//---------------------------------------------------------------------------//
#include "ScopedMpiSession.hh"

#include <iostream>
#include <string>

#include "corecel/Config.hh"
#if CELERITAS_USE_MPI
#    include <mpi.h>
#endif

#include "corecel/Assert.hh"
#include "corecel/Macros.hh"

#include "Environment.hh"
#include "MpiCommunicator.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
ScopedMpiSession::Status ScopedMpiSession::status_
    = ScopedMpiSession::Status::uninitialized;

//---------------------------------------------------------------------------//
/*!
 * Construct with argc/argv references.
 *
 * OpenMPI does not modify or access these, but other implementations might
 * potentially.
 */
ScopedMpiSession::ScopedMpiSession(int* argc, char*** argv)
{
    CELER_EXPECT((argc == nullptr) == (argv == nullptr));

    switch (ScopedMpiSession::status())
    {
        case Status::disabled: {
            break;
        }
        case Status::uninitialized: {
            CELER_MPI_CALL(MPI_Init(argc, argv));
            status_ = Status::initialized;
            do_finalize_ = true;
            break;
        }
        case Status::initialized: {
            break;
        }
    }
    CELER_ENSURE(status_ != Status::uninitialized);
}

//---------------------------------------------------------------------------//
/*!
 * Call MPI finalize on destruction.
 */
void ScopedMpiSession::MpiFinalizer::operator()(bool do_finalize) const
{
    if (do_finalize)
    {
        try
        {
            CELER_MPI_CALL(MPI_Finalize());
            status_ = Status::uninitialized;
        }
        catch (RuntimeError const& e)
        {
            std::clog << "During destruction of scoped MPI initialization: "
                      << e.what() << std::endl;
        }
        catch (...)
        {
            std::clog << "Failure during destruction of scoped MPI"
                      << std::endl;
        }
    }
}

//---------------------------------------------------------------------------//
/*!
 * Manually disable MPI.
 */
void ScopedMpiSession::disable()
{
    CELER_EXPECT(status_ == Status::uninitialized);
    status_ = Status::disabled;
}

//---------------------------------------------------------------------------//
/*!
 * Whether MPI has been initialized or disabled.
 */
auto ScopedMpiSession::status() -> Status
{
    if (CELER_UNLIKELY(status_ == Status::uninitialized))
    {
        if (celeritas::getenv_flag("CELER_DISABLE_PARALLEL", !CELERITAS_USE_MPI)
                .value)
        {
            // Environment variable is set: disable MPI
            status_ = Status::disabled;
        }
        else
        {
            // Allow for the case where another application has already
            // initialized MPI.
            int result = -1;
            CELER_MPI_CALL(MPI_Initialized(&result));
            status_ = result ? Status::initialized : Status::uninitialized;
        }
    }
    return status_;
}

//---------------------------------------------------------------------------//
/*!
 * Convenience method to determine whether a multiprocess job is running.
 *
 * This is a shortcut for <code>comm_world().size() > 1</code> meant primarily
 * for applications. Linking against MPI is not required to use it.
 */
bool ScopedMpiSession::is_world_multiprocess() const
{
    if (status_ == Status::disabled)
        return false;
    return comm_world().size() > 1;
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

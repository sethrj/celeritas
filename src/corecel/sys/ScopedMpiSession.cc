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
    = (CELERITAS_USE_MPI ? ScopedMpiSession::Status::unknown
                         : ScopedMpiSession::Status::disabled);

//---------------------------------------------------------------------------//
/*!
 * Create a scoped MPI session without argc/argv references.
 *
 * OpenMPI does not modify or access these, but other implementations might
 * potentially. MPI-2 and above allow nullptr, but we provide a program name
 * so that \c MPI_Info_env will have a non-empty \c command field.
 */
ScopedMpiSession ScopedMpiSession::without_argv()
{
    static char const program_name[] = "celeritas";
    char* argv[] = {const_cast<char*>(program_name)};
    int argc = std::size(argv);
    char** argv_ptr = std::begin(argv);
    return ScopedMpiSession(&argc, &argv_ptr);
}

//---------------------------------------------------------------------------//
/*!
 * Construct with argc/argv references, initializing MPI if applicable.
 *
 * OpenMPI does not modify or access these, but other implementations might
 * potentially.
 */
ScopedMpiSession::ScopedMpiSession(int* argc, char*** argv)
{
    CELER_EXPECT((argc == nullptr) == (argv == nullptr));

    if (status_ == Status::unknown)
    {
        this->check_status();
    }

    if (status_ == Status::uninitialized)
    {
        CELER_MPI_CALL(MPI_Init(argc, argv));
        status_ = Status::initialized;
        do_finalize_ = true;
    }

    CELER_ENSURE(status_ != Status::uninitialized
                 && status_ != Status::unknown);
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
 *
 * This is called by setup::system
 */
void ScopedMpiSession::disable()
{
    CELER_VALIDATE(status_ == Status::initialized,
                   << "cannot disable MPI after it was already initialized");
    status_ = Status::disabled;
}

//---------------------------------------------------------------------------//
/*!
 * Whether MPI has been initialized or disabled.
 */
void ScopedMpiSession::check_status()
{
    if (status_ == Status::unknown)
    {
        // Allow for the case where another application has already
        // initialized MPI.
        int result = -1;
        CELER_MPI_CALL(MPI_Initialized(&result));
        status_ = result ? Status::initialized : Status::uninitialized;
    }
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

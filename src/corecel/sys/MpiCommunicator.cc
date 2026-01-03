//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/MpiCommunicator.cc
//---------------------------------------------------------------------------//
#include "MpiCommunicator.hh"

#include "corecel/Assert.hh"

#include "ScopedMpiSession.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct a communicator with MPI_COMM_WORLD or null if disabled.
 */
MpiCommunicator MpiCommunicator::world_if_enabled()
{
    if (ScopedMpiSession::status() == ScopedMpiSession::Status::disabled)
        return {};

    return MpiCommunicator::world();
}

//---------------------------------------------------------------------------//
/*!
 * Construct with a native MPI communicator.
 *
 * This will fail with a \c NotConfigured error if MPI is disabled.
 */
MpiCommunicator::MpiCommunicator(MpiComm comm) : comm_(comm)
{
    CELER_EXPECT(comm != detail::mpi_comm_null());
    CELER_VALIDATE(
        ScopedMpiSession::status() != ScopedMpiSession::Status::uninitialized,
        << "MPI was not initialized (needed to construct a communicator). "
           "Maybe set the environment variable CELER_DISABLE_PARALLEL=1 to "
           "disable externally?");
    CELER_VALIDATE(
        ScopedMpiSession::status() != ScopedMpiSession::Status::disabled,
        << "tried to create a non-null communicator when MPI is disabled");

    // Save rank and size
    CELER_MPI_CALL(MPI_Comm_rank(comm_, &rank_));
    CELER_MPI_CALL(MPI_Comm_size(comm_, &size_));

    CELER_ENSURE(this->rank() >= 0 && this->rank() < this->size());
}

//---------------------------------------------------------------------------//
/*!
 * Shared Celeritas communicator.
 *
 * TODO: make mutable; make null communicator by default; require system setup:
 * see corecel/setup/System.hh
 */
MpiCommunicator const& comm_world()
{
    static MpiCommunicator comm{MpiCommunicator::world_if_enabled()};
    return comm;
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

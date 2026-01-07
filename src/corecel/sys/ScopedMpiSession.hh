//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/ScopedMpiSession.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Macros.hh"
#include "corecel/cont/InitializedValue.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Enable MPI during the lifetime of this object.
 *
 * This RAII class calls \c MPI_Init on construction with args and, if MPI was
 * not already initialized, calls \c MPI_Finalize on destruction. Move
 * semantics can be used to hand off responsibility for finalization.
 *
 * \note Unlike the MpiCommunicator and MpiOperations class, it is not
 * necessary to link against MPI to use this class.
 */
class ScopedMpiSession
{
  public:
    //! Status of initialization
    enum class Status
    {
        disabled,  //!< Not compiled *or* disabled via environment
        unknown,  //!< MPI support is compiled in but not yet activated
        uninitialized,  //!< MPI_Init has not been called anywhere
        initialized,  //!< MPI_Init has been called somewhere
    };

    // Get whether MPI should be enabled based on environment
    static bool default_from_env();

    // Whether MPI has been initialized
    static Status status() { return status_; }

    // Update MPI status by calling MPI_Initialized
    static void check_status();

    // Manually disable if no one has initialized
    static void disable();

    // Initialize without access to argc/argv
    static ScopedMpiSession without_argv();

  public:
    // Construct with argc/argv references
    ScopedMpiSession(int* argc, char*** argv);

    // Construct without managing MPI
    ScopedMpiSession() = default;

    // Use RAII semantics to hand off responsibility for finalizing
    CELER_DEFAULT_MOVE_DELETE_COPY(ScopedMpiSession);

    //! True if managing finalization
    explicit operator bool() const { return do_finalize_; }

    // Shortcut for comm_world().size() > 1
    bool is_world_multiprocess() const;

  private:
    struct MpiFinalizer
    {
        void operator()(bool) const;
    };

    InitializedValue<bool, MpiFinalizer> do_finalize_{false};

    static Status status_;
};

//---------------------------------------------------------------------------//
}  // namespace celeritas

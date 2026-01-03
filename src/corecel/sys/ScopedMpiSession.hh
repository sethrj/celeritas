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
 * This RAII class calls \c MPI_Init on construction and, if MPI was
 * not already initialized, calls \c MPI_Finalize on destruction. Move
 * semantics can be used to hand off responsibility for finalization.
 *
 * \note Unlike the MpiCommunicator and MpiOperations class, it is not
 * necessary to link against MPI to use this class.
 *
 * \note This class may undergo more modification to support the use case of
 * controlling the use of MPI manually rather than by environment variable.
 *
 * \todo The semantics of default construction are confusing
 */
class ScopedMpiSession
{
  public:
    //! Status of initialization
    enum class Status
    {
        disabled = -1,  //!< Not compiled *or* disabled via environment
        uninitialized = 0,  //!< MPI_Init has not been called anywhere
        initialized = 1  //!< MPI_Init has been called somewhere
    };

    // Manually disable before anyone has initialized
    static void disable();

    // Whether MPI has been initialized
    static Status status();

  public:
    // Construct with argc/argv references
    ScopedMpiSession(int* argc, char*** argv);

    //! Construct with null argc/argv when those are unavailable
    ScopedMpiSession() : ScopedMpiSession(nullptr, nullptr) {}

    //! Use RAII semantics
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

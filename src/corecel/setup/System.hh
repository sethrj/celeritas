//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/setup/System.hh
//---------------------------------------------------------------------------//
#pragma once

#include <memory>

#include "corecel/sys/ScopedMpiSession.hh"
#include "corecel/sys/TracingSession.hh"

namespace celeritas
{
namespace inp
{
struct Logger;
struct System;
}  // namespace inp

namespace setup
{
//---------------------------------------------------------------------------//
// DEFAULTS
//---------------------------------------------------------------------------//
// Set defaults based on environment
void apply_defaults(inp::System&);

//---------------------------------------------------------------------------//
// SET UP
//---------------------------------------------------------------------------//
/*!
 * Scoped objects created during setup.
 *
 * Global objects not returned are \c celeritas::world_logger, \c
 * celeritas::self_logger,
 * \c celeritas::device, and  \c celeritas::environment.
 */
struct SystemLoaded
{
    std::unique_ptr<ScopedMpiSession> mpi;
    std::unique_ptr<ScopedPerfettoSession> perfetto;
};

// Set up system from input
SystemLoaded system(inp::System const&);

// Saved copy of the configured input, transitional to move away from env vars
inp::System const& loaded_system_inp();

//---------------------------------------------------------------------------//
}  // namespace setup
}  // namespace celeritas

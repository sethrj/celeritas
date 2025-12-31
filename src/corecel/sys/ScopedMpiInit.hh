//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/sys/ScopedMpiInit.hh
//---------------------------------------------------------------------------//
#pragma once

#include "ScopedMpiSession.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
//! \deprecated Remove in v1.0
using ScopedMpiInit = ScopedMpiSession;

//---------------------------------------------------------------------------//
}  // namespace celeritas

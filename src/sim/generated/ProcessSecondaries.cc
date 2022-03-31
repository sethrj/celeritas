//----------------------------------*-C++-*----------------------------------//
// Copyright 2022 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file sim/generated/ProcessSecondaries.cc
//! \note Auto-generated by gen-trackinit.py: DO NOT MODIFY!
//---------------------------------------------------------------------------//
#include "sim/detail/ProcessSecondariesLauncher.hh"

#include "base/Types.hh"

namespace celeritas
{
namespace generated
{
void process_secondaries(
    const CoreParamsHostRef& params,
    const CoreStateHostRef& states,
    const TrackInitStateHostRef& data)
{
    detail::ProcessSecondariesLauncher<MemSpace::host> launch(params, states, data);
    #pragma omp parallel for
    for (ThreadId::size_type i = 0; i < states.size(); ++i)
    {
        launch(ThreadId{i});
    }
}

} // namespace generated
} // namespace celeritas
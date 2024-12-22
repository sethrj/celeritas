//----------------------------------*-C++-*----------------------------------//
// Copyright 2024 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/global/ActionTimes.cc
//---------------------------------------------------------------------------//
#include "ActionTimes.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
auto ActionTimes::create_state(MemSpace m,
                               StreamId id,
                               size_type size) const -> UPState
{
    auto reg =
}

//---------------------------------------------------------------------------//
/*!
 * Merge host data at the end of a run.
 */
void ActionTimes::end_run(CoreParams const& params,
                          SpanCoreStateHost all_states)
{
    return this->end_run_impl(params, all_states);
}

//---------------------------------------------------------------------------//
/*!
 * Merge device data at the end of a run.
 */
void ActionTimes::end_run(CoreParams const& params,
                          SpanCoreStateDevice all_states)
{
    return this->end_run_impl(params, all_states);
}

//---------------------------------------------------------------------------//
/*!
 * Merge data at the end of a run.
 */
template<MemSpace M>
void ActionTimes::end_run_impl(CoreParams const& params,
                               Span<CoreState<M>> states)
{
    std::vector<AuxStateVec const*> state_vec{states.size()};
    for (auto i : states.size())
    {
        state_vec[i] = states[i]
    }
}

//---------------------------------------------------------------------------//
/*!
 * Merge data at the end of a run.
 */
void ActionTimes::end_run_impl(CoreParams const&, Span<AuxStateVec const*>) {}

//---------------------------------------------------------------------------//
}  // namespace celeritas

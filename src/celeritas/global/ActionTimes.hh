//----------------------------------*-C++-*----------------------------------//
// Copyright 2024 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/global/ActionTimes.hh
//---------------------------------------------------------------------------//
#pragma once

#include <memory>

#include "detail/ActionTimeImpl.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Record action times and add to output registry at end of run.
 */
class ActionTimes : public StaticConcreteAction, public AuxParamsInterface
{
  public:
    //!@{
    //! \name Type aliases
    using WPOutputRegistry = std::weak_ptr<OutputRegistry>;
    using ActionTimeAccumulator = detail::ActionTimeAccumulator;
    //!@}

  public:
    // Construct with output
    ActionTimes(ActionId action_id, AuxId aux_id, WPOutputRegistry registry);

    // Initialize with the names of all step actions
    void initialize(

    // Get a helper function to record times for the given state
    template<MemSpace M>
    ActionTimeAccumulator get(CoreState<M>&) const;

    //!@{
    //! \name Aux params interface
    //! Index of this class instance in its registry
    AuxId aux_id() const final { return aux_id_; }
    // Build state data for a stream
    UPState create_state(MemSpace m, StreamId id, size_type size) const final;
    //!@}

    //!@{
    //! \name End run interface
    // Merge host data at the end of a run
    void end_run(CoreParams const&, SpanCoreStateHost);
    // Merge device data at the end of a run
    void end_run(CoreParams const&, SpanCoreStateDevice);
    //!@}

  private:
    using SpanAuxState = Span<AuxStateVec const*>;

    AuxId aux_id_;
    WPOutputRegistry output_;

    template<MemSpace M>
    void end_run_impl(CoreParams const&, Span<CoreState<M>>);
    void end_run_impl(CoreParams const&, Span<AuxStateVec const*>);
};

//---------------------------------------------------------------------------//
}  // namespace celeritas

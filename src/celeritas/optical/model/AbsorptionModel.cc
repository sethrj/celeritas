//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/optical/model/AbsorptionModel.cc
//---------------------------------------------------------------------------//
#include "AbsorptionModel.hh"

#include "corecel/Assert.hh"
#include "corecel/cont/Range.hh"
#include "celeritas/optical/CoreParams.hh"
#include "celeritas/optical/CoreState.hh"
#include "celeritas/optical/InteractionApplier.hh"
#include "celeritas/optical/MfpBuilder.hh"
#include "celeritas/optical/action/ActionLauncher.hh"
#include "celeritas/optical/action/TrackSlotExecutor.hh"

#include "AbsorptionExecutor.hh"

namespace celeritas
{
namespace optical
{
//---------------------------------------------------------------------------//
/*!
 * Create a model builder for the optical absorption model.
 */
auto AbsorptionModel::make_builder(SPConstImported imported) -> ModelBuilder
{
    CELER_EXPECT(imported);
    return [i = std::move(imported)](ActionId id, MfpBuilder& mfp_builder) {
        return std::make_shared<AbsorptionModel>(id, i, mfp_builder);
    };
}

//---------------------------------------------------------------------------//
/*!
 * Construct the model from imported data.
 */
AbsorptionModel::AbsorptionModel(ActionId id,
                                 SPConstImported imported,
                                 MfpBuilder& mfp_builder)
    : Model(id, "absorption", "interact by optical absorption")
    , imported_(ImportModelClass::absorption, imported)
{
    for (auto mat : range(OptMatId{imported_.num_materials()}))
    {
        build_mfps(mat, mfp_builder);
    }
}

//---------------------------------------------------------------------------//
/*!
 * Build the mean free paths for the model.
 */
void AbsorptionModel::build_mfps(OptMatId mat, MfpBuilder& build) const
{
    CELER_EXPECT(mat < imported_.num_materials());
    build(imported_.mfp(mat));
}

//---------------------------------------------------------------------------//
/*!
 * Execute the model on the host.
 */
void AbsorptionModel::step(CoreParams const& params, CoreStateHost& state) const
{
    launch_action(
        state,
        make_action_thread_executor(params.ptr<MemSpace::native>(),
                                    state.ptr(),
                                    this->action_id(),
                                    InteractionApplier{AbsorptionExecutor{}}));
}

//---------------------------------------------------------------------------//
/*!
 * Execute the model on the device.
 */
#if !CELER_USE_DEVICE
void AbsorptionModel::step(CoreParams const&, CoreStateDevice&) const
{
    CELER_NOT_CONFIGURED("CUDA OR HIP");
}
#endif

//---------------------------------------------------------------------------//
}  // namespace optical
}  // namespace celeritas

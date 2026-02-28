//------------------------------ -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/optical/PhysicsParams.cc
//---------------------------------------------------------------------------//
#include "PhysicsParams.hh"

#include "corecel/sys/ActionRegistry.hh"
#include "celeritas/io/ImportData.hh"
#include "celeritas/optical/ModelImporter.hh"

#include "MaterialParams.hh"
#include "MfpBuilder.hh"
#include "Model.hh"
#include "action/DiscreteSelectAction.hh"

namespace celeritas
{
namespace optical
{
//---------------------------------------------------------------------------//
/*!
 * Construct with imported data.
 */
std::shared_ptr<PhysicsParams>
PhysicsParams::from_import(ImportData const& io,
                           SPConstCoreMaterials core_materials,
                           SPConstMaterials materials,
                           SPActionRegistry action_reg)
{
    Input input;
    input.materials = materials;
    input.action_registry = action_reg.get();
    ModelImporter importer{io, materials, core_materials};
    auto add_model
        = [&importer, &mb = input.model_builders](auto const& op_bulk_model) {
              if (auto builder = importer(op_bulk_model.model_class))
              {
                  mb.push_back(builder);
              }
          };
    add_model(io.optical_physics.bulk.absorption);
    add_model(io.optical_physics.bulk.rayleigh);
    add_model(io.optical_physics.bulk.mie);
    add_model(io.optical_physics.bulk.wls);
    add_model(io.optical_physics.bulk.wls2);
    return std::make_shared<PhysicsParams>(std::move(input));
}

//---------------------------------------------------------------------------//
/*!
 * Construct from imported and shared data.
 *
 * The following actions are first registered:
 *  - "discrete-select": sample models by XS for discrete interactions
 *
 * Optical models provided by the model builders input are then constructed,
 * registered in the action registry, and their MFP tables are built
 * concurrently during construction.
 */
PhysicsParams::PhysicsParams(Input input)
{
    CELER_EXPECT(input.materials);
    CELER_EXPECT(input.action_registry);

    // Construct data with known scalars
    HostValue data;
    data.scalars.num_materials = input.materials->num_materials();
    data.scalars.first_model_action = ActionId{1};

    // Create and register actions; build models and MFP tables concurrently
    {
        auto& action_reg = *input.action_registry;

        // Discrete select action
        discrete_select_
            = std::make_shared<DiscreteSelectAction>(action_reg.next_id());
        action_reg.insert(discrete_select_);

        // Build models (MFP tables are built during model construction)
        models_ = this->build_models(input.model_builders, action_reg, data);
    }

    data.scalars.num_models = models_.size();

    CELER_ENSURE(data);

    data_ = ParamsDataStore<PhysicsParamsData>{std::move(data)};
}

//---------------------------------------------------------------------------//
/*!
 * Construct optical models, register them in the given registry, and build
 * their MFP tables into the provided data storage.
 */
auto PhysicsParams::build_models(VecModelBuilders const& model_builders,
                                 ActionRegistry& action_reg,
                                 HostValue& data) const -> VecModels
{
    VecModels models;
    models.reserve(model_builders.size());

    for (auto const& build_model : model_builders)
    {
        CELER_ASSERT(build_model);

        auto action_id = action_reg.next_id();
        MfpBuilder mfp_builder(&data.reals, &data.grids);
        SPConstModel model = build_model(action_id, mfp_builder);
        CELER_ASSERT(model);
        CELER_ASSERT(model->action_id() == action_id);
        CELER_ASSERT(mfp_builder.grid_ids().size()
                     == data.scalars.num_materials);

        action_reg.insert(model);
        models.push_back(std::move(model));
    }

    return models;
}

//---------------------------------------------------------------------------//
}  // namespace optical
}  // namespace celeritas

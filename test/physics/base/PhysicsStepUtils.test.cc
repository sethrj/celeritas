//----------------------------------*-C++-*----------------------------------//
// Copyright 2021 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file PhysicsStepUtils.test.cc
//---------------------------------------------------------------------------//
#include "physics/base/PhysicsStepUtils.hh"

#include "base/PieStateStore.hh"
#include "physics/base/ParticleParams.hh"
#include "physics/base/PhysicsParams.hh"
#include "celeritas_test.hh"
#include "PhysicsTestBase.hh"

using namespace celeritas;
using namespace celeritas_test;
using celeritas::units::MevEnergy;

//---------------------------------------------------------------------------//
// TEST HARNESS
//---------------------------------------------------------------------------//

class PhysicsStepUtilsTest : public PhysicsTestBase
{
    using Base = PhysicsTestBase;

  protected:
    using MaterialStateStore = PieStateStore<MaterialStateData, MemSpace::host>;
    using ParticleStateStore = PieStateStore<ParticleStateData, MemSpace::host>;
    using PhysicsStateStore  = PieStateStore<PhysicsStateData, MemSpace::host>;

    void SetUp() override
    {
        Base::SetUp();

        // Construct state for a single host thread
        mat_state  = MaterialStateStore(*this->materials(), 1);
        par_state  = ParticleStateStore(*this->particles(), 1);
        phys_state = PhysicsStateStore(*this->physics(), 1);
    }

    MaterialStateStore mat_state;
    ParticleStateStore par_state;
    PhysicsStateStore  phys_state;
};

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//

TEST_F(PhysicsStepUtilsTest, calc_tabulated_physics_step)
{
    MaterialTrackView material(
        this->materials()->host_pointers(), mat_state.ref(), ThreadId{0});
    ParticleTrackView particle(
        this->particles()->host_pointers(), par_state.ref(), ThreadId{0});
    MaterialTrackView::Initializer_t mat_init;
    ParticleTrackView::Initializer_t par_init;

    // Test a variety of energy ranges and multiple material IDs
    {
        mat_init.material_id = MaterialId{0};
        par_init.energy      = MevEnergy{1};
        par_init.particle_id = this->particles()->find("gamma");
        material             = mat_init;
        particle             = par_init;
        PhysicsTrackView phys(this->physics()->host_pointers(),
                              phys_state.ref(),
                              par_init.particle_id,
                              mat_init.material_id,
                              ThreadId{0});
        phys.interaction_mfp(1);
        real_type step
            = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(1. / 3.e-4, step);
    }
    {
        mat_init.material_id = MaterialId{1};
        par_init.energy      = MevEnergy{10};
        par_init.particle_id = this->particles()->find("celeriton");
        material             = mat_init;
        particle             = par_init;
        PhysicsTrackView phys(this->physics()->host_pointers(),
                              phys_state.ref(),
                              par_init.particle_id,
                              mat_init.material_id,
                              ThreadId{0});
        phys.interaction_mfp(1e-2);
        real_type step
            = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(1.e-2 / 9.e-3, step);

        // Increase the distance to interaction so range limits the step length
        phys.interaction_mfp(1);
        step = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(4.1595999999999984, step);

        // Decrease the particle energy
        par_init.energy = MevEnergy{1e-2};
        particle        = par_init;
        step = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(2.e-2, step);
    }
    {
        mat_init.material_id = MaterialId{2};
        par_init.energy      = MevEnergy{1e-2};
        par_init.particle_id = this->particles()->find("anti-celeriton");
        material             = mat_init;
        particle             = par_init;
        PhysicsTrackView phys(this->physics()->host_pointers(),
                              phys_state.ref(),
                              par_init.particle_id,
                              mat_init.material_id,
                              ThreadId{0});
        phys.interaction_mfp(1e-2);
        real_type step
            = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(1.e-2 / 9.e-1, step);

        // Increase the distance to interaction so range limits the step length
        phys.interaction_mfp(1);
        step = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(0.03, step);

        // Increase the particle energy so interaction limits the step length
        par_init.energy = MevEnergy{10};
        particle        = par_init;
        step = celeritas::calc_tabulated_physics_step(material, particle, phys);
        EXPECT_SOFT_EQ(1. / 9.e-1, step);
    }
}

TEST_F(PhysicsStepUtilsTest, calc_energy_loss) {}

TEST_F(PhysicsStepUtilsTest, select_model) {}
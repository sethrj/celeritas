//---------------------------------*-CUDA-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file KNDemoKernel.cu
//---------------------------------------------------------------------------//
#include "KNDemoKernel.hh"

#include <thrust/device_ptr.h>
#include <thrust/reduce.h>
#include "base/ArrayUtils.hh"
#include "base/Assert.hh"
#include "base/KernelParamCalculator.cuda.hh"
#include "physics/base/ParticleTrackView.hh"
#include "physics/base/SecondaryAllocatorView.hh"
#include "physics/em/detail/KleinNishinaInteractor.hh"
#include "random/cuda/RngEngine.hh"
#include "physics/grid/PhysicsGridCalculator.hh"
#include "DetectorView.hh"
#include "KernelUtils.hh"

using namespace celeritas;
using celeritas::detail::KleinNishinaInteractor;

namespace demo_interactor
{
namespace
{
//---------------------------------------------------------------------------//
// KERNELS
//---------------------------------------------------------------------------//
/*!
 * Kernel to initialize particle data.
 */
__global__ void initialize_kernel(ParamsDeviceRef const params,
                                  StateDeviceRef const  states,
                                  InitialPointers const init)
{
    const auto tid = KernelParamCalculator::thread_id();

    if (!(tid < states.size()))
    {
        return;
    }

    ParticleTrackView particle(params.particle, states.particle, tid);
    particle = init.particle;

    // Particles begin alive and in the +z direction
    states.direction[tid.get()] = {0, 0, 1};
    states.position[tid.get()]  = {0, 0, 0};
    states.time[tid.get()]      = 0;
    states.alive[tid.get()]     = true;
}

//---------------------------------------------------------------------------//
/*!
 * Perform a single interaction per particle track.
 *
 * The interaction:
 * - Clears the energy deposition
 * - Samples the KN interaction
 * - Allocates and emits a secondary
 * - Kills the secondary, depositing its local energy
 * - Applies the interaction (updating track direction and energy)
 */
__global__ void
move_kernel(ParamsDeviceRef const params, StateDeviceRef const states)
{
    const auto tid = KernelParamCalculator::thread_id();

    // Exit if already dead
    if (!(tid < states.size() && states.alive[tid.get()]))
    {
        return;
    }

    // Construct particle accessor from immutable and thread-local data
    ParticleTrackView particle(params.particle, states.particle, tid);
    RngEngine         rng(states.rng, tid);

    // Move to collision
    PhysicsGridCalculator calc_xs(params.tables.xs, params.tables.reals);
    demo_interactor::move_to_collision(particle,
                                       calc_xs,
                                       states.direction[tid.get()],
                                       &states.position[tid.get()],
                                       &states.time[tid.get()],
                                       rng);
}

//---------------------------------------------------------------------------//
/*!
 * Perform the iteraction plus cleanup.
 *
 * The interaction:
 * - Allocates and emits a secondary
 * - Kills the secondary, depositing its local energy
 * - Applies the interaction (updating track direction and energy)
 */
__global__ void interact_kernel(ParamsDeviceRef const            params,
                                StateDeviceRef const             states,
                                SecondaryAllocatorPointers const secondaries)
{
    const auto tid = KernelParamCalculator::thread_id();

    // Exit if already dead
    if (!(tid < states.size() && states.alive[tid.get()]))
    {
        return;
    }

    // Construct particle accessor from immutable and thread-local data
    ParticleTrackView particle(params.particle, states.particle, tid);
    RngEngine         rng(states.rng, tid);

    // Construct RNG and interaction interfaces
    SecondaryAllocatorView allocate_secondaries(secondaries);
    KleinNishinaInteractor interact(
        params.kn_interactor, particle, states.direction[tid.get()], allocate_secondaries);

    // Perform interaction: should emit a single particle (an electron)
    Interaction interaction = interact(rng);
    CELER_ASSERT(interaction);
    CELER_ASSERT(interaction.secondaries.size() == 1);
    states.interactions[tid.get()] = interaction;
}

//---------------------------------------------------------------------------//
/*!
 * Perform the cutoff and detection.
 *
 * The interaction:
 * - Kills the secondary, depositing its local energy
 * - Applies the interaction (updating track direction and energy)
 */
__global__ void cutoff_kernel(ParamsDeviceRef const  params,
                              StateDeviceRef const   states,
                              DetectorPointers const detector)
{
    const auto tid = KernelParamCalculator::thread_id();

    // Exit if already dead
    if (!(tid < states.size() && states.alive[tid.get()]))
    {
        return;
    }

    // Hit stores pre-interaction state
    Hit h;
    h.dir    = states.direction[tid.get()];
    h.pos    = states.position[tid.get()];
    h.thread = tid;
    h.time   = states.time[tid.get()];

    // Construct views
    DetectorView detector_hit(detector);

    // Perform interaction: should emit a single particle (an electron)
    Interaction interaction = states.interactions[tid.get()];
    CELER_ASSERT(interaction);
    CELER_ASSERT(interaction.secondaries.size() == 1);

    // Deposit energy from the secondary (effectively, an infinite energy
    // cutoff)
    {
        const auto& secondary = interaction.secondaries.front();
        h.dir                 = secondary.direction;
        h.energy_deposited    = secondary.energy;
        detector_hit(h);
    }

    // Apply interaction
    if (interaction.energy < units::MevEnergy{0.01})
    {
        // Particle is below cutoff
        h.energy_deposited = interaction.energy;

        // Deposit energy and kill
        detector_hit(h);
        states.alive[tid.get()] = false;
    }
    else
    {
        // Apply interaction
        states.direction[tid.get()] = interaction.direction;

        ParticleTrackView particle(params.particle, states.particle, tid);
        particle.energy(interaction.energy);
    }
}

} // namespace

//---------------------------------------------------------------------------//
// KERNEL INTERFACES
//---------------------------------------------------------------------------//
/*!
 * Initialize particle states.
 */
void initialize(const CudaOptions&     opts,
                const ParamsDeviceRef& params,
                const StateDeviceRef&  states,
                const InitialPointers& initial)
{
    static const KernelParamCalculator calc_kernel_params(
        initialize_kernel, "initialize", opts.block_size);
    auto grid = calc_kernel_params(states.size());

    CELER_EXPECT(states.alive.size() == states.size());
    CELER_EXPECT(states.rng.size() == states.size());
    initialize_kernel<<<grid.grid_size, grid.block_size>>>(
        params, states, initial);
    CELER_CUDA_CHECK_ERROR();
}

//---------------------------------------------------------------------------//
/*!
 * Run an iteration.
 */
void iterate(const CudaOptions&                 opts,
             const ParamsDeviceRef&             params,
             const StateDeviceRef&              states,
             const SecondaryAllocatorPointers&  secondaries,
             const celeritas::DetectorPointers& detector)
{
    static const KernelParamCalculator calc_move_params(
        move_kernel, "move", opts.block_size);
    auto grid = calc_move_params(states.size());
    move_kernel<<<grid.grid_size, grid.block_size>>>(params, states);
    CELER_CUDA_CHECK_ERROR();

    static const KernelParamCalculator calc_interact_params(
        interact_kernel, "interact", opts.block_size);
    grid = calc_interact_params(states.size());
    interact_kernel<<<grid.grid_size, grid.block_size>>>(
        params, states, secondaries);
    CELER_CUDA_CHECK_ERROR();

    static const KernelParamCalculator calc_cutoff_params(
        cutoff_kernel, "cutoff", opts.block_size);
    grid = calc_cutoff_params(states.size());
    cutoff_kernel<<<grid.grid_size, grid.block_size>>>(
        params, states, detector);
    CELER_CUDA_CHECK_ERROR();

    if (opts.sync)
    {
        // Note: the device synchronize is useful for debugging and necessary
        // for timing diagnostics.
        CELER_CUDA_CALL(cudaDeviceSynchronize());
    }
}

//---------------------------------------------------------------------------//
/*!
 * Sum the total number of living particles.
 */
size_type reduce_alive(const CudaOptions& opts, Span<bool> alive)
{
    size_type result = thrust::reduce(
        thrust::device_pointer_cast(alive.data()),
        thrust::device_pointer_cast(alive.data() + alive.size()),
        size_type(0),
        thrust::plus<size_type>());
    CELER_CUDA_CHECK_ERROR();

    if (opts.sync)
    {
        CELER_CUDA_CALL(cudaDeviceSynchronize());
    }
    return result;
}

//---------------------------------------------------------------------------//
} // namespace demo_interactor

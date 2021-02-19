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
__global__ void iterate_kernel(ParamsDeviceRef const            params,
                               StateDeviceRef const             states,
                               SecondaryAllocatorPointers const secondaries,
                               DetectorPointers const           detector)
{
    const auto tid = KernelParamCalculator::thread_id();

    // Exit if already dead
    if (!(tid < states.size() && states.alive[tid.get()]))
    {
        return;
    }

    // Load global memory into local
    Real3     dir  = states.direction[tid.get()];
    Real3     pos  = states.position[tid.get()];
    real_type time = states.time[tid.get()];

    // Construct particle accessor from immutable and thread-local data
    ParticleTrackView particle(params.particle, states.particle, tid);
    RngEngine         rng(states.rng, tid);

    // Move to collision
    PhysicsGridCalculator calc_xs(params.tables.xs, params.tables.reals);
    demo_interactor::move_to_collision(
        particle, calc_xs, dir, &pos, &time, rng);

    if (particle.energy() < units::MevEnergy{0.01})
    {
        // Particle is below interaction energy
        Hit h;
        h.dir              = dir;
        h.pos              = pos;
        h.thread           = tid;
        h.time             = time;
        h.energy_deposited = particle.energy();

        // Deposit energy and kill
        DetectorView detector_hit(detector);
        detector_hit(h);
        states.alive[tid.get()] = false;
        return;
    }

    // Construct RNG and interaction interfaces
    SecondaryAllocatorView allocate_secondaries(secondaries);
    KleinNishinaInteractor interact(
        params.kn_interactor, particle, dir, allocate_secondaries);

    // Perform interaction: should emit a single particle (an electron)
    Interaction interaction = interact(rng);
    CELER_ASSERT(interaction);
    CELER_ASSERT(interaction.secondaries.size() == 1);

    // Deposit energy from the secondary (effectively, an infinite energy
    // cutoff)
    {
        const auto& secondary = interaction.secondaries.front();
        Hit         h;
        h.pos              = pos;
        h.thread           = tid;
        h.time             = time;
        h.dir              = secondary.direction;
        h.energy_deposited = secondary.energy;
        DetectorView detector_hit(detector);
        detector_hit(h);
    }

    // Update post-interaction state (apply interaction)
    states.position[tid.get()]  = pos;
    states.direction[tid.get()] = interaction.direction;
    states.time[tid.get()]      = time;
    particle.energy(interaction.energy);
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
    static const KernelParamCalculator calc_kernel_params(
        iterate_kernel, "iterate", opts.block_size);

    auto grid = calc_kernel_params(states.size());

    iterate_kernel<<<grid.grid_size, grid.block_size>>>(
        params, states, secondaries, detector);
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

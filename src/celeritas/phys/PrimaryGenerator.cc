//----------------------------------*-C++-*----------------------------------//
// Copyright 2020-2024 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/phys/PrimaryGenerator.cc
//---------------------------------------------------------------------------//
#include "PrimaryGenerator.hh"

#include <random>

#include "corecel/cont/Range.hh"
#include "celeritas/Units.hh"
#include "celeritas/random/distribution/DeltaDistribution.hh"
#include "celeritas/random/distribution/IsotropicDistribution.hh"
#include "celeritas/random/distribution/UniformBoxDistribution.hh"

#include "ParticleParams.hh"
#include "Primary.hh"

namespace celeritas
{
namespace
{
//---------------------------------------------------------------------------//
/*!
 * Validate the number of parameters.
 */
void check_params_size(char const* sampler,
                       std::size_t dimension,
                       DistributionOptions options)
{
    CELER_EXPECT(dimension > 0);
    std::size_t required_params = 0;
    switch (options.distribution)
    {
        case DistributionSelection::delta:
            required_params = dimension;
            break;
        case DistributionSelection::isotropic:
            required_params = 0;
            break;
        case DistributionSelection::box:
            required_params = 2 * dimension;
            break;
        default:
            CELER_ASSERT_UNREACHABLE();
    }

    CELER_VALIDATE(options.params.size() == required_params,
                   << sampler << " input parameters have "
                   << options.params.size() << " elements but the '"
                   << to_cstring(options.distribution)
                   << "' distribution needs exactly " << required_params);
}
}  // namespace

//---------------------------------------------------------------------------//
/*!
 * Return a distribution for sampling the energy.
 */
auto PrimaryGenerator::make_energy_sampler(DistributionOptions const& options)
    -> EnergySampler
{
    CELER_EXPECT(options);

    char const sampler_name[] = "energy";
    check_params_size(sampler_name, 1, options);
    auto const& p = options.params;
    switch (options.distribution)
    {
        case DistributionSelection::delta:
            return DeltaDistribution<real_type>(p[0]);
        default:
            CELER_VALIDATE(false,
                           << "invalid distribution type '"
                           << to_cstring(options.distribution) << "' for "
                           << sampler_name);
    }
}

//---------------------------------------------------------------------------//
/*!
 * Return a distribution for sampling the position.
 */
auto PrimaryGenerator::make_position_sampler(
    DistributionOptions const& options) -> PositionSampler
{
    CELER_EXPECT(options);

    char const sampler_name[] = "position";
    check_params_size(sampler_name, 3, options);
    auto const& p = options.params;
    switch (options.distribution)
    {
        case DistributionSelection::delta:
            return DeltaDistribution<Real3>(Real3{p[0], p[1], p[2]});
        case DistributionSelection::box:
            return UniformBoxDistribution<real_type>(Real3{p[0], p[1], p[2]},
                                                     Real3{p[3], p[4], p[5]});
        default:
            CELER_VALIDATE(false,
                           << "invalid distribution type '"
                           << to_cstring(options.distribution) << "' for "
                           << sampler_name);
    }
}

//---------------------------------------------------------------------------//
/*!
 * Return a distribution for sampling the direction.
 */
auto PrimaryGenerator::make_direction_sampler(
    DistributionOptions const& options) -> DirectionSampler
{
    CELER_EXPECT(options);

    char const sampler_name[] = "direction";
    check_params_size(sampler_name, 3, options);
    auto const& p = options.params;
    switch (options.distribution)
    {
        case DistributionSelection::delta:
            return DeltaDistribution<Real3>(Real3{p[0], p[1], p[2]});
        case DistributionSelection::isotropic:
            return IsotropicDistribution<real_type>();
        default:
            CELER_VALIDATE(false,
                           << "invalid distribution type '"
                           << to_cstring(options.distribution) << "' for "
                           << sampler_name);
    }
}

//---------------------------------------------------------------------------//
/*!
 * Construct from user input.
 *
 * This creates a \c PrimaryGenerator from options read from a JSON input using
 * a few predefined energy, spatial, and angular distributions (that can be
 * extended as needed).
 */
PrimaryGenerator
PrimaryGenerator::from_options(SPConstParticles particles,
                               PrimaryGeneratorOptions const& opts)
{
    CELER_EXPECT(opts);

    PrimaryGenerator::Input inp;
    inp.seed = opts.seed;
    inp.pdg = std::move(opts.pdg);
    inp.num_events = opts.num_events;
    inp.primaries_per_event = opts.primaries_per_event;
    inp.sample_energy = make_energy_sampler(opts.energy);
    inp.sample_pos = make_position_sampler(opts.position);
    inp.sample_dir = make_direction_sampler(opts.direction);
    return PrimaryGenerator(particles, inp);
}

//---------------------------------------------------------------------------//
/*!
 * Construct with options and shared particle data.
 */
PrimaryGenerator::PrimaryGenerator(SPConstParticles particles, Input const& inp)
    : num_events_(inp.num_events)
    , primaries_per_event_(inp.primaries_per_event)
    , sample_energy_(std::move(inp.sample_energy))
    , sample_pos_(std::move(inp.sample_pos))
    , sample_dir_(std::move(inp.sample_dir))
{
    CELER_EXPECT(particles);

    rng_.seed(inp.seed);
    particle_id_.reserve(inp.pdg.size());
    for (auto const& pdg : inp.pdg)
    {
        particle_id_.push_back(particles->find(pdg));
    }
}

//---------------------------------------------------------------------------//
/*!
 * Generate primary particles from a single event.
 */
auto PrimaryGenerator::operator()() -> result_type
{
    if (event_count_ == num_events_)
    {
        return {};
    }

    result_type result(primaries_per_event_);
    for (auto i : range(primaries_per_event_))
    {
        Primary& p = result[i];
        p.particle_id = particle_id_[i % particle_id_.size()];
        p.energy = units::MevEnergy{sample_energy_(rng_)};
        p.position = sample_pos_(rng_);
        p.direction = sample_dir_(rng_);
        p.time = 0;
        p.event_id = EventId{event_count_};
        p.track_id = TrackId{i};
    }
    ++event_count_;
    return result;
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

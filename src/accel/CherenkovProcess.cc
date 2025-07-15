//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file accel/CherenkovProcess.cc
//---------------------------------------------------------------------------//
#include "CherenkovProcess.hh"

#include <limits>

#include "corecel/Assert.hh"
#include "corecel/Macros.hh"
#include "celeritas/UnitTypes.hh"

#include "IntegrationBase.hh"
#include "LocalTransporter.hh"
#include "SharedParams.hh"

#include "detail/IntegrationSingleton.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Construct from shared and local Celeritas data.
 */
CherenkovProcess::CherenkovProcess(SharedParams const* shared,
                                   LocalTransporter* local)
    : G4VProcess{"Cerenkov (Celeritas)", fElectromagnetic}
    , shared_{shared}
    , local_{local}
{
    CELER_VALIDATE(shared_ && local_,
                   << "null pointers given to CherenkovProcess");
}

//---------------------------------------------------------------------------//
/*!
 * Construct from tracking manager integration.
 */
CherenkovProcess::CherenkovProcess(IntegrationBase* integration)
    : CherenkovProcess{
          &detail::IntegrationSingleton::instance().shared_params(),
          &detail::IntegrationSingleton::instance().local_transporter()}
{
    CELER_EXPECT(integration);
}

//---------------------------------------------------------------------------//
/*!
 * Pre-initialization.
 */
void CherenkovProcess::PreparePhysicsTable(G4ParticleDefinition const& particle)
{
    CELER_DISCARD(particle);
}

//---------------------------------------------------------------------------//
/*!
 * Initialization.
 */
void CherenkovProcess::BuildPhysicsTable(G4ParticleDefinition const& particle)
{
    CELER_DISCARD(particle);
    // TODO: is Celeritas ready at this point?
}

//---------------------------------------------------------------------------//
/*!
 * Save debug info.
 */
void CherenkovProcess::DumpInfo() const
{
    return G4VProcess::DumpInfo();
}

//---------------------------------------------------------------------------//
/*!
 * Print a verbose process description.
 */
void CherenkovProcess::ProcessDescription(std::ostream& outfile) const
{
    return G4VProcess::ProcessDescription(outfile);
}

//---------------------------------------------------------------------------//
/*!
 * True if we emit Cherenkov radiation from the given particle type.
 */
G4bool CherenkovProcess::IsApplicable(G4ParticleDefinition const&)
{
    CELER_NOT_IMPLEMENTED("Cherenkov");
}

//---------------------------------------------------------------------------//
/*!
 * Force emission of photons in optical materials if superluminal.
 *
 * \todo Unlike G4Cerenkov we do not have a step limiter. We can add that to
 * Celeritas as part of the main tracking loop and then call it from here.
 */
G4double
CherenkovProcess::PostStepGetPhysicalInteractionLength(G4Track const& track,
                                                       G4double max_step,
                                                       G4ForceCondition* force)
{
    CELER_EXPECT(force);
    CELER_DISCARD(track);
    CELER_NOT_IMPLEMENTED("Cherenkov");

    // TODO: Convert G4Material to Celeritas optical material
    opt_mat_ = {};

    if (opt_mat_)
    {
        // Applicability means it should always be charged right?
        constexpr bool is_charged{true};
        CELER_ASSERT(is_charged);
        constexpr bool is_superluminal = false;
        if (is_superluminal)
        {
            // Always try to generate, even if nothing may hapen
            *force = StronglyForced;
            return max_step;
        }
    }

    *force = NotForced;
    return std::numeric_limits<double>::max();
}

//---------------------------------------------------------------------------//
/*!
 * Generate Cerenkov photon distributions and send to Celeritas.
 *
 * \pre The GPIL method should have set opt_mat_ .
 */
G4VParticleChange*
CherenkovProcess::PostStepDoIt(G4Track const& track, G4Step const& step_data)
{
    CELER_DISCARD(track);

    // Step length
    using Length = Quantity<units::ClhepTraits::Length, double>;
    real_type step_length
        = native_value_from(Length{step_data.GetStepLength()});
    CELER_DISCARD(step_length);

    /*
     * \todo
     * - convert pre-step data
     * - get optical material view using optical ID
     * - use GeantParticleView etc to wrap track/step data
     * - build celeritas RNG wrapper for CLHEP::HepRandomEngine
     */
    return nullptr;
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file accel/CherenkovProcess.hh
//---------------------------------------------------------------------------//
#pragma once

#include <G4VProcess.hh>

#include "celeritas/Types.hh"

namespace celeritas
{
class LocalTransporter;
class SharedParams;
class IntegrationBase;
//---------------------------------------------------------------------------//
/*!
 * Generate Cherenkov photons into Celeritas from superluminal particles.
 *
 * This process requires Celeritas to be set up with tracking manager \em user
 * action integration, which will call \c LocalTransporter::Flush at the end of
 * an event to create photons.
 */
class CherenkovProcess final : public G4VProcess
{
  public:
    //!@{
    //! \name Type aliases
    using LocalTransporterFromThread = std::function<LocalTransporter*(int)>;
    //!@}

  public:
    // Construct from shared and local Celeritas data
    CherenkovProcess(SharedParams const* shared, LocalTransporter* local);

    // Construct from Celeritas tracking/user integration class
    explicit CherenkovProcess(IntegrationBase* integration);

    //// CONSTRUCTION ////

    // Pre-initialization
    void PreparePhysicsTable(G4ParticleDefinition const&) final;

    // Initialization
    void BuildPhysicsTable(G4ParticleDefinition const&) final;

    //// DEBUGGING ////

    // Print info to screen
    void DumpInfo() const final;
    void ProcessDescription(std::ostream& outfile) const final;

    //// INTERACTION ////

    G4bool IsApplicable(G4ParticleDefinition const&) final;

    G4double
    PostStepGetPhysicalInteractionLength(G4Track const& track,
                                         G4double previousStepSize,
                                         G4ForceCondition* condition) final;

    G4VParticleChange*
    PostStepDoIt(G4Track const& track, G4Step const& stepData) final;

    //!@{
    //! No at-rest action
    G4double
    AtRestGetPhysicalInteractionLength(G4Track const&, G4ForceCondition*) final
    {
        return -1;
    }
    G4VParticleChange* AtRestDoIt(G4Track const&, G4Step const&) final
    {
        return nullptr;
    }
    //!@}

    //!@{
    //! No along-step action
    G4double AlongStepGetPhysicalInteractionLength(
        G4Track const&, G4double, G4double, G4double&, G4GPILSelection*) final
    {
        return -1;
    }
    G4VParticleChange* AlongStepDoIt(G4Track const&, G4Step const&) final
    {
        return nullptr;
    }
    //!@}

  private:
    //// SHARED DATA ////

    SharedParams const* shared_{nullptr};
    // TODO: optical materials
    // TODO: cherenkov params

    //// LOCAL DATA ////

    LocalTransporter* local_{nullptr};
    OptMatId opt_mat_;
};

//---------------------------------------------------------------------------//
}  // namespace celeritas

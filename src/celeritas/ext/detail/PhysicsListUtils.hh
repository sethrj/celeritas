//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file celeritas/ext/detail/PhysicsListUtils.hh
//---------------------------------------------------------------------------//
#pragma once

#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include <G4VModularPhysicsList.hh>
#include <G4VPhysicsConstructor.hh>

#include "corecel/io/Logger.hh"
#include "corecel/sys/TypeDemangler.hh"

namespace celeritas
{
namespace detail
{
namespace
{
void log_physics_construction(G4VPhysicsConstructor const& physics,
                              std::string_view what)
{
    TypeDemangler<G4VPhysicsConstructor> demangle;
    CELER_LOG_LOCAL(debug) << demangle(physics) << " '"
                           << physics.GetPhysicsName() << "': constructing "
                           << what;
}

//! Wrapper class to log before doing local particle/process construction
template<class T>
class DebugModularPhysicsWrapper : public T
{
    static_assert(std::is_base_of_v<G4VPhysicsConstructor, T>);

  public:
    // Use base connstructors
    using T::T;

    // Set up minimal EM particle list
    void ConstructParticle() final
    {
        log_physics_construction(*this, "particles");
        T::ConstructParticle();
    }

    // Set up process list
    void ConstructProcess() final
    {
        log_physics_construction(*this, "processes");
        T::ConstructProcess();
    }
};

}  // namespace

//---------------------------------------------------------------------------//
/*!
 * Create a suitably owned physics class and add it to the physics list.
 */
template<class PL, class... T>
void emplace_physics(G4VModularPhysicsList& list, T&&... args)
{
    // Construct physics with wrapper
    auto phys = std::make_unique<DebugModularPhysicsWrapper<PL>>(
        std::forward<T>(args)...);

    CELER_LOG(debug) << "Registering physics: " << phys->GetPhysicsName();

    // Register, passing ownership to the list
    list.RegisterPhysics(phys.release());
}

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

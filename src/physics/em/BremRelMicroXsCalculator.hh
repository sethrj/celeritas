//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file BremRelMicroXsCalculator.hh
//---------------------------------------------------------------------------//
#pragma once

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Calculate relativistic bremsstrahlung cross sections.
 *
 * Optional detailed class description, and possibly example usage:
 * \code
    BremRelMicroXsCalculator ...;
   \endcode
 */
class BremRelMicroXsCalculator
{
  public:
    //@{
    //! Type aliases
    using MevEnergy = units::MevEnergy;
    //@}

  public:
    // Construct with material properties and particle enrgy
    inline CELER_FUNCTION
    BremRelMicroXsCalculator(const BremRelInteractorPointers& shared,
                             const MaterialView&              mat,
                             const ParticleTrackView&         particle);

    // Compute
    inline CELER_FUNCTION real_type operator()(ElementDefId el_id) const;

    // Characteristic energy for LPM effects [MeV]
    CELER_FUNCTION real_type lpm_energy() const
    {
        return shared_.lpm_constant * mat_.radiation_length();
    }

  private:
    //@{
    //! Material-dependent properties
    // Material reference
    const MaterialView& mat_;
    // Density correction for this material [MeV^2]
    real_type density_corr_;
    //@}

    //@{
    //! State-dependent properties
    // Incident gamma energy (MeV)
    real_type inc_energy_;
    // Incident direction
    const Real3& inc_direction_;
    // Whether LPM correction is used for the material and energy
    bool use_lpm_;
    //@}
};

//---------------------------------------------------------------------------//
// INLINE MEMBER FUNCTIONS
//---------------------------------------------------------------------------//
CELER_FUNCTION BremRelMicroXsCalculator::BremRelMicroXsCalculator(
    const BremRelInteractorPointers& shared,
    const MaterialView&              mat,
    const ParticleTrackView&         particle)
    : shared_(shared)
    , mat_(mat)
    , inc_energy_(particle.energy().value())
    , use_lpm_(shared.use_lpm)
{
    REQUIRE(particle.def_id() == shared_.electron_id);

    real_type density_factor = shared_.migdal_constant
                               * mat_.electron_density();
    if (use_lpm_)
    {
        real_type threshold = std::sqrt(density_factor) * this->lpm_energy();
        if (particle.energy().value() < threshold)
        {
            // Energy is below material-based cutoff
            use_lpm_ = false;
        }
    }
}

//---------------------------------------------------------------------------//
} // namespace celeritas

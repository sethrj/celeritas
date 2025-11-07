//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file accel/GeantStepDiagnostic.hh
//---------------------------------------------------------------------------//
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <G4Track.hh>

#include "corecel/Types.hh"
#include "corecel/io/OutputInterface.hh"
#include "corecel/sys/ThreadId.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Tally the steps per track transported with Geant4 for each particle type.
 *
 * For the diagnostic class that collects the same result for tracks
 * transported with Celeritas, see: \sa StepDiagnostic.
 */
class GeantStepDiagnostic final : public OutputInterface
{
  public:
    //!@{
    //! \name Type aliases
    using VecCount = std::vector<size_type>;
    using VecVecCount = std::vector<VecCount>;
    using MapIntVecCount = std::unordered_map<int, VecCount>;
    //!@}

  public:
    // Construct with number of bins and streams
    GeantStepDiagnostic(size_type num_bins, StreamId::size_type num_streams);

    //!@{
    //! \name Output interface

    //! Category of data to write
    Category category() const final { return Category::result; }
    //! Key for the entry inside the category.
    std::string_view label() const final { return "g4-step-diagnostic"; }
    // Write output to the given JSON object
    void output(JsonPimpl*) const final;
    //!@}

    // Update the step count from the given track
    void Update(G4Track const* track);

    // Get the results accumulated over all streams
    VecVecCount CalcSteps() const;

    // Get a sorted vector of PDGs
    std::vector<int> GetPDGs() const;

  private:
    std::vector<MapIntVecCount> stream_store_;
    size_type num_bins_;
};

//---------------------------------------------------------------------------//
}  // namespace celeritas

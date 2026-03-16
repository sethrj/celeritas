//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file geocel/VolumePathAccumulator.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Assert.hh"
#include "corecel/Macros.hh"

#include "VolumeData.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Incrementally compute the unique ID of a path in the volume hierarchy.
 *
 * Each \c VolumeUniqueInstanceId uniquely identifies a root-to-node path
 * (i.e., a Geant4 "touchable") in the volume DAG.  This class computes the ID
 * on the fly without allocating a path buffer: call \c operator() once for
 * each \c VolumeInstanceId encountered while descending from the world volume,
 * passing the current accumulated ID and receiving the updated one.
 *
 * The mapping relies on the \c unique_instance_offsets precomputed in
 * \c VolumeParamsData (see \c VolumeData.hh for the mathematical definition).
 * For a path \f$[vi_0, vi_1, \ldots, vi_k]\f$ the unique instance ID is
 * \f[
 *   \text{uid} = \sum_{i=0}^{k} \bigl(\text{offset}[vi_i] + 1\bigr).
 * \f]
 * The empty path (the world volume itself, with no enclosing instance) maps
 * to ID 0 (see \c world_unique_instance ).
 *
 * \code
   VolumePathAccumulator accum{params.host_ref()};
   VolumeUniqueInstanceId uid = world_unique_instance;
   for (VolumeInstanceId vi : path_below_world)
   {
       uid = accum(uid, vi);  // unique ID for the node reached via this step
   }
 * \endcode
 */
class VolumePathAccumulator
{
  public:
    //!@{
    //! \name Type aliases
    using ParamsRef = NativeCRef<VolumeParamsData>;
    //!@}

  public:
    //! Construct from volume hierarchy data
    explicit CELER_FUNCTION VolumePathAccumulator(ParamsRef const& params)
        : params_(params)
    {
    }

    // Descend one level: add offset[vi]+1 to uid and return the result
    inline CELER_FUNCTION VolumeUniqueInstanceId
    operator()(VolumeUniqueInstanceId uid, VolumeInstanceId vi) const;

  private:
    ParamsRef const& params_;
};

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Accumulate the contribution of one path step and return the updated ID.
 *
 * The result after \f$k\f$ calls equals the \c VolumeUniqueInstanceId of the
 * node reached via the first \f$k\f$ instances in the path.
 */
CELER_FUNCTION VolumeUniqueInstanceId VolumePathAccumulator::operator()(
    VolumeUniqueInstanceId uid, VolumeInstanceId vi) const
{
    CELER_EXPECT(vi < params_.unique_instance_offsets.size());
    auto const offset = params_.unique_instance_offsets[vi].unchecked_get();
    return VolumeUniqueInstanceId{uid.unchecked_get() + offset + 1};
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

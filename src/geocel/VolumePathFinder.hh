//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file geocel/VolumePathFinder.hh
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Assert.hh"
#include "corecel/Macros.hh"
#include "corecel/cont/Span.hh"

#include "VolumeData.hh"
#include "VolumeView.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Reconstruct the volume-instance path for a \c VolumeUniqueInstanceId.
 *
 * Each \c VolumeUniqueInstanceId uniquely identifies a root-to-node path
 * (i.e., a Geant4 "touchable") in the volume DAG.  This class performs the
 * inverse of \c VolumeUniqueInstanceAccumulator: given an ID it fills a
 * caller-supplied scratch buffer with the \c VolumeInstanceId sequence and
 * returns a (possibly shorter) span of the result.
 *
 * The algorithm descends from the world volume level by level.  At each
 * level it scans the current volume's children to find the unique child whose
 * subtree contains the remaining UID, exploiting the fact that sibling offsets
 * are strictly increasing.  The cost is \f$O(D \cdot C)\f$ where \f$D\f$ is
 * the path depth and \f$C\f$ is the maximum number of children of any volume.
 *
 * The scratch buffer must be at least \c num_volume_levels long (the
 * maximum possible path depth).  Successive calls reuse the same buffer, so
 * callers must consume the returned span before the next call.
 *
 * \code
   std::vector<VolumeInstanceId> buf(params.num_volume_levels());
   VolumePathFinder find_path{params.host_ref(), make_span(buf)};

   VolumeUniqueInstanceId uid = ...;
   auto path = find_path(uid);   // [vi_0, vi_1, ..., vi_k]
 * \endcode
 */
class VolumePathFinder
{
  public:
    //!@{
    //! \name Type aliases
    using ParamsRef = NativeCRef<VolumeParamsData>;
    using SpanVI = Span<VolumeInstanceId>;
    //!@}

  public:
    //! Construct from volume hierarchy data and a scratch buffer
    CELER_FUNCTION VolumePathFinder(ParamsRef const& params, SpanVI scratch)
        : params_(params), scratch_(scratch)
    {
        CELER_EXPECT(scratch_.size() == params_.num_volume_levels);
    }

    // Reconstruct the path whose unique instance ID equals uid
    inline CELER_FUNCTION SpanVI operator()(VolumeUniqueInstanceId uid) const;

  private:
    ParamsRef const& params_;
    SpanVI scratch_;
};

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Reconstruct the path whose unique instance ID equals \c uid.
 *
 * Returns an empty span for UID 0 (the world volume, before any instance is
 * entered).  Otherwise writes the path starting from the world's enclosing
 * instance down to the node identified by \c uid, and returns a sub-span of
 * the scratch buffer containing exactly those entries.
 */
CELER_FUNCTION auto
VolumePathFinder::operator()(VolumeUniqueInstanceId uid) const -> SpanVI
{
    using size_type = VolumeUniqueInstanceId::size_type;

    size_type remaining = uid.unchecked_get();
    // Start from the parents of the world volume (typically just world_PV)
    auto current_vis = VolumeView{params_, params_.world}.parents();
    size_type depth = 0;

    while (remaining > 0)
    {
        CELER_ASSERT(depth < scratch_.size());
        CELER_ASSERT(!current_vis.empty());

        // Sibling offsets are a strict prefix sum (offset[vi_0] = 0 always
        // satisfies the condition when remaining >= 1).  Scan forward to find
        // the last child whose offset is still less than remaining.
        VolumeInstanceId chosen{};
        for (VolumeInstanceId vi : current_vis)
        {
            if (params_.unique_instance_offsets[vi].unchecked_get() < remaining)
                chosen = vi;
            else
                break;
        }
        CELER_ASSERT(chosen);

        scratch_[depth++] = chosen;
        remaining -= params_.unique_instance_offsets[chosen].unchecked_get()
                     + size_type{1};
        current_vis
            = VolumeView{params_, params_.volume_ids[chosen]}.children();
    }

    return scratch_.first(depth);
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/detail/BIHBuilder.hh
//---------------------------------------------------------------------------//
#pragma once

#include <set>
#include <variant>
#include <vector>

#include "corecel/Types.hh"
#include "corecel/cont/Array.hh"
#include "corecel/cont/EnumArray.hh"
#include "corecel/data/CollectionBuilder.hh"
#include "geocel/BoundingBox.hh"  // IWYU pragma: keep
#include "orange/OrangeTypes.hh"
#include "orange/detail/BIHData.hh"

#include "../inp/Bih.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Builder-local representation of a BIH internal node.
 */
struct BIHInternalNode
{
    struct Edge
    {
        //! The position of the bounding plane along the partition axis
        fast_real_type bounding_plane_pos{};
        //! The child node connected to this edge
        BIHNodeId child;
        //! Bbox created by clipping an inf bbox with the bounding planes
        //! between this edge (inclusive) and the root.
        FastBBox bbox;
    };

    Axis axis;  //!< Axis that the partition is performed on
    EnumArray<BihSide, Edge> edges;  //!< Left/right edges

    explicit CELER_FUNCTION operator bool() const
    {
        return this->edges[BihSide::left].child
               && this->edges[BihSide::right].child;
    }
};

//---------------------------------------------------------------------------//
/*!
 * Create a bounding interval hierarchy from the supplied bounding boxes.
 *
 * This implementation matches the structure proposed in the original paper
 * \citep{wachter-bih-2006, https://doi.org/10.2312/EGWR/EGSR06/139-149}.
 * Construction is done recursively. With each recursion, partitioning is done
 * on the basis of bounding box centers using the "longest dimension"
 * heuristic. Leaf nodes are created when one of the following criteria are
 * met:
 *
 * 1) the number of remaining bounding boxes is max_leaf_size or fewer,
 * 2) the remaining bounding boxes are non-partitionable (i.e., they all have
 *    the same center),
 * 3) the current recursion depth has reached the `depth_limit`.
 *
 * Any bounding boxes that have at least one infinite dimension are not stored
 * on the tree, but rather a separate `inf_vols` structure. In the event that
 * all bounding boxes are infinite, the tree will consist of a single empty
 * leaf node with all volumes in the stored `inf_vols`. This final case should
 * not occur unless an ORANGE geometry is created via a method where volume
 * bounding boxes are not available.
 *
 * Bounding boxes supplied to this builder should "bumped," i.e. expanded
 * outward by at least floating-point epsilson from the volumes they bound.
 * This eliminates the possibility of accidentally missing a volume during
 * tracking.
 */
class BIHBuilder
{
  public:
    //!@{
    //! \name Type aliases
    using VecBBox = std::vector<FastBBox>;
    using Storage = BIHTreeData<Ownership::value, MemSpace::host>;
    using SetLocalVolId = std::set<LocalVolumeId>;
    using Input = inp::BIHBuilder;
    //!@}

  public:
    // Construct from Storage and Input objects
    BIHBuilder(Storage* storage, Input inp);

    // Create BIH Nodes
    BIHTreeRecord
    operator()(VecBBox&& bboxes, SetLocalVolId const& implicit_vol_id);

  private:
    /// TYPES ///

    using Real3 = Array<fast_real_type, 3>;
    using VecIndices = std::vector<LocalVolumeId>;
    using VecNodes = std::vector<std::variant<BIHInternalNode, BIHLeafNode>>;
    using VecInnerNodes = std::vector<BIHInternalNode>;
    using VecLeafNodes = std::vector<BIHLeafNode>;
    using VecNodeBboxes = std::vector<FastBBox>;

    struct ReorderedNodes
    {
        VecInnerNodes inner;
        VecLeafNodes leaf;
        VecNodeBboxes bboxes;
    };

    struct Temporaries
    {
        VecBBox bboxes;
        std::vector<Real3> centers;
    };

    //// DATA ////
    Temporaries temp_;

    CollectionBuilder<FastBBox> bboxes_;
    CollectionBuilder<FastBBox> node_bboxes_;
    CollectionBuilder<LocalVolumeId> local_volume_ids_;
    CollectionBuilder<Axis> axes_;
    CollectionBuilder<fast_real_type> bounding_planes_;
    CollectionBuilder<BIHNodeId> children_;
    CollectionBuilder<BIHLeafNode> leaf_nodes_;

    Input inp_;

    //// HELPER FUNCTIONS ////

    // Recursively construct BIH nodes for a vector of bbox indices
    void construct_tree(VecIndices const& indices,
                        VecNodes* nodes,
                        VecNodeBboxes* node_bboxes,
                        size_type current_depth,
                        size_type& depth);

    // Separate nodes into inner and leaf vectors and renumber accordingly
    ReorderedNodes arrange_nodes(VecNodes const& nodes,
                                 VecNodeBboxes const& node_bboxes) const;
};

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

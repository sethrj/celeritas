//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/detail/BIHData.hh
//! \todo move to orange/BihTreeData
//---------------------------------------------------------------------------//
#pragma once

#include "corecel/Types.hh"
#include "corecel/data/Collection.hh"
#include "geocel/BoundingBox.hh"  // IWYU pragma: keep

#include "../OrangeTypes.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
//! Side of a BIH internal-node partition
enum class BihSide
{
    left,
    right,
    size_
};

//---------------------------------------------------------------------------//!
// The maximum depth of the BIH tree (single leaf node is 1)
inline constexpr size_type max_bih_depth = 18;

//---------------------------------------------------------------------------//
/*!
 * Data for a single leaf node in a Bounding Interval Hierarchy.
 */
struct BIHLeafNode
{
    ItemRange<LocalVolumeId> vol_ids;

    explicit CELER_FUNCTION operator bool() const { return !vol_ids.empty(); }
};

//---------------------------------------------------------------------------//
/*!
 * Bounding Interval Hierarchy tree.
 *
 * Infinite bounding boxes are not included in the tree itself. They are stored
 * separately and checked after traversing the tree.
 */
struct BIHTreeRecord
{
    //// TYPES ////
    struct Metadata
    {
        //! The number of finite bounding boxes in the tree
        LocalVolumeId::size_type num_finite_bboxes{};
        //! The number of infinite bounding boxes, i.e., those not included in
        //! the tree itself.
        LocalVolumeId::size_type num_infinite_bboxes{};
        //! The depth of the most embedded leaf node. This has a value of 1
        //! when the root node is a leaf.
        size_type depth{};
    };

    using node_difference_type = BIHNodeId::difference_type;

    //// DATA ////

    //! All bounding boxes managed by the BIH
    ItemMap<LocalVolumeId, FastBBoxId> bboxes;

    //! All node bounding boxes managed by the BIH
    ItemMap<BIHNodeId, FastBBoxId> node_bboxes;

    //! Internal-node slots, the first being the root
    ItemRange<Axis> internal_nodes;

    //! Add to BIHNodeId of leaf node to get ItemId<BIHLeafNodeId>
    node_difference_type first_leaf_node_id{};
    //! Number of leaf nodes
    size_type num_leaf_nodes{};

    //! Local volumes that have infinite bounding boxes
    ItemRange<LocalVolumeId> inf_vol_ids;

    //! The metadata for this tree
    Metadata metadata;

    //// METHODS ////

    explicit CELER_FUNCTION operator bool() const
    {
        return !bboxes.empty() && !node_bboxes.empty() && num_leaf_nodes > 0;
    }
};

//---------------------------------------------------------------------------//
/*!
 * Persistent data used by all BIH trees.
 */
template<Ownership W, MemSpace M>
struct BIHTreeData
{
    template<class T>
    using Items = Collection<T, W, M>;

    // Low-level storage
    Items<FastBBox> bboxes;
    Items<FastBBox> node_bboxes;
    Items<LocalVolumeId> local_volume_ids;
    Items<Axis> axes;
    Items<fast_real_type> bounding_planes;
    Items<BIHNodeId> children;
    Items<FastBBox> fast_bboxes;
    Items<detail::BIHLeafNode> leaf_nodes;

    //! True if assigned
    explicit CELER_FUNCTION operator bool() const
    {
        // Note that internal-node arrays may be empty for single-node trees
        return !bboxes.empty() && !local_volume_ids.empty()
               && !node_bboxes.empty() && !leaf_nodes.empty()
               && node_bboxes.size() == axes.size() + leaf_nodes.size()
               && 2 * axes.size() == bounding_planes.size()
               && 2 * axes.size() == children.size()
               && 2 * axes.size() == fast_bboxes.size();
    }

    //! Assign from another set of data
    template<Ownership W2, MemSpace M2>
    BIHTreeData& operator=(BIHTreeData<W2, M2> const& other)
    {
        bboxes = other.bboxes;
        node_bboxes = other.node_bboxes;
        local_volume_ids = other.local_volume_ids;
        axes = other.axes;
        bounding_planes = other.bounding_planes;
        children = other.children;
        fast_bboxes = other.fast_bboxes;
        leaf_nodes = other.leaf_nodes;

        CELER_ENSURE(static_cast<bool>(*this) == static_cast<bool>(other));
        return *this;
    }
};

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

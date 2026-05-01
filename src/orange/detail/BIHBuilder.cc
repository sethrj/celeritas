//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/detail/BIHBuilder.cc
//---------------------------------------------------------------------------//
#include "BIHBuilder.hh"

#include "corecel/cont/VariantUtils.hh"
#include "geocel/BoundingBox.hh"
#include "orange/OrangeTypes.hh"

#include "BIHPartitioner.hh"
#include "../BoundingBoxUtils.hh"

namespace celeritas
{
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * \brief Constructor.
 *
 * \param[in] storage  Struct containing collections of persistent data for
 *                     all BIH trees
 * \param[in] inp      Input options that govern BIH construction, i.e.,
 *                     the maximum leaf size and the recursion depth limit
 */
BIHBuilder::BIHBuilder(Storage* storage, Input inp)
    : bboxes_{&storage->bboxes}
    , local_volume_ids_{&storage->local_volume_ids}
    , inner_nodes_{&storage->inner_nodes}
    , leaf_nodes_{&storage->leaf_nodes}
    , inp_{inp}
{
    CELER_EXPECT(storage);
    CELER_EXPECT(inp_);
}

//---------------------------------------------------------------------------//
/*!
 * \brief Build a BIH tree for the supplied bounding boxes.
 *
 * \param[in] bboxes            All bounding boxes to be included in the tree
 * \param[in] implicit_vol_ids  The ids of the "background" volumes, to be
 *                              excluded from the tree
 *
 * \return The record of the resultant BIH tree
 */
BIHTreeRecord
BIHBuilder::operator()(VecBBox&& bboxes,
                       BIHBuilder::SetLocalVolId const& implicit_vol_ids)
{
    CELER_EXPECT(!bboxes.empty());

    // Store bounding boxes and their corresponding centers
    temp_vols_.bboxes = std::move(bboxes);
    temp_vols_.centers.resize(temp_vols_.bboxes.size());
    std::transform(temp_vols_.bboxes.begin(),
                   temp_vols_.bboxes.end(),
                   temp_vols_.centers.begin(),
                   &celeritas::calc_center<fast_real_type>);

    // Separate infinite bounding boxes from finite
    VecIndices indices;
    VecIndices inf_vol_ids;
    for (auto i : range(temp_vols_.bboxes.size()))
    {
        LocalVolumeId id(i);

        if (implicit_vol_ids.find(id) != implicit_vol_ids.end())
        {
            // Background volume, do not include bbox in tree
        }
        else if (is_infinite(temp_vols_.bboxes[i]))
        {
            // Infinite in *every* direction
            /*!
             * \todo make an exception for "EXTERIOR" volume and remove the
             * "infinite volume" exceptions?
             */
            inf_vol_ids.push_back(id);
        }
        else
        {
            // Prohibit semi-infinite bounding boxes because those break the
            // cost function
            CELER_ASSERT(is_finite(temp_vols_.bboxes[i]));
            indices.push_back(id);
        }
    }

    BIHTreeRecord tree;

    tree.vol_bboxes = ItemMap<LocalVolumeId, FastBBoxId>(bboxes_.insert_back(
        temp_vols_.bboxes.begin(), temp_vols_.bboxes.end()));
    tree.inf_vol_ids = local_volume_ids_.insert_back(inf_vol_ids.begin(),
                                                     inf_vol_ids.end());

    // The depth of the most embedded node (where 1 is the root node), to be
    // calculated during the recursive construction process
    size_type depth = 0;

    if (!indices.empty())
    {
        // Construct the tree recursively
        this->construct_nodes(indices, BIHNodeId{}, 0, depth);
        auto nodes = this->calc_reordered_nodes();

        tree.inner_nodes
            = inner_nodes_.insert_back(nodes.inner.begin(), nodes.inner.end());
        tree.leaf_nodes
            = leaf_nodes_.insert_back(nodes.leaf.begin(), nodes.leaf.end());
        tree.node_bboxes = ItemMap<BIHNodeId, FastBBoxId>{
            bboxes_.insert_back(nodes.bboxes.begin(), nodes.bboxes.end())};
    }
    else
    {
        // Degenerate case where all bounding boxes are infinite. Create a
        // single empty leaf node, so that the existence of leaf nodes does not
        // need to be checked at runtime.
        BIHLeafNode const empty_nodes[] = {{}};
        tree.leaf_nodes = leaf_nodes_.insert_back(std::begin(empty_nodes),
                                                  std::end(empty_nodes));
        FastBBox const inf_bboxes[] = {FastBBox::from_infinite()};
        tree.node_bboxes = ItemMap<BIHNodeId, FastBBoxId>{
            bboxes_.insert_back(std::begin(inf_bboxes), std::end(inf_bboxes))};
    }
    temp_nodes_.records.clear();
    temp_nodes_.bboxes.clear();

    // Assign metadata for diagnostic purposes
    BIHTreeRecord::Metadata md;
    md.num_finite_bboxes = indices.size();
    md.num_infinite_bboxes = inf_vol_ids.size();
    md.depth = depth;
    tree.metadata = md;

    return tree;
}

//---------------------------------------------------------------------------//
// HELPER FUNCTIONS
//---------------------------------------------------------------------------//
/*!
 * Recursively construct BIH nodes for a vector of bbox indices.
 *
 * \param[in] indices        The indices of the bboxes that will be partitioned
 *                           or placed on a leaf node in this function call
 * \param[in, out] nodes     All nodes constructed so far, to be added to
 * \param[in] parent         The parent node
 * \param[in] current_depth  The recursion depth of this function call
 * \param[in,out] tree_depth The maximum recursion depth encountered during the
 *                           full construction process
 * \return Constructed node ID
 */
BIHNodeId BIHBuilder::construct_nodes(VecIndices const& indices,
                                      BIHNodeId parent,
                                      size_type current_depth,
                                      size_type& tree_depth)
{
    CELER_EXPECT(current_depth < inp_.depth_limit);

    using Side = BIHInnerNode::Side;

    ++current_depth;
    auto node_id = id_cast<BIHNodeId>(temp_nodes_.records.size());
    temp_nodes_.records.resize(*node_id + 1);
    temp_nodes_.bboxes.resize(*node_id + 1);

    // Create a single leaf containing all bboxes. This lambda is used only
    // once per call to construct_tree.
    auto make_leaf = [&]() {
        BIHLeafNode node;
        node.parent = parent;
        node.vol_ids
            = local_volume_ids_.insert_back(indices.begin(), indices.end());
        CELER_EXPECT(node);
        temp_nodes_.records[*node_id] = node;
        tree_depth = std::max(tree_depth, current_depth);
    };

    if (indices.size() <= inp_.max_leaf_size
        || current_depth == inp_.depth_limit)
    {
        // All bboxes fit on a single leaf, or we have reached the depth limit;
        // make a leaf and exit early
        make_leaf();
        return node_id;
    }

    BIHPartitioner partition(
        &temp_vols_.bboxes, &temp_vols_.centers, inp_.num_part_cands);

    if (auto p = partition(indices))
    {
        // Create inner node
        BIHInnerNode node;
        node.parent = parent;
        node.axis = p.axis;

        // Recursively construct the left and right branches
        for (auto side : range(Side::size_))
        {
            // Populate bounding plane:
            // TODO: replace Side with Bound and define flip
            auto pos = p.bboxes[side].point(
                side == Side::left ? Bound::hi : Bound::lo, p.axis);
            node.edges[side].bounding_plane_pos = pos;

            auto child_id = this->construct_nodes(
                p.indices[side], node_id, current_depth, tree_depth);
            node.edges[side].child = child_id;
            temp_nodes_.bboxes[*child_id] = p.bboxes[side];
        }

        CELER_EXPECT(node);
        temp_nodes_.records[*node_id] = std::move(node);
    }
    else
    {
        // Bboxes cannot be partitioned; put them all on a single leaf
        make_leaf();
    }
    return node_id;
}

//---------------------------------------------------------------------------//
/*!
 * Separate inner nodes from leaf nodes and renumber accordingly.
 *
 * \param[in] nodes  The interspersed inner and leaf nodes
 *
 * \returns  The separated inner and leaf nodes
 */
auto BIHBuilder::calc_reordered_nodes() const -> ReorderedNodes
{
    std::vector<bool> is_leaf;
    std::vector<std::size_t> new_indices;

    is_leaf.reserve(temp_nodes_.records.size());
    new_indices.reserve(temp_nodes_.records.size());

    ReorderedNodes result;

    auto insert_node = Overload{[&](BIHInnerNode const& node) {
                                    new_indices.push_back(result.inner.size());
                                    result.inner.push_back(node);
                                    is_leaf.push_back(false);
                                },
                                [&](BIHLeafNode const& node) {
                                    new_indices.push_back(result.leaf.size());
                                    result.leaf.push_back(node);
                                    is_leaf.push_back(true);
                                }};
    for (auto const& node : temp_nodes_.records)
    {
        std::visit(insert_node, node);
    }

    // Transform "leaf ID" to "node ID"
    auto offset = result.inner.size();
    for (auto i : range(temp_nodes_.records.size()))
    {
        if (is_leaf[i])
        {
            new_indices[i] += offset;
        }
    }

    // Remap IDs. "parent" will only be undefined for the root node.
    auto remapped_id = [&new_indices](BIHNodeId old) {
        CELER_EXPECT(old < new_indices.size());
        return id_cast<BIHNodeId>(new_indices[old.unchecked_get()]);
    };

    for (auto& inner_node : result.inner)
    {
        for (auto& edge : inner_node.edges)
        {
            edge.child = remapped_id(edge.child);
        }
        if (inner_node.parent)
        {
            inner_node.parent = remapped_id(inner_node.parent);
        }
    }

    for (auto& leaf_node : result.leaf)
    {
        if (leaf_node.parent)
        {
            leaf_node.parent = remapped_id(leaf_node.parent);
        }
    }

    result.bboxes.resize(new_indices.size());
    for (auto i : range(temp_nodes_.bboxes.size()))
    {
        auto new_id = remapped_id(id_cast<BIHNodeId>(i));
        CELER_ASSERT(new_id < result.bboxes.size());
        result.bboxes[*new_id] = temp_nodes_.bboxes[i];
    }

    return result;
}

//---------------------------------------------------------------------------//
}  // namespace detail
}  // namespace celeritas

//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file geocel/Volume.test.cc
//! Test VolumeParams and related utilities
//---------------------------------------------------------------------------//
#include <unordered_map>

#include "corecel/OpaqueIdUtils.hh"
#include "corecel/cont/LabelIdMultiMapUtils.hh"
#include "corecel/io/Label.hh"
#include "corecel/io/StreamUtils.hh"
#include "geocel/Types.hh"
#include "geocel/VolumeParams.hh"
#include "geocel/VolumePathFinder.hh"
#include "geocel/VolumeToString.hh"
#include "geocel/VolumeUniqueInstanceAccumulator.hh"
#include "geocel/VolumeVisitor.hh"

#include "TestMacros.hh"
#include "VolumeTestBase.hh"
#include "celeritas_test.hh"

namespace celeritas
{
namespace test
{
//---------------------------------------------------------------------------//
struct NameVisitor
{
    VolumeParams const& vols;
    std::vector<std::string> names;

    bool operator()(VolumeId id, int)
    {
        names.push_back(vols.volume_labels().at(id).name);
        return true;
    }

    bool operator()(VolumeInstanceId id, int depth)
    {
        auto const& vlabels = vols.volume_instance_labels();
        std::ostringstream os;
        os << depth << ':' << vlabels.at(id).name;
        names.push_back(std::move(os).str());
        return true;
    }
};

template<class T = VolumeId>
struct MaxVisitor
{
    LabelIdMultiMap<T> const& labels;
    std::unordered_map<T, int> max_depth;

    bool operator()(T id, int depth)
    {
        auto&& [iter, inserted] = max_depth.insert({id, depth});
        if (!inserted)
        {
            if (iter->second >= depth)
            {
                // Already visited PV at this depth or more
                return false;
            }
            // Update the max depth
            iter->second = depth;
        }
        return true;
    }

    auto get_names() const
    {
        std::vector<std::string> names;
        for (auto&& [id, depth] : max_depth)
        {
            std::ostringstream os;
            os << depth << ':' << labels.at(id).name;
            names.push_back(std::move(os).str());
        }
        // Make reproducible across unordered map implementation
        std::sort(names.begin(), names.end());
        return names;
    }
};

//---------------------------------------------------------------------------//
/*!
 * Note in the following tests:
 * - volumes are alphabetical (A, B, C...)
 * - volume instances are numeric (0, 1, 2...)
 */

//---------------------------------------------------------------------------//
class NoVolumeTest : public VolumeTestBase
{
  protected:
    std::shared_ptr<VolumeParams> build_volumes() const override
    {
        return std::make_shared<VolumeParams>();
    }
};

/*!
 * No volumes, for unit testing
 */
TEST_F(NoVolumeTest, params)
{
    VolumeParams const& params = this->volumes();
    EXPECT_TRUE(params.empty());
    EXPECT_EQ(0, params.num_volumes());
    EXPECT_EQ(VolumeId{}, params.world());
    EXPECT_EQ(0, params.num_volume_levels());
}

TEST_F(NoVolumeTest, volume_to_string)
{
    VolumeToString to_string;
    EXPECT_EQ("<null>", to_string(VolumeId{}));
    EXPECT_EQ("<null>", to_string(VolumeInstanceId{}));
    EXPECT_EQ("v 1", to_string(VolumeId{1}));
    EXPECT_EQ("vi 2", to_string(VolumeInstanceId{2}));
}

//---------------------------------------------------------------------------//
/*!
 * Graph:
 *    A
 */
using SingleVolumeTest = SingleVolumeTestBase;

TEST_F(SingleVolumeTest, params)
{
    VolumeParams const& params = this->volumes();

    EXPECT_FALSE(params.empty());
    EXPECT_EQ(1, params.num_volumes());
    EXPECT_EQ(0, params.num_volume_instances());
    EXPECT_EQ(VolumeId{0}, params.world());
    EXPECT_EQ(1, params.num_volume_levels());
    EXPECT_EQ(1, params.volume_labels().size());
    EXPECT_EQ(0, params.volume_instance_labels().size());

    // Check that volume 0 is correctly mapped
    VolumeId vol_id{0};
    EXPECT_TRUE(params.volume_labels().find_unique("A") == vol_id);

    // Verify material assignment
    EXPECT_EQ(GeoMatId{0}, params.get(vol_id).material());

    // A single volume should have no parents or children
    EXPECT_TRUE(params.get(vol_id).parents().empty());
    EXPECT_TRUE(params.children(vol_id).empty());

    // Test out-of-bounds access should assert
    if (CELERITAS_DEBUG)
    {
        EXPECT_THROW(params.get(VolumeId{1}).material(), DebugError);
        EXPECT_THROW(params.get(VolumeId{1}).parents(), DebugError);
        EXPECT_THROW(params.children(VolumeId{1}), DebugError);
        EXPECT_THROW(params.volume(VolumeInstanceId{0}), DebugError);
    }
}

TEST_F(SingleVolumeTest, volume_to_string)
{
    VolumeToString to_string{this->volumes()};
    EXPECT_EQ("<null>", to_string(VolumeId{}));
    EXPECT_EQ("<null>", to_string(VolumeInstanceId{}));
    EXPECT_EQ("A", to_string(VolumeId{0}));

    if (CELERITAS_DEBUG)
    {
        EXPECT_THROW(to_string(VolumeId{1}), DebugError);
    }
}

TEST_F(SingleVolumeTest, visit)
{
    VolumeVisitor visit(this->volumes());
    {
        NameVisitor nv{this->volumes(), {}};
        visit(nv, VolumeId{0});

        static std::string const expected_names[] = {"A"};
        EXPECT_VEC_EQ(expected_names, nv.names);
    }
}

//---------------------------------------------------------------------------//
using ComplexVolumeTest = ComplexVolumeTestBase;

TEST_F(ComplexVolumeTest, params)
{
    VolumeParams const& params = this->volumes();
    EXPECT_EQ(4, params.num_volume_levels());

    static std::string const expected_volume_labels[]
        = {"A", "B", "C", "D", "E"};
    static std::string const expected_volume_instance_labels[]
        = {"0", "1", "2", "3", "4", "", "6"};

    // Check volume labels
    EXPECT_VEC_EQ(expected_volume_labels,
                  get_multimap_labels(params.volume_labels()));
    EXPECT_VEC_EQ(expected_volume_instance_labels,
                  get_multimap_labels(params.volume_instance_labels()));

    std::vector<std::vector<int>> children;
    std::vector<std::vector<int>> parents;
    std::vector<int> geo_mat;

    // Loop over all volumes to collect children and parents
    for (auto vol_id : range(VolumeId(params.num_volumes())))
    {
        auto v = params.get(vol_id);
        children.push_back(id_to_int(params.children(vol_id)));
        parents.push_back(id_to_int(v.parents()));
        geo_mat.push_back(id_to_int(v.material()));
    }

    static std::vector<int> const expected_children[]
        = {{0, 1}, {2, 3}, {4, 6}, {}, {}};
    static std::vector<int> const expected_parents[]
        = {{}, {0}, {1, 2, 3}, {4}, {6}};
    static int const expected_geo_mat[] = {0, 1, 2, 3, 4};
    EXPECT_VEC_EQ(expected_children, children);
    EXPECT_VEC_EQ(expected_parents, parents);
    EXPECT_VEC_EQ(expected_geo_mat, geo_mat);

    // Check volume instance to volume mappings
    std::vector<int> volume_mapping;
    for (auto inst_id : range(VolumeInstanceId(params.num_volume_instances())))
    {
        volume_mapping.push_back(id_to_int(params.volume(inst_id)));
    }

    static int const expected_volume_mapping[] = {1, 2, 2, 2, 3, -1, 4};
    EXPECT_VEC_EQ(expected_volume_mapping, volume_mapping);
}

TEST_F(ComplexVolumeTest, volume_to_string)
{
    VolumeToString to_string{this->volumes()};
    EXPECT_EQ("A", to_string(VolumeId{0}));
    EXPECT_EQ("1", to_string(VolumeInstanceId{1}));
}

TEST_F(ComplexVolumeTest, visit)
{
    VolumeVisitor visit(this->volumes());

    {
        NameVisitor nv{this->volumes(), {}};
        visit(nv, VolumeId{0});

        static std::string const expected_names[]
            = {"A", "B", "C", "D", "E", "C", "D", "E", "C", "D", "E"};
        EXPECT_VEC_EQ(expected_names, nv.names);
    }

    {
        NameVisitor nv{this->volumes(), {}};
        visit(nv, VolumeInstanceId{0});

        static std::string const expected_names[]
            = {"0:0", "1:2", "2:4", "2:6", "1:3", "2:4", "2:6"};
        EXPECT_VEC_EQ(expected_names, nv.names);
    }

    {
        MaxVisitor<> mpv{this->volumes().volume_labels(), {}};
        visit(mpv, VolumeId{0});
        static std::string const expected_names[]
            = {"0:A", "1:B", "2:C", "3:D", "3:E"};
        EXPECT_VEC_EQ(expected_names, mpv.get_names());
    }
}

//---------------------------------------------------------------------------//
using MultiLevelTest = MultiLevelVolumeTestBase;

TEST_F(MultiLevelTest, visit)
{
    auto const& vols = this->volumes();
    auto const world_vi
        = vols.volume_instance_labels().find_unique("world_PV");
    VolumeVisitor visit(vols);

    {
        NameVisitor nv{vols, {}};
        visit(nv, vols.world());

        static std::string const expected_names[] = {
            "world",
            "box",
            "sph",
            "sph",
            "tri",
            "sph",
            "box",
            "sph",
            "sph",
            "tri",
            "box",
            "sph",
            "sph",
            "tri",
            "box_refl",
            "sph_refl",
            "sph_refl",
            "tri_refl",
        };
        EXPECT_VEC_EQ(expected_names, nv.names);
    }

    {
        NameVisitor nv{vols, {}};
        visit(nv, world_vi);

        static std::string const expected_names[] = {
            "0:world_PV",
            "1:topbox1",
            "2:boxsph1",
            "2:boxsph2",
            "2:boxtri",
            "1:topsph1",
            "1:topbox2",
            "2:boxsph1",
            "2:boxsph2",
            "2:boxtri",
            "1:topbox3",
            "2:boxsph1",
            "2:boxsph2",
            "2:boxtri",
            "1:topbox4",
            "2:boxsph1",
            "2:boxsph2",
            "2:boxtri",
        };
        EXPECT_VEC_EQ(expected_names, nv.names);
    }

    {
        MaxVisitor<> mpv{vols.volume_labels(), {}};
        visit(mpv, vols.world());
        static std::string const expected_names[] = {
            "0:world",
            "1:box",
            "1:box_refl",
            "2:sph",
            "2:sph_refl",
            "2:tri",
            "2:tri_refl",
        };
        EXPECT_VEC_EQ(expected_names, mpv.get_names());
    }

    {
        MaxVisitor<VolumeInstanceId> mpv{vols.volume_instance_labels(), {}};
        visit(mpv, world_vi);
        static std::string const expected_names[] = {
            "0:world_PV",
            "1:topbox1",
            "1:topbox2",
            "1:topbox3",
            "1:topbox4",
            "1:topsph1",
            "2:boxsph1",
            "2:boxsph1",
            "2:boxsph2",
            "2:boxsph2",
            "2:boxtri",
            "2:boxtri",
        };
        EXPECT_VEC_EQ(expected_names, mpv.get_names());
    }
}

TEST_F(MultiLevelTest, io)
{
    auto const& vols = this->volumes();
    auto vols_json_str = stream_to_string(vols);
    EXPECT_JSON_EQ(R"json({
"children": [
[],
[],
[0, 1, 2 ],
[3, 4, 5, 6, 10 ],
[7, 8, 9 ],
[],
[]
],
"instance_to_volume": [ 0, 0, 1, 2, 0, 2, 2, 5, 5, 6, 4, 3 ],
"volume_instances": [ "boxsph1@0", "boxsph2@0", "boxtri@0", "topbox1", "topsph1", "topbox2", "topbox3", "boxsph1@1", "boxsph2@1", "boxtri@1", "topbox4", "world_PV" ],
"volumes": [ "sph", "tri", "box", "world", "box_refl", "sph_refl", "tri_refl" ],
"world": 3
})json",
                   vols_json_str);
}

TEST_F(MultiLevelTest, unique_instance)
{
    // Check offsets
    auto const& vols = this->volumes();

    constexpr auto all = AllItems<VolumeUniqueInstanceId>{};
    auto offsets = id_to_int(vols.host_ref().unique_instance_offsets[all]);
    EXPECT_EQ(vols.num_volume_instances(), offsets.size());
    static int const expected_offsets[] = {0, 1, 2, 0, 4, 5, 9, 0, 1, 2, 13, 0};
    EXPECT_VEC_EQ(expected_offsets, offsets);
}

TEST_F(MultiLevelTest, unique_instance_accumulator)
{
    // vi indices (from JSON):
    //  0:boxsph1@0→sph, 1:boxsph2@0→sph, 2:boxtri@0→tri,
    //  3:topbox1→box,   4:topsph1→sph,   5:topbox2→box, 6:topbox3→box,
    //  7:boxsph1@1→sph_refl, 8:boxsph2@1→sph_refl, 9:boxtri@1→tri_refl,
    //  10:topbox4→box_refl, 11:world_PV→world
    auto const& vols = this->volumes();
    VolumeUniqueInstanceAccumulator acc{vols.host_ref()};
    auto const& vi_labels = vols.volume_instance_labels();

    auto world_pv = vi_labels.find_unique("world_PV");
    auto topbox1 = vi_labels.find_unique("topbox1");
    auto topsph1 = vi_labels.find_unique("topsph1");
    auto topbox4 = vi_labels.find_unique("topbox4");
    auto boxsph1_0 = vi_labels.find_exact(Label::from_separator("boxsph1@0"));
    auto boxsph1_1 = vi_labels.find_exact(Label::from_separator("boxsph1@1"));
    auto boxtri_1 = vi_labels.find_exact(Label::from_separator("boxtri@1"));

    // Path [world_PV] → uid 1
    VolumeUniqueInstanceId uid{0};
    EXPECT_EQ(VolumeUniqueInstanceId{1}, uid = acc(uid, world_pv));

    // Path [world_PV, topbox1] → uid 2  (box)
    EXPECT_EQ(VolumeUniqueInstanceId{2}, uid = acc(uid, topbox1));

    // Path [world_PV, topbox1, boxsph1@0] → uid 3  (sph)
    EXPECT_EQ(VolumeUniqueInstanceId{3}, uid = acc(uid, boxsph1_0));

    // Path [world_PV, topsph1] → uid 6  (sph directly under world)
    uid = acc(VolumeUniqueInstanceId{0}, world_pv);
    EXPECT_EQ(VolumeUniqueInstanceId{6}, uid = acc(uid, topsph1));

    // Path [world_PV, topbox4] → uid 15  (box_refl)
    uid = acc(VolumeUniqueInstanceId{0}, world_pv);
    EXPECT_EQ(VolumeUniqueInstanceId{15}, uid = acc(uid, topbox4));

    // Path [world_PV, topbox4, boxsph1@1] → uid 16  (sph_refl)
    EXPECT_EQ(VolumeUniqueInstanceId{16}, uid = acc(uid, boxsph1_1));

    // Path [world_PV, topbox4, boxtri@1] → uid 18  (tri_refl, last)
    uid = acc(VolumeUniqueInstanceId{0}, world_pv);
    uid = acc(uid, topbox4);
    EXPECT_EQ(VolumeUniqueInstanceId{18}, uid = acc(uid, boxtri_1));
}

//---------------------------------------------------------------------------//
TEST_F(MultiLevelTest, offset)
{
    auto const& vols = this->volumes();
    auto const& vi_labels = vols.volume_instance_labels();

    // First child of any volume always has offset 0
    auto world_pv = vi_labels.find_unique("world_PV");
    EXPECT_EQ(0u, vols.offset(world_pv));
    auto topbox1 = vi_labels.find_unique("topbox1");
    EXPECT_EQ(0u, vols.offset(topbox1));

    // topsph1 follows topbox1 whose subtree has num_desc = 4
    // (box itself + boxsph1@0, boxsph2@0, boxtri@0)
    auto topsph1 = vi_labels.find_unique("topsph1");
    EXPECT_EQ(4u, vols.offset(topsph1));

    // topbox4 follows topbox1+topsph1+topbox2+topbox3; each sph/tri leaf
    // contributes 1, each box contributes 4 → 4+1+4+4 = 13
    auto topbox4 = vi_labels.find_unique("topbox4");
    EXPECT_EQ(13u, vols.offset(topbox4));
}

//---------------------------------------------------------------------------//
TEST_F(MultiLevelTest, path_finder)
{
    auto const& vols = this->volumes();
    std::vector<VolumeInstanceId> buf(vols.num_volume_levels());
    VolumePathFinder find_path{vols.host_ref(), make_span(buf)};

    auto const& vi_labels = vols.volume_instance_labels();
    auto world_pv = vi_labels.find_unique("world_PV");
    auto topbox1 = vi_labels.find_unique("topbox1");
    auto topbox4 = vi_labels.find_unique("topbox4");
    auto topsph1 = vi_labels.find_unique("topsph1");
    auto boxsph1_0 = vi_labels.find_exact(Label::from_separator("boxsph1@0"));
    auto boxsph1_1 = vi_labels.find_exact(Label::from_separator("boxsph1@1"));
    auto boxtri_1 = vi_labels.find_exact(Label::from_separator("boxtri@1"));

    // uid 0 → empty path (world, no enclosing instance)
    EXPECT_EQ(0u, find_path(VolumeUniqueInstanceId{0}).size());

    // uid 1 → [world_PV]
    auto path = find_path(VolumeUniqueInstanceId{1});
    ASSERT_EQ(1u, path.size());
    EXPECT_EQ(world_pv, path[0]);

    // uid 2 → [world_PV, topbox1]
    path = find_path(VolumeUniqueInstanceId{2});
    ASSERT_EQ(2u, path.size());
    EXPECT_EQ(world_pv, path[0]);
    EXPECT_EQ(topbox1, path[1]);

    // uid 3 → [world_PV, topbox1, boxsph1@0]
    path = find_path(VolumeUniqueInstanceId{3});
    ASSERT_EQ(3u, path.size());
    EXPECT_EQ(world_pv, path[0]);
    EXPECT_EQ(topbox1, path[1]);
    EXPECT_EQ(boxsph1_0, path[2]);

    // uid 6 → [world_PV, topsph1]
    path = find_path(VolumeUniqueInstanceId{6});
    ASSERT_EQ(2u, path.size());
    EXPECT_EQ(world_pv, path[0]);
    EXPECT_EQ(topsph1, path[1]);

    // uid 15 → [world_PV, topbox4]
    path = find_path(VolumeUniqueInstanceId{15});
    ASSERT_EQ(2u, path.size());
    EXPECT_EQ(world_pv, path[0]);
    EXPECT_EQ(topbox4, path[1]);

    // uid 16 → [world_PV, topbox4, boxsph1@1]
    path = find_path(VolumeUniqueInstanceId{16});
    ASSERT_EQ(3u, path.size());
    EXPECT_EQ(world_pv, path[0]);
    EXPECT_EQ(topbox4, path[1]);
    EXPECT_EQ(boxsph1_1, path[2]);

    // uid 18 → [world_PV, topbox4, boxtri@1]  (last leaf)
    path = find_path(VolumeUniqueInstanceId{18});
    ASSERT_EQ(3u, path.size());
    EXPECT_EQ(world_pv, path[0]);
    EXPECT_EQ(topbox4, path[1]);
    EXPECT_EQ(boxtri_1, path[2]);
}

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas

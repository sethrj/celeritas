//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file geocel/VolumeParamsIO.json.cc
//---------------------------------------------------------------------------//
#include "VolumeParamsIO.json.hh"

#include <istream>
#include <ostream>

#include "corecel/Assert.hh"
#include "corecel/cont/Range.hh"
#include "corecel/io/JsonUtils.json.hh"
#include "corecel/io/LabelIO.json.hh"

#include "inp/Model.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Write volume hierarchy data to JSON.
 *
 * The output object has:
 * - \c volumes : array of volume labels (indexed by VolumeId)
 * - \c volume_instances : array of volume instance labels (indexed by
 *   VolumeInstanceId)
 * - \c world : integer VolumeId of the root, or null if empty
 * - \c children : array of arrays; \c children[v] lists the VolumeInstanceId
 *   integers of the direct children placed inside volume \c v
 * - \c volume_ids : array; \c volume_ids[vi] is the integer VolumeId that
 *   instance \c vi instantiates, or null if \c vi is a null instance
 */
void to_json(nlohmann::json& j, VolumeParams const& vp)
{
    using json = nlohmann::json;

    // Volume labels
    auto j_vols = json::array();
    for (auto v : range(VolumeId{vp.num_volumes()}))
    {
        j_vols.push_back(vp.volume_labels().at(v));
    }

    // Volume instance labels
    auto j_vis = json::array();
    for (auto vi : range(VolumeInstanceId{vp.num_volume_instances()}))
    {
        j_vis.push_back(vp.volume_instance_labels().at(vi));
    }

    // Children: children[v] = [vi, ...]
    auto j_children = json::array();
    for (auto v : range(VolumeId{vp.num_volumes()}))
    {
        auto ch = json::array();
        for (VolumeInstanceId vi : vp.children(v))
        {
            ch.push_back(vi);
        }
        j_children.push_back(std::move(ch));
    }

    // volume_ids: volume_ids[vi] = v (null for a null instance)
    auto j_vids = json::array();
    for (auto vi : range(VolumeInstanceId{vp.num_volume_instances()}))
    {
        j_vids.push_back(vp.volume(vi));
    }

    j = {
        {"volumes", std::move(j_vols)},
        {"volume_instances", std::move(j_vis)},
        {"world", vp.world()},
        {"children", std::move(j_children)},
        {"volume_ids", std::move(j_vids)},
    };
}

//---------------------------------------------------------------------------//
// Helper to write the volume hierarchy to a stream.
std::ostream& operator<<(std::ostream& os, VolumeParams const& vp)
{
    nlohmann::json j = vp;
    os << j.dump(0);
    return os;
}

namespace inp
{
//---------------------------------------------------------------------------//
/*!
 * Read volume hierarchy data from JSON.
 *
 * See \c to_json for the expected format.  \c GeoMatId assignments are not
 * serialised and will be left null on construction.
 */
void from_json(nlohmann::json const& j, inp::Volumes& in)
{
    // Volume labels
    for (auto const& lj : j.at("volumes"))
    {
        inp::Volume v;
        v.label = lj.get<Label>();
        in.volumes.push_back(std::move(v));
    }

    // Volume instance labels
    for (auto const& lj : j.at("volume_instances"))
    {
        inp::VolumeInstance vi;
        vi.label = lj.get<Label>();
        in.volume_instances.push_back(std::move(vi));
    }

    // World
    j.at("world").get_to(in.world);

    // Children: fill volumes[v].children
    auto const& j_children = j.at("children");
    CELER_VALIDATE(j_children.size() == in.volumes.size(),
                   << "\"children\" length " << j_children.size()
                   << " != number of volumes " << in.volumes.size());
    for (auto v_idx : range(in.volumes.size()))
    {
        for (auto const& vi_j : j_children[v_idx])
        {
            VolumeInstanceId vi;
            vi_j.get_to(vi);
            in.volumes[v_idx].children.push_back(vi);
        }
    }

    // volume_ids: fill volume_instances[vi].volume
    auto const& j_vids = j.at("volume_ids");
    CELER_VALIDATE(j_vids.size() == in.volume_instances.size(),
                   << "\"volume_ids\" length " << j_vids.size()
                   << " != number of volume instances "
                   << in.volume_instances.size());
    for (auto vi_idx : range(in.volume_instances.size()))
    {
        j_vids[vi_idx].get_to(in.volume_instances[vi_idx].volume);
    }
}

//---------------------------------------------------------------------------//
// Helper to read the volume hierarchy from a stream.
std::istream& operator>>(std::istream& is, Volumes& vols)
{
    auto j = nlohmann::json::parse(is);
    j.get_to(vols);
    return is;
}

//---------------------------------------------------------------------------//
}  // namespace inp
}  // namespace celeritas

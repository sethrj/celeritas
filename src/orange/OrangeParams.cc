//----------------------------------*-C++-*----------------------------------//
// Copyright 2021-2024 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/OrangeParams.cc
//---------------------------------------------------------------------------//
#include "OrangeParams.hh"

#include <fstream>
#include <initializer_list>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

#include "corecel/Config.hh"

#include "corecel/Assert.hh"
#include "corecel/OpaqueId.hh"
#include "corecel/cont/Range.hh"
#include "corecel/cont/VariantUtils.hh"
#include "corecel/data/Collection.hh"
#include "corecel/data/CollectionBuilder.hh"
#include "corecel/io/Logger.hh"
#include "corecel/io/ScopedTimeLog.hh"
#include "corecel/io/StringUtils.hh"
#include "corecel/sys/ScopedMem.hh"
#include "corecel/sys/ScopedProfiling.hh"
#include "geocel/BoundingBox.hh"
#include "geocel/GeantGeoUtils.hh"

#include "OrangeData.hh"  // IWYU pragma: associated
#include "OrangeInput.hh"
#include "OrangeInputIO.json.hh"
#include "OrangeTypes.hh"
#include "g4org/Converter.hh"
#include "univ/detail/LogicStack.hh"

#include "detail/DepthCalculator.hh"
#include "detail/RectArrayInserter.hh"
#include "detail/UnitInserter.hh"
#include "detail/UniverseInserter.hh"

namespace celeritas
{
namespace
{
//---------------------------------------------------------------------------//
/*!
 * Load a geometry from the given JSON file.
 */
OrangeInput input_from_json(std::string filename)
{
    CELER_LOG(info) << "Loading ORANGE geometry from JSON at " << filename;
    ScopedTimeLog scoped_time;

    OrangeInput result;

    std::ifstream infile(filename);
    CELER_VALIDATE(infile,
                   << "failed to open geometry at '" << filename << '\'');
    // Use the `from_json` defined in OrangeInputIO.json to read the JSON input
    nlohmann::json::parse(infile).get_to(result);

    return result;
}

//---------------------------------------------------------------------------//
/*!
 * Load a geometry from the given filename.
 */
OrangeInput input_from_file(std::string filename)
{
    if (ends_with(filename, ".gdml"))
    {
        if (CELERITAS_USE_GEANT4
            && CELERITAS_REAL_TYPE == CELERITAS_REAL_TYPE_DOUBLE)
        {
            // Load with Geant4: must *not* be using run manager
            auto* world = ::celeritas::load_geant_geometry_native(filename);
            auto result = g4org::Converter{}(world).input;
            ::celeritas::reset_geant_geometry();
            return result;
        }
        else
        {
            CELER_LOG(warning) << "Using ORANGE geometry with GDML suffix "
                                  "when Geant4 conversion is disabled: trying "
                                  "`.org.json` instead";
            filename.erase(filename.end() - 5, filename.end());
            filename += ".org.json";
        }
    }
    else
    {
        CELER_VALIDATE(ends_with(filename, ".json"),
                       << "expected JSON extension for ORANGE input '"
                       << filename << "'");
    }
    return input_from_json(std::move(filename));
}

//---------------------------------------------------------------------------//
}  // namespace

//---------------------------------------------------------------------------//
/*!
 * Construct from a JSON file (if JSON is enabled).
 *
 * The JSON format is defined by the SCALE ORANGE exporter (not currently
 * distributed).
 */
OrangeParams::OrangeParams(std::string const& filename)
    : OrangeParams(input_from_file(filename))
{
}

//---------------------------------------------------------------------------//
/*!
 * Construct in-memory from a Geant4 geometry.
 *
 * TODO: expose options? Fix volume mappings?
 */
OrangeParams::OrangeParams(G4VPhysicalVolume const* world)
    : OrangeParams(std::move(g4org::Converter{}(world).input))
{
}

//---------------------------------------------------------------------------//
/*!
 * Advanced usage: construct from explicit host data.
 *
 * Volume and surface labels must be unique for the time being.
 */
OrangeParams::OrangeParams(OrangeInput&& input)
{
    CELER_VALIDATE(input, << "input geometry is incomplete");

    auto const use_device = static_cast<bool>(celeritas::device());
    ScopedProfiling profile_this{"finalize-orange-runtime"};
    ScopedMem record_mem("orange.finalize_runtime");
    CELER_LOG(debug) << "Merging runtime data"
                     << (use_device ? " and copying to GPU" : "");
    ScopedTimeLog scoped_time;

    // Save global bounding box
    bbox_ = [&input] {
        auto& global = input.universes[orange_global_universe.unchecked_get()];
        CELER_VALIDATE(std::holds_alternative<UnitInput>(global),
                       << "global universe is not a SimpleUnit");
        return std::get<UnitInput>(global).bbox;
    }();

    // Create host data for construction, setting tolerances first
    HostVal<OrangeParamsData> host_data;
    host_data.scalars.tol = input.tol;
    host_data.scalars.max_depth = detail::DepthCalculator{input.universes}();

    // Insert all universes
    {
        std::vector<Label> universe_labels;
        std::vector<Label> surface_labels;
        std::vector<Label> volume_labels;

        detail::UniverseInserter insert_universe_base{
            &universe_labels, &surface_labels, &volume_labels, &host_data};
        Overload insert_universe{
            detail::UnitInserter{&insert_universe_base, &host_data},
            detail::RectArrayInserter{&insert_universe_base, &host_data}};

        for (auto&& u : input.universes)
        {
            std::visit(insert_universe, std::move(u));
        }

        univ_labels_ = LabelIdMultiMap<UniverseId>{std::move(universe_labels)};
        surf_labels_ = LabelIdMultiMap<SurfaceId>{std::move(surface_labels)};
        vol_labels_ = LabelIdMultiMap<VolumeId>{std::move(volume_labels)};
    }

    // Simple safety if all SimpleUnits have simple safety and no RectArrays
    // are present
    supports_safety_
        = std::all_of(
              host_data.simple_units[AllItems<SimpleUnitRecord>()].begin(),
              host_data.simple_units[AllItems<SimpleUnitRecord>()].end(),
              [](SimpleUnitRecord const& su) { return su.simple_safety; })
          && host_data.rect_arrays.empty();

    // Verify scalars *after* loading all units
    CELER_VALIDATE(host_data.scalars.max_logic_depth
                       < detail::LogicStack::max_stack_depth(),
                   << "input geometry has at least one volume with a "
                      "logic depth of"
                   << host_data.scalars.max_logic_depth
                   << " (a volume's CSG tree is too deep); but the logic "
                      "stack is limited to a depth of "
                   << detail::LogicStack::max_stack_depth());

    // Round up strides based on whether GPU is enabled
    auto update_stride = [stride_bytes = use_device ? 32u : 8u](
                             size_type* s, size_type scalar_bytes) {
        auto item_width = stride_bytes / scalar_bytes;
        *s = ceil_div(*s, item_width) * item_width;
    };
    update_stride(&host_data.scalars.max_faces, sizeof(Sense));
    update_stride(&host_data.scalars.max_intersections,
                  min(sizeof(size_type), sizeof(real_type)));

    // Construct device values and device/host references
    CELER_ASSERT(host_data);
    data_ = CollectionMirror<OrangeParamsData>{std::move(host_data)};

    CELER_ENSURE(data_);
    CELER_ENSURE(vol_labels_.size() > 0);
    CELER_ENSURE(bbox_);
}

//---------------------------------------------------------------------------//
/*!
 * Get the label of a volume.
 */
Label const& OrangeParams::id_to_label(VolumeId vol) const
{
    CELER_EXPECT(vol < vol_labels_.size());
    return vol_labels_.get(vol);
}

//---------------------------------------------------------------------------//
/*!
 * Locate the volume ID corresponding to a unique name.
 *
 * If the name isn't in the geometry, a null ID will be returned. If the name
 * is not unique, a RuntimeError will be raised.
 */
VolumeId OrangeParams::find_volume(std::string const& name) const
{
    auto result = vol_labels_.find_all(name);
    if (result.empty())
        return {};
    CELER_VALIDATE(result.size() == 1,
                   << "volume '" << name << "' is not unique");
    return result.front();
}

//---------------------------------------------------------------------------//
/*!
 * Locate the volume ID corresponding to a label.
 *
 * If the label isn't in the geometry, a null ID will be returned.
 */
VolumeId OrangeParams::find_volume(Label const& label) const
{
    return vol_labels_.find(label);
}

//---------------------------------------------------------------------------//
/*!
 * Get zero or more volume IDs corresponding to a name.
 *
 * This is useful for volumes that are repeated in the geometry with different
 * uniquifying 'extensions'.
 */
auto OrangeParams::find_volumes(std::string const& name) const -> SpanConstVolumeId
{
    return vol_labels_.find_all(name);
}

//---------------------------------------------------------------------------//
/*!
 * Get the label of a surface.
 */
Label const& OrangeParams::id_to_label(SurfaceId surf) const
{
    CELER_EXPECT(surf < surf_labels_.size());
    return surf_labels_.get(surf);
}

//---------------------------------------------------------------------------//
/*!
 * Locate the surface ID corresponding to a label name.
 */
SurfaceId OrangeParams::find_surface(std::string const& name) const
{
    auto result = surf_labels_.find_all(name);
    if (result.empty())
        return {};
    CELER_VALIDATE(result.size() == 1,
                   << "surface '" << name << "' is not unique");
    return result.front();
}

//---------------------------------------------------------------------------//
/*!
 * Get the label of a universe.
 */
Label const& OrangeParams::id_to_label(UniverseId univ) const
{
    CELER_EXPECT(univ < univ_labels_.size());
    return univ_labels_.get(univ);
}

//---------------------------------------------------------------------------//
/*!
 * Locate the universe ID corresponding to a label name.
 */
UniverseId OrangeParams::find_universe(std::string const& name) const
{
    auto result = univ_labels_.find_all(name);
    if (result.empty())
        return {};
    CELER_VALIDATE(result.size() == 1,
                   << "universe '" << name << "' is not unique");
    return result.front();
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file orange/inp/IO.json.cc
//---------------------------------------------------------------------------//
#include "IO.json.hh"

#include "corecel/Assert.hh"
#include "corecel/io/EnumStringMapper.hh"
#include "corecel/io/JsonUtils.json.hh"
#include "corecel/io/StringEnumMapper.hh"
#include "orange/OrangeTypesIO.json.hh"  // IWYU pragma: keep

#include "OrangePerf.hh"

namespace celeritas
{
namespace inp
{
namespace
{
static char const g4org_format_str[] = "g4org-options";
static char const perf_format_str[] = "orange-perf";
}  // namespace

//---------------------------------------------------------------------------//
/*!
 * Get a string corresponding to an inline singletons option.
 */
char const* to_cstring(InlineSingletons value)
{
    static EnumStringMapper<InlineSingletons> const to_cstring_impl{
        "none",
        "untransformed",
        "unrotated",
        "all",
    };
    return to_cstring_impl(value);
}

//---------------------------------------------------------------------------//
//!@{
//! I/O routines for JSON

void to_json(nlohmann::json& j, InlineSingletons const& v)
{
    j = std::string{to_cstring(v)};
}

void from_json(nlohmann::json const& j, InlineSingletons& v)
{
    static auto const from_string
        = StringEnumMapper<InlineSingletons>::from_cstring_func(
            to_cstring, "inline singletons");
    v = from_string(j.get<std::string>());
}

void to_json(nlohmann::json& j, OrangeGeoFromGeant const& v)
{
#define OPT_JSON_STRING(NAME) CELER_JSON_PAIR_WHEN(v, NAME, !v.NAME.empty())

    j = nlohmann::json{
        CELER_JSON_PAIR(v, unit_length),
        CELER_JSON_PAIR(v, explicit_interior_threshold),
        CELER_JSON_PAIR(v, inline_childless),
        CELER_JSON_PAIR(v, inline_singletons),
        CELER_JSON_PAIR(v, inline_unions),
        CELER_JSON_PAIR(v, remove_interior),
        CELER_JSON_PAIR(v, remove_negated_join),
        CELER_JSON_PAIR(v, verbose_volumes),
        CELER_JSON_PAIR(v, verbose_structure),
        CELER_JSON_PAIR_OPTION(v, tol),
        OPT_JSON_STRING(objects_output_file),
        OPT_JSON_STRING(csg_output_file),
        OPT_JSON_STRING(org_output_file),
    };

#undef OPT_JSON_STRING

    save_format(j, g4org_format_str);
}

void from_json(nlohmann::json const& j, OrangeGeoFromGeant& v)
{
    check_format(j, g4org_format_str);

#define OPT_LOAD_OPTION(NAME) CELER_JSON_LOAD_OPTION(j, v, NAME)

    OPT_LOAD_OPTION(unit_length);
    OPT_LOAD_OPTION(tol);
    OPT_LOAD_OPTION(explicit_interior_threshold);
    OPT_LOAD_OPTION(inline_childless);
    OPT_LOAD_OPTION(inline_singletons);
    OPT_LOAD_OPTION(inline_unions);
    OPT_LOAD_OPTION(remove_interior);
    OPT_LOAD_OPTION(remove_negated_join);
    OPT_LOAD_OPTION(verbose_volumes);
    OPT_LOAD_OPTION(verbose_structure);
    OPT_LOAD_OPTION(objects_output_file);
    OPT_LOAD_OPTION(csg_output_file);
    OPT_LOAD_OPTION(org_output_file);

#undef OPT_LOAD_OPTION
}

void to_json(nlohmann::json& j, OrangePerf const& v)
{
    j = nlohmann::json{
        CELER_JSON_PAIR(v, max_intersect),
    };
    save_format(j, perf_format_str);
}

void from_json(nlohmann::json const& j, OrangePerf& v)
{
    check_format(j, perf_format_str);
    CELER_JSON_LOAD_OPTION(j, v, max_intersect);
}

//!@}

//---------------------------------------------------------------------------//
/*!
 * Helper to read the import options from a file or stream.
 *
 * Example to read from a file:
 * \code
    Options inp;
    std::ifstream("foo.json") >> inp;
 * \endcode
 */
std::istream& operator>>(std::istream& is, OrangeGeoFromGeant& inp)
{
    auto j = nlohmann::json::parse(is);
    j.get_to(inp);
    return is;
}

//---------------------------------------------------------------------------//
/*!
 * Helper to write the options to a file or stream.
 */
std::ostream& operator<<(std::ostream& os, OrangeGeoFromGeant const& inp)
{
    nlohmann::json j = inp;
    os << j.dump();
    return os;
}

//---------------------------------------------------------------------------//
/*!
 * Helper to read the performance knobs from a file or stream.
 *
 * Example to read from a file:
 * \code
    OrangePerf perf;
    std::ifstream("foo.json") >> perf;
 * \endcode
 */
std::istream& operator>>(std::istream& is, OrangePerf& perf)
{
    auto j = nlohmann::json::parse(is);
    j.get_to(perf);
    return is;
}

//---------------------------------------------------------------------------//
/*!
 * Helper to write the performance knobs to a file or stream.
 */
std::ostream& operator<<(std::ostream& os, OrangePerf const& perf)
{
    nlohmann::json j = perf;
    os << j.dump();
    return os;
}

//---------------------------------------------------------------------------//
}  // namespace inp
}  // namespace celeritas

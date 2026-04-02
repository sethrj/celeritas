//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/cont/LdgSpan.hh
//---------------------------------------------------------------------------//
#pragma once

#include <cstddef>

#include "corecel/data/LdgIterator.hh"  // IWYU pragma: export

namespace celeritas
{
//---------------------------------------------------------------------------//
//! Cast an LdgSpan to a regular Span
template<class T, std::size_t N>
CELER_CONSTEXPR_FUNCTION Span<T const, N>
remove_ldg_wrapper(LdgSpan<T const, N> cont)
{
    return {cont.data(), cont.size()};
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

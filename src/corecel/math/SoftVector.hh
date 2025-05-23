//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/math/SoftVector.hh
//! \brief Soft equivalence operations for vectors.
//---------------------------------------------------------------------------//
#pragma once

#include <cmath>

#include "corecel/Types.hh"
#include "corecel/cont/Array.hh"

#include "detail/SoftEqualTraits.hh"

namespace celeritas
{
//---------------------------------------------------------------------------//
/*!
 * Test for being approximately a unit vector.
 *
 * Consider a unit vector \em v with a small perturbation along a unit vector
 * \em e : \f[
   \vec v + \epsilon \vec e
  \f]
 * The magnitude of this "nearly unit" is
 * \f[
  m^2 = (v + \epsilon e) \cdot (v + \epsilon e)
   = 1 + 2 (v \cdot e) \epsilon + \epsilon^2
 \f]
 * The perturbation \f$\epsilon\f$ means that moving a unit length will result
 * in only a small positional error.
 *
 * Since by the triangle inequality \f[ |v \cdot e|  <= |v||e| = 1 \f] ,
 * then the magnitude squared of a perturbed unit vector is bounded
 * \f[
  m^2 = 1 \pm 2 \epsilon + \epsilon^2
  \f]
 *
 * Instead of calculating the square of the tolerance we use
 * \f$ \epsilon^2 < \epsilon \f$ to make the "soft unit vector" condition
 * \f[
   | \vec v \vd \vec v - 1 | < 3 \epsilon .
   \f]
 */
template<class T = ::celeritas::real_type>
class SoftUnit
{
  public:
    //!@{
    //! \name Type aliases
    using value_type = T;
    //!@}

  public:
    // Construct with explicit tolerance
    CELER_FUNCTION inline SoftUnit(value_type tol);

    // Construct with default tolerance
    CELER_CONSTEXPR_FUNCTION SoftUnit();

    // Calculate whether the array is nearly a unit vector
    template<::celeritas::size_type N>
    CELER_CONSTEXPR_FUNCTION bool operator()(Array<T, N> const& arr) const;

  private:
    value_type tol_;
};

//---------------------------------------------------------------------------//
/*!
 * Test for being approximately orthogonal unit vectors.
 *
 * Consider two unit vectors \em a and \em b and a small perturbation along a
 * unit vector \em e, and let \f[
   \vec b' \equiv \vec b + \epsilon \vec e \quad.
  \f]
 * If \em a and \em b are exactly orthogonal, then the nearly orthogonal vector
 * has a dot product
 * \f[
  \vec a \cdot \vec b'
   = \vec a \cdot \vec b + \epsilon \vec a \cdot e
   = \epsilon \vec a \cdot e \,.
 \f]
 *
 * By the triangle inequality \f[ |a \cdot e|  <= |a||e| = 1 \f] ,
 * so we have the soft orthogonality condition
 * \f[
   \vec a \cdot \vec b' < \epsilon .
  \f]
 *
 * The only difference from \c soft_zero is that the tolerance \f$ \epsilon \f$
 * is actually a \em relative error rather than the absolute error used by
 * \c soft_zero.
 */
template<class T = ::celeritas::real_type>
class SoftOrthogonal
{
  public:
    //!@{
    //! \name Type aliases
    using value_type = T;
    //!@}

  public:
    // Construct with explicit tolerance
    CELER_FUNCTION inline SoftOrthogonal(value_type tol);

    // Construct with default tolerance
    CELER_CONSTEXPR_FUNCTION SoftOrthogonal();

    // Calculate whether the array is nearly a unit vector
    template<::celeritas::size_type N>
    CELER_CONSTEXPR_FUNCTION bool
    operator()(Array<T, N> const& a, Array<T, N> const& b) const;

  private:
    value_type tol_;
};

//---------------------------------------------------------------------------//
// TEMPLATE DEDUCTION GUIDES
//---------------------------------------------------------------------------//
template<class T>
CELER_FUNCTION SoftUnit(T) -> SoftUnit<T>;

template<class T>
CELER_FUNCTION SoftOrthogonal(T) -> SoftOrthogonal<T>;

//---------------------------------------------------------------------------//
// FREE FUNCTIONS
//---------------------------------------------------------------------------//
// Test for being approximately a unit vector
template<class T, size_type N>
CELER_CONSTEXPR_FUNCTION bool is_soft_unit_vector(Array<T, N> const& v);

// Test for being approximately a unit vector
template<class T, size_type N>
CELER_CONSTEXPR_FUNCTION bool
is_soft_orthogonal(Array<T, N> const& a, Array<T, N> const& b);

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Construct with explicit tolereance.
 */
template<class T>
CELER_FUNCTION SoftUnit<T>::SoftUnit(T tol) : tol_{3 * tol}
{
    CELER_EXPECT(tol_ > 0);
}

//---------------------------------------------------------------------------//
/*!
 * Construct with default tolereance.
 */
template<class T>
CELER_CONSTEXPR_FUNCTION SoftUnit<T>::SoftUnit()
    : tol_{3 * detail::SoftEqualTraits<T>::rel_prec()}
{
}

//---------------------------------------------------------------------------//
/*!
 * Calculate whether the array is nearly a unit vector.
 *
 * The calculation below is equivalent to
 * \code
 * return SoftEqual{tol, tol}(1, dot_product(arr, arr));
 * \endcode.
 */
template<class T>
template<::celeritas::size_type N>
CELER_CONSTEXPR_FUNCTION bool
SoftUnit<T>::operator()(Array<T, N> const& arr) const
{
    T length_sq{};
    for (size_type i = 0; i != N; ++i)
    {
        length_sq = std::fma(arr[i], arr[i], length_sq);
    }
    return std::fabs(length_sq - 1) < tol_ * std::fmax(real_type(1), length_sq);
}

//---------------------------------------------------------------------------//
/*!
 * Construct with explicit tolereance.
 */
template<class T>
CELER_FUNCTION SoftOrthogonal<T>::SoftOrthogonal(T tol) : tol_{tol}
{
    CELER_EXPECT(tol_ > 0);
}

//---------------------------------------------------------------------------//
/*!
 * Construct with default (*relative*) tolereance.
 */
template<class T>
CELER_CONSTEXPR_FUNCTION SoftOrthogonal<T>::SoftOrthogonal()
    : tol_{detail::SoftEqualTraits<T>::rel_prec()}
{
}

//---------------------------------------------------------------------------//
/*!
 * Calculate whether the two vectors are orthogonal.
 *
 * The calculation below is equivalent to
 * \code
 * return SoftZero{tol}(dot_product(arr, arr));
 * \endcode.
 */
template<class T>
template<::celeritas::size_type N>
CELER_CONSTEXPR_FUNCTION bool
SoftOrthogonal<T>::operator()(Array<T, N> const& a, Array<T, N> const& b) const
{
    T dot{};
    for (size_type i = 0; i != N; ++i)
    {
        dot = std::fma(a[i], b[i], dot);
    }
    return std::fabs(dot) < tol_;
}

//---------------------------------------------------------------------------//
//! Test with default tolerance for being a unit vector
template<class T, size_type N>
CELER_CONSTEXPR_FUNCTION bool is_soft_unit_vector(Array<T, N> const& v)
{
    return SoftUnit<T>{}(v);
}

//---------------------------------------------------------------------------//
//! Test with default tolerance for being a unit vector
template<class T, size_type N>
CELER_CONSTEXPR_FUNCTION bool
is_soft_orthogonal(Array<T, N> const& a, Array<T, N> const& b)
{
    return SoftOrthogonal<T>{}(a, b);
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

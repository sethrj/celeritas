//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/cont/ScopedSwap.hh
//---------------------------------------------------------------------------//
#pragma once

#include <type_traits>
#include <utility>

#include "corecel/Assert.hh"

namespace celeritas
{
namespace detail
{
template<class T>
class ScopedSwapPointerSetter
{
  public:
    explicit ScopedSwapPointerSetter(T* target) : target_{target} {}

    void operator()(T value) const noexcept { *target_ = value; }

  private:
    T* target_;
};

template<class T>
T const& scoped_swap_target(T* target)
{
    CELER_EXPECT(target);
    return *target;
}
}  // namespace detail
//---------------------------------------------------------------------------//
/*!
 * Swap a value into place and restore the original value on destruction.
 *
 * The target must outlive this object. Moving transfers responsibility for
 * restoring the target and leaves the moved-from object inactive.
 *
 * \code
    int value = 0;
    {
        ScopedSwap swap{&value, 1};
        // value == 1
    }
    // value == 0
   \endcode
 */
template<class T, class Setter = detail::ScopedSwapPointerSetter<T>>
class ScopedSwap
{
  public:
    // Swap in a replacement value
    ScopedSwap(T* target, T replacement) noexcept(
        std::is_nothrow_invocable_v<Setter&, T>);

    // Set a replacement value through a custom setter
    ScopedSwap(T original, T replacement, Setter setter) noexcept(
        std::is_nothrow_move_constructible_v<Setter>
        && std::is_nothrow_invocable_v<Setter&, T>);

    // Restore the target on destruction
    ~ScopedSwap() noexcept(std::is_nothrow_invocable_v<Setter&, T>);

    // Transfer restoration responsibility
    ScopedSwap(ScopedSwap&& other) noexcept(
        std::is_nothrow_move_constructible_v<T>
        && std::is_nothrow_move_constructible_v<Setter>);
    ScopedSwap(ScopedSwap const&) = delete;
    ScopedSwap& operator=(ScopedSwap&&) = delete;
    ScopedSwap& operator=(ScopedSwap const&) = delete;

    //! Keep the replacement value without restoring the target
    void dismiss() noexcept { active_ = false; }

    //! Whether restoration is still active
    bool active() const noexcept { return active_; }

    //! Restore the original value immediately
    void restore() noexcept(std::is_nothrow_invocable_v<Setter&, T>);

  private:
    Setter setter_;
    T original_;
    bool active_{true};
};

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
//! Swap in a replacement value
template<class T, class Setter>
ScopedSwap<T, Setter>::ScopedSwap(T* target, T replacement) noexcept(
    std::is_nothrow_invocable_v<Setter&, T>)
    : setter_{target}, original_{detail::scoped_swap_target(target)}
{
    setter_(std::move(replacement));
}

//! Set a replacement value through a custom setter
template<class T, class Setter>
ScopedSwap<T, Setter>::ScopedSwap(
    T original,
    T replacement,
    Setter setter) noexcept(std::is_nothrow_move_constructible_v<Setter>
                            && std::is_nothrow_invocable_v<Setter&, T>)
    : setter_{std::move(setter)}, original_{std::move(original)}
{
    setter_(std::move(replacement));
}

//! Restore the target on destruction
template<class T, class Setter>
ScopedSwap<T, Setter>::~ScopedSwap() noexcept(
    std::is_nothrow_invocable_v<Setter&, T>)
{
    this->restore();
}

//! Restore the original value immediately
template<class T, class Setter>
void ScopedSwap<T, Setter>::restore() noexcept(
    std::is_nothrow_invocable_v<Setter&, T>)
{
    if (active_)
    {
        setter_(std::move(original_));
        active_ = false;
    }
}

//! Transfer restoration responsibility
template<class T, class Setter>
ScopedSwap<T, Setter>::ScopedSwap(ScopedSwap&& other) noexcept(
    std::is_nothrow_move_constructible_v<T>
    && std::is_nothrow_move_constructible_v<Setter>)
    : setter_{std::move(other.setter_)}
    , original_{std::move(other.original_)}
    , active_{std::exchange(other.active_, false)}
{
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

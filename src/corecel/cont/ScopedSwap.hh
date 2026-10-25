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
//---------------------------------------------------------------------------//
/*!
 * Default setter for ScopedSwap.
 */
template<class T>
class DefaultScopedSetter
{
  public:
    DefaultScopedSetter() = default;

    explicit DefaultScopedSetter(T* target) : target_{target} {}

    void operator()(T value) const noexcept { *target_ = value; }

    explicit operator bool() const { return static_cast<bool>(target_); }

  private:
    T* target_ = nullptr;
};
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
template<class T, class Setter = detail::DefaultScopedSetter<T>>
class ScopedSwap
{
    static constexpr bool nothrow_setter
        = std::is_nothrow_invocable_v<Setter&, T>;
    static constexpr bool nothrow_constructible
        = std::is_nothrow_move_constructible_v<T>
          && std::is_nothrow_move_constructible_v<Setter>;

  public:
    // Swap in a replacement value
    ScopedSwap(T* target, T replacement) noexcept(nothrow_setter);

    // Set a replacement value through a custom setter
    ScopedSwap(T original, T replacement, Setter setter) noexcept(
        nothrow_constructible && nothrow_setter);

    // Restore the target on destruction
    ~ScopedSwap() noexcept(nothrow_setter);

    // Transfer restoration responsibility
    ScopedSwap(ScopedSwap&& other) noexcept(nothrow_constructible);
    ScopedSwap(ScopedSwap const&) = delete;
    ScopedSwap& operator=(ScopedSwap&&) = delete;
    ScopedSwap& operator=(ScopedSwap const&) = delete;

    //! Whether restoration is still active
    bool active() const noexcept { return static_cast<bool>(setter_); }

    //! Restore the original value immediately
    void restore() noexcept(nothrow_setter);

  private:
    Setter setter_;
    T original_;

    static T const& checked_target(T* t)
    {
        CELER_EXPECT(t);
        return *t;
    }
};

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
//! Swap in a replacement value
template<class T, class Setter>
ScopedSwap<T, Setter>::ScopedSwap(T* target,
                                  T replacement) noexcept(nothrow_setter)
    : setter_{target}, original_{checked_target(target)}
{
    setter_(std::move(replacement));
}

//! Set a replacement value through a custom setter
template<class T, class Setter>
ScopedSwap<T, Setter>::ScopedSwap(
    T original, T replacement, Setter setter) noexcept(nothrow_constructible
                                                       && nothrow_setter)
    : setter_{std::move(setter)}, original_{std::move(original)}
{
    setter_(std::move(replacement));
}

//! Restore the target on destruction
template<class T, class Setter>
ScopedSwap<T, Setter>::~ScopedSwap() noexcept(nothrow_setter)
{
    this->restore();
}

//! Transfer restoration responsibility
template<class T, class Setter>
ScopedSwap<T, Setter>::ScopedSwap(ScopedSwap&& other) noexcept(
    nothrow_constructible)
    : setter_{std::exchange(other.setter_, {})}
    , original_{std::move(other.original_)}
{
    CELER_ENSURE(!other.active());
}

//---------------------------------------------------------------------------//
//! Restore the original value immediately
template<class T, class Setter>
void ScopedSwap<T, Setter>::restore() noexcept(nothrow_setter)
{
    if (this->active())
    {
        setter_(std::move(original_));
        setter_ = {};
    }
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

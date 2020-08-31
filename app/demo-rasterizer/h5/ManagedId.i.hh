//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file ManagedId.i.hh
//---------------------------------------------------------------------------//

#include <utility>

namespace h5
{
//---------------------------------------------------------------------------//
/*!
 * Construct with HDF5-owned ID (e.g. H5P_DEFAULT).
 */
ManagedId::ManagedId(hid_t id) : id_(id), close_(nullptr)
{
    REQUIRE(id_ >= 0);
}

//---------------------------------------------------------------------------//
/*!
 * Construct with unmanaged ID and corresponding close function.
 *
 * The given ID is allowed to be an invalid value, but its reference count
 * should be 1 if it's freshly initialized.
 */
ManagedId::ManagedId(hid_t id, Deleter close_function)
    : id_(id), close_(close_function)
{
    REQUIRE(close_);
}

//---------------------------------------------------------------------------//
/*!
 * Destructor closes the ID.
 */
ManagedId::~ManagedId()
{
    this->reset();
}

//---------------------------------------------------------------------------//
/*!
 * Move constructor.
 */
ManagedId::ManagedId(ManagedId&& other) noexcept
{
    this->swap(other);
    ENSURE(!other);
}

//---------------------------------------------------------------------------//
/*!
 * Copy constructor.
 */
ManagedId::ManagedId(const ManagedId& other)
    : id_(other.id_), close_(other.close_)
{
    // Increment reference counter *if and only if* we're managing it
    if (*this && close_)
    {
        CHECK(H5Iis_valid(id_));
        int cur_count = H5Iinc_ref(id_);
        CHECK(cur_count > 1);
    }
}

//---------------------------------------------------------------------------//
/*!
 * Assign another ID.
 *
 * If an error occurs releasing this object, the right-hand side won't be
 * affected.
 */
ManagedId& ManagedId::operator=(ManagedId&& other)
{
    // Reset first so that errors don't affect rhs
    this->reset();

    // Swap with result, zeroing it and capturing its values.
    other.swap(*this);
    return *this;
}

//---------------------------------------------------------------------------//
/*!
 * Assign another ID.
 */
ManagedId& ManagedId::operator=(const ManagedId& other)
{
    // Copy-and-swap on uninitialized type: correctly resets/clears IDs
    ManagedId temp(other);
    temp.swap(*this);
    return *this;
}

//---------------------------------------------------------------------------//
/*!
 * Reset to an empty state, calling close if needed.
 */
void ManagedId::reset()
{
    // Pull out ID and close function in case HDF5 errors. (If we don't do this
    // and an HDF5 check fails, there's no way to trigger a std::terminate
    // since the destructor calls reset.)
    hid_t   id       = id_;
    Deleter close_fn = close_;

    // Clear stored ID and close function
    id_    = H5I_UNINIT;
    close_ = nullptr;

    if (close_fn && id >= 0)
    {
        int orig_ref_count;
#if CELERITAS_DEBUG
        orig_ref_count = H5Iget_ref(id);
#endif

        // Close associated ID
        CELER_H5_CALL(close_fn(id));
        ENSURE(!H5Iis_valid(id) || H5Iget_ref(id) == orig_ref_count - 1);
    }
}

//---------------------------------------------------------------------------//
/*!
 * Swap.
 */
void ManagedId::swap(ManagedId& other) noexcept
{
    using std::swap;
    swap(id_, other.id_);
    swap(close_, other.close_);
}

//---------------------------------------------------------------------------//
/*!
 * Access the underlying ID.
 */
inline hid_t ManagedId::operator*() const
{
    REQUIRE(*this);
    return id_;
}

//---------------------------------------------------------------------------//
} // namespace h5

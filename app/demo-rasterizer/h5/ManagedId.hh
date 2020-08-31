//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file ManagedId.hh
//---------------------------------------------------------------------------//
#pragma once

#include <hdf5.h>

#include "Assert.hh"

namespace h5
{
//---------------------------------------------------------------------------//
/*!
 * Hold a reference to an HDF5 ID that closes itself.
 *
 * This RAII class takes care of closing HDF5 IDs when they go out of scope.
 *
 * \code
   // Call with ID and corresponding close function
   ManagedId dataset(H5Dopen(loc, name, H5P_Default),
                     H5Dclose);
   CHECK(dataset);
   // It can be dereferenced to get an hid_t
   hid_t dataspace = H5Dget_space(*dataset);

   // Special values don't have deleter functions
   ManagedId default_plist(H5P_DEFAULT);
   \endcode
 */
class ManagedId
{
  public:
    //@{
    //! Type aliases
    using Deleter = herr_t (*)(hid_t);
    //@}

  public:
    // Empty constructor for uninitialized ID
    ManagedId() = default;

    // Construct with ALWAYS unmanaged ID (e.g. H5P_DEFAULT)
    inline explicit ManagedId(hid_t id);

    // Construct with 'new' ID and corresponding close function
    inline ManagedId(hid_t id, Deleter close_function);

    // Destructor closes the ID
    inline ~ManagedId();

    // Allow move and copy semantics
    inline ManagedId(ManagedId&&) noexcept;
    inline ManagedId(const ManagedId&);
    inline ManagedId& operator=(ManagedId&&);
    inline ManagedId& operator=(const ManagedId&);

    // Delete ID if present; resets to uninitialied state
    inline void reset();

    // Swap
    inline void swap(ManagedId& other) noexcept;

    // >>> ACCESSORS

    // Return HDF5 ID (raise if not valid)
    inline hid_t operator*() const;

    //! Whether we have a valid ID assigned
    explicit operator bool() const { return id_ >= 0; }

    //! Close method, or nullptr if unmanaged ID
    Deleter deleter() const { return close_; }

  private:
    // >>> DATA

    // HDF5 ID handle
    hid_t id_ = H5I_UNINIT;

    // Function pointer to HDF5 close function (i.e. 'deleter')
    Deleter close_ = nullptr;
};

//---------------------------------------------------------------------------//
// FREE FUNCTIONS
//---------------------------------------------------------------------------//
//! Swap two managed IDs
inline void swap(ManagedId& left, ManagedId& right) noexcept
{
    left.swap(right);
}

//---------------------------------------------------------------------------//
} // namespace h5

#include "ManagedId.i.hh"

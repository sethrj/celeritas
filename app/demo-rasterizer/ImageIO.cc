//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file ImageIO.cc
//---------------------------------------------------------------------------//
#include "ImageIO.hh"

#include "h5/ManagedId.hh"

using h5::ManagedId;

namespace demo_rasterizer
{
//---------------------------------------------------------------------------//
//@{
//! I/O routines for JSON
void to_json(nlohmann::json& j, const ImageRunArgs& v)
{
    j = nlohmann::json{{"lower_left", v.lower_left},
                       {"upper_right", v.upper_right},
                       {"rightward_ax", v.rightward_ax},
                       {"vertical_pixels", v.vertical_pixels}};
}

void from_json(const nlohmann::json& j, ImageRunArgs& v)
{
    j.at("lower_left").get_to(v.lower_left);
    j.at("upper_right").get_to(v.upper_right);
    j.at("rightward_ax").get_to(v.rightward_ax);
    j.at("vertical_pixels").get_to(v.vertical_pixels);
}
//@}

//---------------------------------------------------------------------------//
//! Write an array
void to_h5(const char*                        name,
           const celeritas::array<double, 3>& arr,
           const ManagedId&                   loc_id)
{
    REQUIRE(loc_id);

    ManagedId datatype(H5T_NATIVE_DOUBLE);

    // Create dataset on disk
    const hsize_t dims[] = {3};
    ManagedId     filespace(H5Screate_simple(1, dims, NULL), H5Sclose);
    ManagedId     dataset(H5Dcreate(*loc_id,
                                name,
                                *datatype,
                                *filespace,
                                H5P_DEFAULT,
                                H5P_DEFAULT,
                                H5P_DEFAULT),
                      H5Dclose);
    CHECK(dataset);

    // Write from memory to dataset
    ManagedId memspace(H5Screate_simple(1, dims, NULL), H5Sclose);
    CELER_H5_CALL(H5Dwrite(
        *dataset, *datatype, *memspace, *filespace, H5P_DEFAULT, arr.data()));
}

//---------------------------------------------------------------------------//
//! Write a scalar
void to_h5(const char* name, double value, const ManagedId& loc_id)
{
    REQUIRE(loc_id);

    ManagedId datatype(H5T_NATIVE_DOUBLE);

    // Create dataspace on disk
    ManagedId dataset(H5Dopen(*loc_id, name, H5P_DEFAULT), H5Dclose);
    ManagedId filespace(H5Screate(H5S_SCALAR), H5Sclose);
    ManagedId dataset(H5Dcreate(*loc_id,
                                name,
                                *datatype,
                                *filespace,
                                H5P_DEFAULT,
                                H5P_DEFAULT,
                                H5P_DEFAULT),
                      H5Dclose);
    CHECK(dataset);

    // Write from memory to dataset
    ManagedId memspace(H5Screate(H5S_SCALAR), H5Sclose);
    CELER_H5_CALL(H5Dwrite(
        *dataset, *datatype, *memspace, H5S_ALL, H5P_DEFAULT, arr.data()));
}

//---------------------------------------------------------------------------//
void to_h5(const char*                name,
           const UInt2&               dims,
           celeritas::span<const int> data,
           const ManagedId&           loc_id)
{
    ManagedId dataset(H5Dopen(file, name, H5P_DEFAULT), H5Dclose);

    ManagedId     datatype(H5T_NATIVE_INT);
    constexpr int ndims = 2;

    // Set up chunking and compression
    const hsize_t chunk_dims[ndims] = {32, 32};
    Managed_Id    dcpl(H5Pcreate(H5P_DATASET_CREATE), H5Pclose);
    CELER_H5_CALL(H5Pset_chunk(*dcpl, ndims, chunk_dims));
    CELER_H5_CALL(H5Pset_shuffle(*dcpl));
    CELER_H5_CALL(H5Pset_deflate(dcpl_id, 5));

    // Create dataset on disk
    const hsize_t dims[ndims] = {hsize_t(dims[0]), hsize_t(dims[1])};
    ManagedId     filespace(H5Screate_simple(ndims, dims, NULL), H5Sclose);
    ManagedId     dataset(H5Dcreate(*loc_id,
                                name,
                                *datatype,
                                *filespace,
                                H5P_DEFAULT,
                                H5P_DEFAULT,
                                H5P_DEFAULT),
                      H5Dclose);
    CHECK(dataset);

    // Write from memory to dataset
    ManagedId memspace(H5Screate_simple(ndims, dims, NULL), H5Sclose);
    CELER_H5_CALL(H5Dwrite(
        *dataset, *datatype, *memspace, *filespace, H5P_DEFAULT, arr.data()));
}

//---------------------------------------------------------------------------//
void to_h5(const ImageStore& image, const char* filename)
{
    ManagedId file(H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT),
                   H5Fclose);

    to_h5("upper_left", image.upper_left(), file);
    to_h5("pixel_width", image.pixel_width(), file);
    to_h5("upper_left", image.upper_left(), file);
}

//---------------------------------------------------------------------------//
} // namespace demo_rasterizer

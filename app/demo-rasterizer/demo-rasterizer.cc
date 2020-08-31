//----------------------------------*-C++-*----------------------------------//
// Copyright 2020 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file demo-rasterizer.cc
//---------------------------------------------------------------------------//
#include <cstddef>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "comm/Communicator.hh"
#include "comm/ScopedMpiInit.hh"
#include "comm/Utils.hh"
#include "h5/ManagedId.hh"
#include "RDemoRunner.hh"

using namespace celeritas;
using std::cerr;
using std::cout;
using std::endl;

namespace demo_rasterizer
{
//---------------------------------------------------------------------------//
/*!
 * Run, launch, and output.
 */
void run(std::istream& is)
{
    // Read input options
    auto inp = nlohmann::json::parse(is);

    // Initialize GPU
    celeritas::initialize_device(Communicator::comm_world());

    // Load geometry
    auto geo_params = std::make_shared<GeoParams>(
        inp.at("gdml").get<std::string>().c_str());

    // Construct image
    ImageStore image(inp.at("image").get<ImageRunArgs>());
}

} // namespace demo_rasterizer

//---------------------------------------------------------------------------//
/*!
 * Execute and run.
 *
 * \todo This is copied from the other demo apps; move these to a shared driver
 * at some point.
 */
int main(int argc, char* argv[])
{
    ScopedMpiInit scoped_mpi(&argc, &argv);
    Communicator  comm = Communicator::comm_world();
    if (comm.size() != 1)
    {
        if (comm.rank() == 0)
        {
            cerr << "This app is currently serial-only. Run with 1 proc."
                 << endl;
        }
        return EXIT_FAILURE;
    }

    // Process input arguments
    std::vector<std::string> args(argv, argv + argc);
    if (args.size() != 2 || args[1] == "--help" || args[1] == "-h")
    {
        cerr << "usage: " << args[0] << " {input}.json" << endl;
        return EXIT_FAILURE;
    }

    if (args[1] != "-")
    {
        std::ifstream infile(args[1]);
        if (!infile)
        {
            cerr << "fatal: failed to open '" << args[1] << "'" << endl;
            return EXIT_FAILURE;
        }
        demo_rasterizer::run(infile);
    }
    else
    {
        // Read input from STDIN
        demo_rasterizer::run(std::cin);
    }

    return EXIT_SUCCESS;
}

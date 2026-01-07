//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/inp/System.hh
//---------------------------------------------------------------------------//
#pragma once

#include <map>
#include <optional>
#include <string>
#include <variant>

#include "corecel/Types.hh"
#include "corecel/io/LoggerTypes.hh"
#include "corecel/sys/ThreadId.hh"

namespace celeritas
{
namespace inp
{
//---------------------------------------------------------------------------//
// MPI
//---------------------------------------------------------------------------//
/*!
 * Automatically choose whether to use MPI or not.
 *
 * It is allowed for MPI to be enabled before setup (possibly using
 * ScopedMpiSession).
 *
 * The system checks the \c CELER_ENABLE_MPI environment variable. If not set,
 * it defaults to whether MPI was enabled at compile time (\c
 * CELERITAS_USE_MPI). The deprecated \c CELER_DISABLE_PARALLEL environment
 * variable can also disable MPI.
 */
using DefaultMpi = std::monostate;

//! Disable MPI even if configured
struct DisableMpi
{
};

//! Enable MPI
struct EnableMpi
{
    // TODO: add external communicator, or argv/argc?
};

//! Choose whether to use MPI for parallel execution
using Mpi = std::variant<DefaultMpi, DisableMpi, EnableMpi>;

//---------------------------------------------------------------------------//
// EXECUTION
//---------------------------------------------------------------------------//
/*!
 * Automatically choose a runtime execution space.
 *
 * The system checks the \c CELER_ENABLE_DEVICE environment variable. If not
 * set and GPU support is compiled in (\c CELER_USE_DEVICE), it defaults to
 * GPU execution only if at least one GPU device is detected. The deprecated
 * \c CELER_DISABLE_DEVICE environment variable can force CPU execution.
 */
using DefaultExecution = std::monostate;

//! Disable setup and run in Geant4 integration mode (formerly \c
//! CELER_DISABLE)
struct DisableExecution
{
};

//! Immediately kill all tracks (formerly \c CELER_KILL_OFFLOAD)
struct NullExecution
{
};

//! Run on CPU (TODO: openmp vs serial)
struct CpuExecution
{
};

/*!
 * Run on GPU with fine-grained control over resources.
 *
 * The CUDA/HIP stack and heap sizes may be needed for complex geometries with
 * VecGeom, which has dynamic resource requirements. Both are ignored if zero.
 *
 * The device ID allows selecting a specific GPU. If not set, the device is
 * automatically selected based on the MPI rank to ensure each process uses a
 * different GPU when multiple GPUs are available.
 *
 * Asynchronous memory allocation can improve performance but may require
 * driver/runtime support. If not explicitly set, the value is determined by
 * the \c CELER_DEVICE_ASYNC environment variable.
 */
struct GpuExecution
{
    //! Per-thread CUDA stack size (ignored if zero) [B]
    size_type stack_size{};
    //! Global dynamic CUDA heap size (ignored if zero) [B]
    size_type heap_size{};

    //! CUDA/HIP Device ID (automatic if unset)
    DeviceId id;

    //! Support async allocation: default from CELER_DEVICE_ASYNC
    std::optional<bool> async;
};

//! Where to run Celeritas
using Execution = std::variant<DefaultExecution,
                               DisableExecution,
                               NullExecution,
                               CpuExecution,
                               GpuExecution>;

//---------------------------------------------------------------------------//
// LOGGER
//---------------------------------------------------------------------------//
/*!
 * Configure verbosity levels for different logging outputs.
 *
 * The \c global field controls global/problem-level output.  It defaults to
 * the \c CELER_LOG environment variable, or \c status if not set
 *
 * The \c local field controls individual track/event/thread-local output. It
 * defaults to the \c CELER_LOG_LOCAL environment variable, or \c warning if
 * not set.
 */
struct Logger
{
    //! Global/problem-level output (default 'status' if size_)
    LogLevel global{LogLevel::size_};

    //! Individual track/event/thread-local output (default 'warning' if size_)
    LogLevel local{LogLevel::size_};
};

//---------------------------------------------------------------------------//
// PROFILING
//---------------------------------------------------------------------------//
/*!
 * Choose a default automatically.
 *
 * Profiling is enabled only if the \c CELER_ENABLE_PROFILING environment
 * variable is set to true. When enabled, it uses Perfetto profiling if
 * compiled in (\c CELERITAS_USE_PERFETTO), otherwise falls back to device
 * profiling. If profiling is not requested, it defaults to \c
 * DisableProfiling.
 */
using DefaultProfiling = std::monostate;

//! Do not set up performance profiling
struct DisableProfiling
{
};

/*!
 * Enable CPU profiling with Perfetto.
 *
 * If \c tracing_file is specified, performance tracing data is written to
 * that file. If empty, Celeritas connects to the Perfetto tracing daemon
 * instead, allowing real-time monitoring and analysis.
 */
struct PerfettoProfiling
{
    //! Write Perfetto tracing data to this filename, or to daemon if empty
    std::string tracing_file;
};

/*!
 * Enable GPU profiling.
 *
 * This requires CUDA with NVTX support or HIP with rocTX support. It enables
 * device-side profiling that can be visualized with tools like NVIDIA Nsight
 * Systems or AMD ROCProfiler.
 */
struct DeviceProfiling
{
};

//! Choose performance profiling backend
using Profiling = std::variant<DefaultProfiling,
                               DisableProfiling,
                               PerfettoProfiling,
                               DeviceProfiling>;

//---------------------------------------------------------------------------//
/*!
 * Set up system parameters defined once at program startup.
 *
 * \todo Add OpenMP options
 */
struct System
{
    using MapStrStr = std::map<std::string, std::string>;

    //! MPI execution
    Mpi mpi;

    //! Choose running Celeritas on CPU or GPU
    Execution execution;

    //! Enable performance profiling
    Profiling profiling;

    //! Set up logger verbosity
    Logger logger;

    //! Environment variables used for hacky setup/diagnostics
    MapStrStr environment;
};

//---------------------------------------------------------------------------//
}  // namespace inp
}  // namespace celeritas

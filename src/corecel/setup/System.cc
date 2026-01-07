//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/setup/System.cc
//---------------------------------------------------------------------------//
#include "System.hh"

#include <iostream>
#include <variant>

#include "corecel/Config.hh"
#include "corecel/DeviceRuntimeApi.hh"  // IWYU pragma: keep
#include "corecel/Version.hh"

#include "corecel/Assert.hh"
#include "corecel/inp/System.hh"
#include "corecel/io/BuildOutput.hh"
#include "corecel/io/ColorUtils.hh"
#include "corecel/io/JsonPimpl.hh"
#include "corecel/io/LogHandlers.hh"
#include "corecel/io/Logger.hh"
#include "corecel/io/LoggerTypes.hh"
#include "corecel/sys/Device.hh"
#include "corecel/sys/Environment.hh"
#include "corecel/sys/MpiCommunicator.hh"
#include "corecel/sys/ScopedMpiSession.hh"
#include "corecel/sys/Stopwatch.hh"
#include "corecel/sys/TracingSession.hh"

namespace celeritas
{
namespace setup
{
namespace
{
//---------------------------------------------------------------------------//
template<class T, class F, class V>
void set_conditionally(V& dst, bool value)
{
    if (value)
    {
        dst = T{};
    }
    else
    {
        dst = F{};
    }
}

//---------------------------------------------------------------------------//
SystemLoaded setup_system_impl(inp::System const& i)
{
    SystemLoaded result;

    // Merge environment variables
    for (auto const& [key, value] : i.environment)
    {
        bool inserted = environment().insert({key, value});
        CELER_VALIDATE(inserted,
                       << "cannot set environment variable '" << key
                       << "' because it has already been used: instead, "
                          "directly modify inp::System setup");
    }

    // Set up MPI
    Stopwatch get_mpi_time;
    if (std::holds_alternative<inp::EnableMpi>(i.mpi))
    {
        if constexpr (!CELERITAS_USE_MPI)
        {
            CELER_NOT_CONFIGURED("MPI");
        }
        result.mpi = std::make_unique<ScopedMpiSession>();
    }
    else
    {
        CELER_ASSERT(std::holds_alternative<inp::DisableMpi>(i.mpi));
        ScopedMpiSession::disable();
    }
    auto& mpi_comm = comm_world();
    CELER_ASSERT(static_cast<bool>(mpi_comm)
                 == std::holds_alternative<inp::EnableMpi>(i.mpi));

    // Set up loggers
    world_logger() = Logger{[&mpi_comm]() -> LogHandler {
                                if (mpi_comm.rank() == 0)
                                {
                                    return StreamLogHandler{std::clog};
                                }
                                else
                                {
                                    return nullptr;
                                }
                            }(),
                            i.logger.global};
    self_logger() = Logger{[&mpi_comm]() -> LogHandler {
                               if (mpi_comm)
                               {
                                   return LocalMpiHandler{std::clog, mpi_comm};
                               }
                               else
                               {
                                   return MutexedStreamLogHandler{std::clog};
                               }
                           }(),
                           i.logger.local};
    if (result.mpi)
    {
        CELER_LOG(debug) << "MPI initialization took " << get_mpi_time() << "s";
        if (!*result.mpi)
        {
            CELER_LOG(warning)
                << R"(MPI was initialized before calling ScopedMpiSession)";
        }
    }

    // Set up device if GPU execution is requested
    if (auto const* gpu = std::get_if<inp::GpuExecution>(&i.execution))
    {
        if constexpr (!CELER_USE_DEVICE)
        {
            CELER_NOT_CONFIGURED("GPU support");
        }
        if (gpu->id)
        {
            activate_device(Device{gpu->id});
        }
        else
        {
            activate_device(comm_world());
        }

        // Set CUDA/HIP parameters if specified
        if (gpu->stack_size > 0)
        {
            set_cuda_stack_size(gpu->stack_size);
        }
        if (gpu->heap_size > 0)
        {
            set_cuda_heap_size(gpu->heap_size);
        }
    }
    else
    {
        CELER_ASSERT(std::holds_alternative<inp::CpuExecution>(i.execution));
    }

    // Set up profiling
    if (auto const* perfetto = std::get_if<inp::PerfettoProfiling>(&i.profiling))
    {
        if constexpr (!CELERITAS_USE_PERFETTO)
        {
            CELER_NOT_CONFIGURED("Perfetto");
        }
        result.perfetto = std::make_unique<ScopedPerfettoSession>(
            perfetto->tracing_file.c_str());
    }
    else if (std::holds_alternative<inp::DeviceProfiling>(i.profiling))
    {
        if (CELERITAS_USE_HIP)
        {
            CELER_VALIDATE(!CELERITAS_HAVE_ROCTX,
                           << "device profiling is unavailable since the "
                              "system does not include rocTX");
        }
        else if (!CELERITAS_USE_CUDA)
        {
            CELER_NOT_CONFIGURED("CUDA/HIP");
        }
    }

    return result;
}

//---------------------------------------------------------------------------//
void print_setup_message(inp::System const& i)
{
    auto const nocolor = ansi_color();
    auto const bblue = ansi_color('B');
    auto const yellow = ansi_color('y');

    auto msg = CELER_LOG(info);
    msg << "Activated Celeritas version " << bblue << version_string;
    if constexpr (!CELER_USE_DEVICE)
    {
        msg << ansi_color('x') << " (CPU only build)";
    }
    else if (std::holds_alternative<inp::GpuExecution>(i.execution))
    {
        msg << nocolor << " using " << ansi_color('B') << device().name();
    }
    else
    {
        msg << yellow << "(GPU disabled)";
    }
    if constexpr (CELERITAS_USE_MPI)
    {
        if (ScopedMpiSession::status() == ScopedMpiSession::Status::disabled)
        {
            msg << yellow << " (MPI disabled)";
        }
        else
        {
            auto const& comm = celeritas::comm_world();
            msg << nocolor << " with " << bblue << comm.size() << nocolor
                << " process";
            if (comm.size() > 1)
            {
                msg << "es";
            }
        }
    }
    msg << nocolor;
}

//---------------------------------------------------------------------------//
//! Saved copy of configured system
auto& configured_system_uptr()
{
    static std::unique_ptr<inp::System> result;
    return result;
}

}  // namespace

//---------------------------------------------------------------------------//
// DEFAULTS
//---------------------------------------------------------------------------//
//! Set MPI initialization based on environment variables
bool get_default_mpi()
{
    auto use_mpi = getenv_flag("CELER_ENABLE_MPI", CELERITAS_USE_MPI);
    if (use_mpi.defaulted)
    {
        // Check deprecated flag
        auto inverse_flag
            = getenv_flag("CELER_DISABLE_PARALLEL", !use_mpi.value);
        if (!inverse_flag.defaulted)
        {
            // Deprecated flag is set
            CELER_LOG(warning) << "Deprecated environment variable "
                                  "`CELER_DISABLE_PARALLEL`: use "
                                  "`CELER_ENABLE_MPI=0`";
            // Set opposite of flag
            use_mpi.value = !inverse_flag.value;
        }
    }
    return use_mpi.value;
}

//---------------------------------------------------------------------------//
/*!
 * Set logger defaults based on environment variables.
 */
void apply_defaults(inp::Logger& logger)
{
    // Apply environment variable defaults if not explicitly set
    if (logger.global == LogLevel::size_)
    {
        logger.global = getenv_loglevel("CELER_LOG", LogLevel::status);
    }
    if (logger.local == LogLevel::size_)
    {
        logger.local = getenv_loglevel("CELER_LOG_LOCAL", LogLevel::warning);
    }
}

//---------------------------------------------------------------------------//
/*!
 * Set system defaults based on environment and configuration.
 */
void apply_defaults(inp::System& sys)
{
    using namespace celeritas::inp;

    // Set up logger defaults first
    apply_defaults(sys.logger);

    // Set MPI default based on environment
    if (std::holds_alternative<DefaultMpi>(sys.mpi))
    {
        switch (ScopedMpiSession::status())
        {
            case ScopedMpiSession::Status::disabled:
                sys.mpi.emplace<DisableMpi>();
                break;
            case ScopedMpiSession::Status::initialized:
                sys.mpi.emplace<EnableMpi>();
                break;
            case ScopedMpiSession::Status::uninitialized:
                set_conditionally<EnableMpi, DisableMpi>(sys.mpi,
                                                         get_default_mpi());
        };
    }

    // Set execution default
    if (std::holds_alternative<DefaultExecution>(sys.execution))
    {
        auto has_device = [] {
            if (!CELER_USE_DEVICE)
                return false;

            // Using default value and CUDA/HIP are enabled:
            // Manually call device count to avoid logging env logic in
            // Device::get_num_devices (for now)
            int result = -1;
            CELER_DEVICE_API_CALL(GetDeviceCount(&result));
            return result > 0;
        };

        auto use_device = getenv_flag_lazy("CELER_ENABLE_DEVICE", has_device);
        if (use_device.defaulted)
        {
            // Check for old flag if user didn't provide `CELER_ENABLE_DEVICE`
            auto inverse_flag
                = getenv_flag("CELER_DISABLE_DEVICE", !use_device.value);
            if (!inverse_flag.defaulted)
            {
                // Deprecated flag is set
                CELER_LOG(warning) << "Deprecated environment variable "
                                      "`CELER_DISABLE_DEVICE`: use "
                                      "`CELER_ENABLE_DEVICE=0`";
                // Set opposite of flag
                use_device.value = !inverse_flag.value;
            }
        }
        set_conditionally<GpuExecution, CpuExecution>(sys.execution,
                                                      use_device.value);
    }

    // Set profiling default based on environment
    if (std::holds_alternative<DefaultProfiling>(sys.profiling))
    {
        auto enable_profiling = getenv_flag("CELER_ENABLE_PROFILING", false);
        if (enable_profiling.value)
        {
            set_conditionally<PerfettoProfiling, DeviceProfiling>(
                sys.profiling, CELERITAS_USE_PERFETTO);
        }
        else
        {
            sys.profiling = DisableProfiling{};
        }
    }
}  // namespace setup

//---------------------------------------------------------------------------//
// SET UP
//---------------------------------------------------------------------------//
/*!
 * Set up system from input.
 *
 * \pre Default values must be set up in advance with \c apply_defaults.
 */
SystemLoaded system(inp::System const& i)
{
    CELER_VALIDATE(
        !std::holds_alternative<inp::DefaultMpi>(i.mpi)
            && !std::holds_alternative<inp::DefaultExecution>(i.execution)
            && !std::holds_alternative<inp::DefaultProfiling>(i.profiling)
            && i.logger.global != LogLevel::size_
            && i.logger.local != LogLevel::size_,
        << "apply_defaults(inp::System) was not called before setup");

    try
    {
        auto result = setup_system_impl(i);

        print_setup_message(i);

        // Save a copy of the configured system
        configured_system_uptr() = std::make_unique<inp::System>(i);

        return result;
    }
    catch (RuntimeError const&)
    {
        CELER_LOG(critical) << "Failed to set up Celeritas: "
                            << output_to_json(BuildOutput{}).dump(0);
        throw;
    }

    CELER_ASSERT_UNREACHABLE();
}

//---------------------------------------------------------------------------//
/*!
 * Saved copy of the configured input.
 *
 * This is transitional to move away from environment variables.
 */
inp::System const& loaded_system_inp()
{
    auto& uptr_sys = configured_system_uptr();
    CELER_VALIDATE(uptr_sys, << "system input accessed before calling setup");
    return *uptr_sys;
}

//---------------------------------------------------------------------------//
}  // namespace setup
}  // namespace celeritas

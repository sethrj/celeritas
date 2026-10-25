//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file geocel/ScopedGeantLogger.cc
//---------------------------------------------------------------------------//
#include "ScopedGeantLogger.hh"

#include <cctype>
#include <memory>
#include <regex>
#include <G4String.hh>
#include <G4Threading.hh>
#include <G4Types.hh>
#include <G4UImanager.hh>
#include <G4Version.hh>
#include <G4coutDestination.hh>

#include "corecel/Macros.hh"
#if G4VERSION_NUMBER >= 1120
#    include <G4ios.hh>
#    define CELER_G4SSBUF 0
#else
#    include <G4strstreambuf.hh>
#    define CELER_G4SSBUF 1
#endif

#include "corecel/Assert.hh"
#include "corecel/cont/ScopedSwap.hh"
#include "corecel/io/Logger.hh"
#include "corecel/io/StringUtils.hh"

namespace celeritas
{
namespace
{
//---------------------------------------------------------------------------//
//! Get a string view matched by a regular expression
template<class T>
std::string_view to_string_view(std::sub_match<T> const& cs)
{
    if (!cs.matched)
    {
        return {};
    }
    return {&(*cs.first), static_cast<std::size_t>(cs.length())};
}

//---------------------------------------------------------------------------//
/*!
 * Log the actual message.
 *
 * Example inputs (cout):
 * - "*** G4Navigator::LocateGlobalPointAndSetup: ***"
 * - "    Called with arguments: "
 * Examples (cerr):
 * - "!!! Csv file name not defined."
 * - "ERROR : G4VScoringMesh::SetPrimitiveScorer() : <message>"
 */
G4int log_impl(celeritas::Logger& log, G4String const& str, LogLevel level)
{
    // Try to guess the log level based on each line
    static std::regex const err_warn_regex{
        R"regex(^\W*(\w+)?\s*(warning|error|!+|\*+)\W+)regex",
        std::regex::icase};
    static std::regex const highlighted_regex{
        R"regex(^\s*\*+\s+(\S+):\s+\*+\s*$)regex"};
    static std::regex const info_regex{R"regex(^(\w+):\s+)regex"};

    std::smatch m;
    std::string_view msg;
    std::string_view source{"Geant4"};
    if (std::regex_search(str, m, err_warn_regex))
    {
        CELER_ASSERT(m.size() == 3);
        if (m[1].matched)
        {
            // Warning is coming from somewhere in particular
            source = to_string_view(m[1]);
        }

        // Strip the beginning text from the err/warning
        msg = to_string_view(m.suffix());
        // Update the warning level
        auto first_char = std::tolower(static_cast<unsigned char>(*m[2].first));
        if (first_char == 'w')
        {
            level = LogLevel::warning;
        }
        else if (first_char == '*')
        {
            if (std::regex_search(str, m, highlighted_regex))
            {
                // Not actually a warning: just code location (leave as is)
                msg = str;
            }
            else
            {
                level = LogLevel::warning;
            }
        }
        else if (first_char == 'e' || first_char == '!')
        {
            level = LogLevel::error;
        }
        else
        {
            CELER_ASSERT_UNREACHABLE();
        }
    }
    else if (std::regex_search(str, m, info_regex))
    {
        CELER_ASSERT(m.size() == 2);
        source = to_string_view(m[1]);
        msg = to_string_view(m.suffix());
    }
    else
    {
        msg = str;
    }

    // Output with dummy file/line
    log({source, 0}, level) << trim(msg);

    // 0 for success, -1 for failure
    return 0;
}

//---------------------------------------------------------------------------//
//! Thread-local flags for ownership/usability of the Geant4 logger
enum class SGLState
{
    inactive,
    active,
    disabled
};

SGLState& sgl_state()
{
    static G4ThreadLocal SGLState result{SGLState::inactive};
    return result;
}

#if CELER_G4SSBUF
struct G4CoutDestinationSetter
{
    void operator()(G4coutDestination* destination) const noexcept
    {
        G4coutbuf.SetDestination(destination);
    }
};

struct G4CerrDestinationSetter
{
    void operator()(G4coutDestination* destination) const noexcept
    {
        G4cerrbuf.SetDestination(destination);
    }
};
#else
struct G4DestinationSetter
{
    void operator()(G4coutDestination* destination) const noexcept
    {
        G4iosSetDestination(destination);
    }
};
#endif

//---------------------------------------------------------------------------//
}  // namespace

//---------------------------------------------------------------------------//
/*!
 * Send Geant4 log messages to Celeritas' world logger.
 */
class ScopedGeantLogger::StreamDestination final : public G4coutDestination
{
  public:
    // Assign to Geant handlers on construction
    explicit StreamDestination(Logger&);
    CELER_DELETE_COPY_MOVE(StreamDestination);
    ~StreamDestination() final;

    // Handle error messages
    G4int ReceiveG4cout(G4String const& str) final;
    G4int ReceiveG4cerr(G4String const& str) final;

    // Access logger from ScopedGeantLogger cleanup
    Logger& logger() { return celer_log_; }

  private:
    Logger& celer_log_;
#if CELER_G4SSBUF
    using CoutSwap = ScopedSwap<G4coutDestination*, G4CoutDestinationSetter>;
    using CerrSwap = ScopedSwap<G4coutDestination*, G4CerrDestinationSetter>;
    std::unique_ptr<CoutSwap> cout_swap_;
    std::unique_ptr<CerrSwap> cerr_swap_;
#else
    using DestinationSwap = ScopedSwap<G4coutDestination*, G4DestinationSetter>;
    std::unique_ptr<DestinationSwap> destination_swap_;
#endif
};

//---------------------------------------------------------------------------//
/*!
 * Redirect geant4's stdout/cerr on construction.
 *
 * Note that all these buffers, and the UI pointers, are thread-local.
 */
ScopedGeantLogger::StreamDestination::StreamDestination(Logger& celer_log)
    : celer_log_{celer_log}
{
    // Make sure UI pointer has been instantiated, since its constructor
    // resets the cout destination
    if (!G4UImanager::GetUIpointer())
    {
        // Always-on debug assertion (not a "runtime" error but a
        // subtle programming logic error that always causes a crash)
        CELER_DEBUG_FAIL(
            R"(Geant4 logging cannot be changed after G4UImanager has been destroyed)",
            precondition);
    }

#if CELER_G4SSBUF
    cout_swap_ = std::make_unique<CoutSwap>(
        G4coutbuf.GetDestination(), this, G4CoutDestinationSetter{});
    cerr_swap_ = std::make_unique<CerrSwap>(
        G4cerrbuf.GetDestination(), this, G4CerrDestinationSetter{});
#else
    // See Geant4 change global-V11-01-01
    destination_swap_ = std::make_unique<DestinationSwap>(
        nullptr, this, G4DestinationSetter{});
#endif
}

//---------------------------------------------------------------------------//
/*!
 * Restore iostream buffers on destruction.
 */
ScopedGeantLogger::StreamDestination::~StreamDestination() = default;

//---------------------------------------------------------------------------//
/*!
 * Process stdout message.
 */
G4int ScopedGeantLogger::StreamDestination::ReceiveG4cout(G4String const& str)
{
    return log_impl(celer_log_, str, LogLevel::diagnostic);
}

//---------------------------------------------------------------------------//
/*!
 * Process stderr message.
 */
G4int ScopedGeantLogger::StreamDestination::ReceiveG4cerr(G4String const& str)
{
    return log_impl(celer_log_, str, LogLevel::info);
}

//---------------------------------------------------------------------------//
//! Enable and disable to avoid recursion with accel/Logger
//!@{
bool ScopedGeantLogger::enabled()
{
    return sgl_state() != SGLState::disabled;
}

void ScopedGeantLogger::enabled(bool)
{
    CELER_EXPECT(sgl_state() != SGLState::active);
    sgl_state() = SGLState::disabled;
}
//!@}

//---------------------------------------------------------------------------//
/*!
 * Install the Celeritas Geant4 logger.
 *
 * A global flag allows multiple logger adapters to be nested without
 * consequence.
 */
ScopedGeantLogger::ScopedGeantLogger(Logger& celer_log)
{
    auto& state = sgl_state();
    if (state == SGLState::inactive)
    {
        celer_log(CELER_CODE_PROVENANCE, LogLevel::debug)
            << "Activating ScopedGeantLogger";

        state = SGLState::active;
        logger_ = std::make_unique<StreamDestination>(celer_log);
    }
    else
    {
        celer_log(CELER_CODE_PROVENANCE, LogLevel::debug)
            << R"(Ignoring ScopedGeantLogger initialization because )"
            << (state == SGLState::active
                    ? R"(Geant4 is already being redirected on this thread)"
                : state == SGLState::disabled ? R"(log redirection is disabled)"
                                              : "<unreachable>");
    }
}

//---------------------------------------------------------------------------//
/*!
 * Install the Celeritas Geant4 logger using the world logger.
 */
ScopedGeantLogger::ScopedGeantLogger()
    : ScopedGeantLogger{celeritas::world_logger()}
{
}

//---------------------------------------------------------------------------//
/*!
 * Revert to the previous exception handler.
 */
ScopedGeantLogger::~ScopedGeantLogger()
{
    auto& state = sgl_state();
    if (state == SGLState::active)
    {
        state = SGLState::inactive;
    }
    if (logger_)
    {
        logger_->logger()(CELER_CODE_PROVENANCE, LogLevel::debug)
            << "Resetting ScopedGeantLogger";
        logger_.reset();
    }
}

//---------------------------------------------------------------------------//
}  // namespace celeritas

//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/ScopedLogStorer.cc
//---------------------------------------------------------------------------//
#include "ScopedLogStorer.hh"

#include <iostream>
#include <memory>
#include <regex>

#include "corecel/io/ColorUtils.hh"
#include "corecel/io/Logger.hh"
#include "corecel/io/LoggerTypes.hh"
#include "corecel/io/Repr.hh"
#include "corecel/sys/Environment.hh"

#include "StringSimplifier.hh"

namespace celeritas
{
namespace test
{
namespace
{
void debug_clog(LogProvenance, LogLevel lev, std::string msg)
{
    std::clog << color_code('x') << to_cstring(lev) << ": " << msg
              << color_code(' ') << std::endl;
}
}  // namespace

//---------------------------------------------------------------------------//
/*!
 * Construct reference to log to temporarily replace.
 */
ScopedLogStorer::ScopedLogStorer(Logger* orig, LogLevel min_level)
    : logger_{orig}
{
    CELER_EXPECT(logger_);
    CELER_EXPECT(min_level != LogLevel::size_);
    result_ = std::make_shared<ScopedLogResult>();
    scoped_logger_ = std::make_unique<ScopedSwap<Logger>>(
        logger_,
        Logger{LogHandler{ScopedLogHandler{result_, min_level}},
               LogLevel::debug});
}

//---------------------------------------------------------------------------//
/*!
 * Construct reference to log to temporarily replace.
 */
ScopedLogStorer::ScopedLogStorer(Logger* orig)
    : ScopedLogStorer{orig, Logger::default_level()}
{
}

//---------------------------------------------------------------------------//
/*!
 * Restore original logger on destruction.
 */
ScopedLogStorer::~ScopedLogStorer() = default;

//---------------------------------------------------------------------------//
//! Save a log message through the shared result handler
void ScopedLogHandler::operator()(
    LogProvenance prov, LogLevel lev, std::string msg)
{
    static LogLevel const debug_level
        = getenv_loglevel("CELER_LOG_SCOPED", LogLevel::warning);
    if (lev >= debug_level)
    {
        if (getenv_flag("CELER_LOG_SCOPED_VERBOSE", false).value)
        {
            // Print entire message
            debug_clog(prov, lev, msg);
        }
        else
        {
            // Strip colors and only write the first line
            static std::regex const strip_ansi_regex("\033\\[[0-9;]*m");
            auto temp_msg = std::regex_replace(msg, strip_ansi_regex, "");
            auto newline_pos = temp_msg.find_first_of('\n');
            if (newline_pos != std::string::npos)
            {
                // Erase newline and after
                temp_msg.erase(newline_pos);
                temp_msg
                    += "... [truncated: set CELER_LOG_SCOPED_VERBOSE to show]";
            }
            // Trim trailing whitespace so that captured output stays
            // consistent with whitespace-cleaned regression baselines
            temp_msg.erase(temp_msg.find_last_not_of(" \t") + 1);
            debug_clog(prov, lev, temp_msg);
        }
    }

    if (lev < min_level_)
    {
        return;
    }

    msg = StringSimplifier{float_digits_}(std::move(msg));

    std::lock_guard<std::mutex> scoped_lock{result_->mutex_};
    result_->messages.emplace_back(std::move(msg));
    result_->levels.push_back(to_cstring(lev));
}

//---------------------------------------------------------------------------//
//! Delete messages that contain a certain string
void ScopedLogStorer::remove_if_contains(std::string const& s)
{
    auto it_msg = result_->messages.begin();
    auto it_lev = result_->levels.begin();

    while (it_msg != result_->messages.end())
    {
        if (it_msg->find(s) != std::string::npos)
        {
            it_msg = result_->messages.erase(it_msg);
            it_lev = result_->levels.erase(it_lev);
        }
        else
        {
            ++it_msg;
            ++it_lev;
        }
    }
}

//---------------------------------------------------------------------------//
//! Print the expected values
void ScopedLogStorer::print_expected() const
{
    using std::cout;
    using std::endl;
    cout << "/*** ADD THE FOLLOWING UNIT TEST CODE ***/\n"
            "static char const* const expected_log_messages[] = "
         << repr(this->messages())
         << ";\n"
            "EXPECT_VEC_EQ(expected_log_messages, scoped_log_.messages());\n"
            "static char const* const expected_log_levels[] = "
         << repr(this->levels())
         << ";\n"
            "EXPECT_VEC_EQ(expected_log_levels, scoped_log_.levels());\n"
            "/*** END CODE ***/"
         << endl;
}

//---------------------------------------------------------------------------//
/*!
 * Print expected results.
 */
std::ostream& operator<<(std::ostream& os, ScopedLogStorer const& logs)
{
    os << "messages: " << repr(logs.messages())
       << "\n"
          "levels: "
       << repr(logs.levels());
    return os;
}

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas

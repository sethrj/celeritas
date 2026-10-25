//------------------------------- -*- C++ -*- -------------------------------//
// Copyright Celeritas contributors: see top-level COPYRIGHT file for details
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file corecel/ScopedLogStorer.hh
//---------------------------------------------------------------------------//
#pragma once

#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

#include "corecel/Assert.hh"
#include "corecel/cont/ScopedSwap.hh"
#include "corecel/io/LoggerTypes.hh"

namespace celeritas
{
class Logger;
namespace test
{
//---------------------------------------------------------------------------//

struct ScopedLogResult
{
    std::vector<std::string> messages;
    std::vector<std::string> levels;

    //! Clear results
    void clear()
    {
        messages.clear();
        levels.clear();
    }

    std::mutex mutex_;
};

//---------------------------------------------------------------------------//
class ScopedLogHandler
{
  public:
    ScopedLogHandler(std::shared_ptr<ScopedLogResult> result,
                     LogLevel min_level)
        : result_{std::move(result)}, min_level_(min_level)
    {
    }

    //! Write a log message
    void operator()(LogProvenance, LogLevel, std::string);

    //! Get the minimum level being recorded
    LogLevel level() const { return min_level_; }

    //! Change the level to record
    void level(LogLevel lev) { min_level_ = lev; }

    //! Get the minimum level being recorded
    int float_digits() const { return float_digits_; }

    //! Change the level to record
    void float_digits(int fd)
    {
        CELER_EXPECT(fd >= 0);
        float_digits_ = fd;
    }

  private:
    std::shared_ptr<ScopedLogResult> result_;
    LogLevel min_level_{LogLevel::warning};
    int float_digits_{4};
};

//---------------------------------------------------------------------------//
/*!
 * Log handle for saving messages for testing.
 *
 * Temporarily replace the given logger with this function. This removes ANSI
 * sequences and replaces pointer-like strings with 0x0.
 *
 * You can use the \c CELER_LOG_SCOPED environment variable to print the
 * captured log messages as they are written.
 *
 * Messages may be written concurrently from multiple threads, but the
 * accessors must not be called while other threads are still logging.
 *
 * \par Example:
 * \code
    ScopedLogStorer scoped_log_{&celeritas::world_logger()};
    CELER_LOG(info) << "captured";
    scoped_log_.print_expected();
    EXPECT_TRUE(scoped_log_.empty()) << scoped_log_;
   \endcode
 */
class ScopedLogStorer
{
  public:
    //!@{
    //! \name Type aliases
    using VecString = std::vector<std::string>;
    //!@}

  public:
    // Construct reference to log to temporarily replace
    ScopedLogStorer(Logger* orig, LogLevel min_level);

    // Construct reference with default level
    explicit ScopedLogStorer(Logger* orig);

    // No default constructor
    ScopedLogStorer() = delete;

    // Careful data management required due to pointer-to-this
    ScopedLogStorer(ScopedLogStorer&&) = default;
    ScopedLogStorer(ScopedLogStorer const&) = delete;
    ScopedLogStorer& operator=(ScopedLogStorer&&) = delete;
    ScopedLogStorer& operator=(ScopedLogStorer const&) = delete;

    // Restore original logger on destruction
    ~ScopedLogStorer();

    //! Whether no messages were stored
    bool empty() const { return result_->messages.empty(); }

    //! Get saved messages
    VecString const& messages() const { return result_->messages; }

    //! Get corresponding log levels
    VecString const& levels() const { return result_->levels; }

    // Delete messages that contain a certain string
    void remove_if_contains(std::string const& s);

    // Print expected results to stdout
    void print_expected() const;

    //! Clear results
    void clear() { result_->clear(); }

  private:
    Logger* logger_{nullptr};
    std::shared_ptr<ScopedLogResult> result_;
    std::unique_ptr<ScopedSwap<Logger>> scoped_logger_;
};

// Print expected results
std::ostream& operator<<(std::ostream& os, ScopedLogStorer const& logs);

//---------------------------------------------------------------------------//
}  // namespace test
}  // namespace celeritas

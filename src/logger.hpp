// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2025 Frank Dean <frank.dean@fdsd.co.uk>

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU Affero General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
    for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <array>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <ostream>
#include <sstream>
#include <string>
#include <syslog.h>

namespace fdsd
{
namespace utils
{

class Logger
{
public:
  enum log_level {
    emergency = LOG_EMERG, alert = LOG_ALERT, critical = LOG_CRIT,
    error = LOG_ERR, warn = LOG_WARNING, notice = LOG_NOTICE, info = LOG_INFO,
    debug = LOG_DEBUG
  };
  enum manip {
    /// Write a linefeed to the stream and sets the new_line flag to true
    endl,

    /// Write sets the new_line flag to true without writing a new line
    newline
  };
private:
  static const std::array<std::string, 8> levels;

  /// The label is included to help identify entries in the log
  std::string label;

  /// The stream we are logging to
  std::ostream& os;

  /// For logging to syslog
  static std::ostringstream s_syslog;

  /// The target logging level of the class.  Everything of lower priority is
  /// ignored.
  log_level level;

  /// The current log level for logging performed via streaming to this
  /// instance.
  log_level manip_level;

  /// True if the next log message should prefix output with date, label and
  /// log level.
  bool new_line;

  /// Writes the current date and time to the stream.
  void put_now() const;
public:
  Logger(std::string label,
         std::ostream& stream = std::clog,
         log_level level = info)
    : label(label),
      os(stream),
      level(level),
      manip_level(level),
      new_line(true) {}

  void log(std::string s, log_level log_level = debug);

  bool is_level(log_level log_level) const {
    return (log_level <= level);
  }

  inline friend Logger& operator<<(Logger& logger,
                                   std::ostream& (*manip)(std::ostream&)) {
    if (logger.manip_level <= logger.level) {
      manip(logger.os);
    }
    return logger;
  }

  inline friend Logger& operator<<(Logger& logger, log_level level) {
    logger.manip_level = level;
    return logger;
  }

  inline friend Logger& operator<<(Logger& logger, manip m) {
    if (logger.manip_level <= logger.level) {
      if (m == Logger::endl || m == Logger::newline)
        logger.new_line = true;
      if (m == Logger::endl)
        logger.os << std::endl;
    }
    return logger;
  }

  template <typename T>
  inline friend Logger& operator<<(Logger& logger, const T& value) {
    if (logger.manip_level <= logger.level) {
      if (logger.new_line) {
        logger.put_now();
        logger.os << ' ' << logger.label << " ["
                   << Logger::levels[logger.manip_level] << "] ";
        // Make sure we're at the end of the stream
        s_syslog.flush();
        s_syslog.seekp(0, std::ios_base::end);
        if (s_syslog.tellp() != 0) {
          syslog(logger.manip_level,
                 "[%s] %s",
                 levels[logger.manip_level].c_str(),
                 s_syslog.str().c_str());
          s_syslog.clear();
          s_syslog.str("");
        }
      }
      logger.os << value;
      s_syslog << value;
      logger.new_line = false;
    }
    return logger;
  }

};

} // namespace utils
} // namespace fdsd

#endif // LOGGER_HPP

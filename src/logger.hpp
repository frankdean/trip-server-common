// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022 Frank Dean <frank.dean@fdsd.co.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
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
#include <string>

namespace fdsd
{
namespace utils
{

class Logger
{
public:
  enum log_level {
    emergency, alert, critical, error, warn, notice, info, debug
  };
  enum manip {
    /// Write a linefeed to the stream and sets the new_line flag to true
    endl,

    /// Write sets the new_line flag to true without writing a new line
    newline
  };
private:
  static const std::array<std::string, 8> levels;

  // The label is included to help identify entries in the log
  std::string label;

  // The stream we are logging to
  std::ostream& os;

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
  void put_now() const {
    std::time_t t =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm* nowTM = std::localtime(&t);

    const std::time_put<char>& tp =
      std::use_facet<std::time_put<char>>(std::locale());
    std::string format = "%F %T %z";
    tp.put(os, os, ' ', nowTM,
           format.c_str(), format.c_str() + format.size());
  }
public:
  Logger(std::string label,
         std::ostream& stream = std::clog,
         log_level level = info)
    : label(label),
      os(stream),
      level(level),
      manip_level(level),
      new_line(true) {}

  void log(std::string s, log_level log_level = debug) {
    if (log_level <= level) {
      put_now();
      os << ' ' << label << " [" << levels[level] << "] "<< s
          << '\n';
    }
  }

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
      }
      logger.os << value;
      logger.new_line = false;
    }
    return logger;
  }

};

} // namespace utils
} // namespace fdsd

#endif // LOGGER_HPP

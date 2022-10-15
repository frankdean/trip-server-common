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
#ifndef DATE_UTILS_HPP
#define DATE_UTILS_HPP

#include "logger.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <regex>
#include <sstream>
#include <string>

namespace fdsd
{
namespace utils
{

  const int year_offset = 1900;
  const int month_offset = 1;

  /**
   * This class represents times in the local time, based on the default
   * locale.
   *
   * When constructed from the various date sources, times are treated as
   * though they are local times, with any time zone information being
   * ignored.  If time zone conversion is required, convert times to the local
   * time before using this class.
   *
   * This allows consistent display of times regardless of whether DST is
   * relevant to now or the time being referenced.
   */
  class DateTime
  {
  private:
    std::chrono::system_clock::time_point datetime;
    // std::time_t datetime;
    std::tm convert(std::time_t t) const;
    static Logger logger;
  public:
    static const std::regex numeric_regex;
    // static const std::regex valid_yyyy_mm_dd_regex;
    static const std::regex iso8601_regex;
    enum date_format {
      yyyy_mm_dd_hh_mm_ss,
      yyyy_mm_dd_hh_mm_ss_z,
      yyyy_mm_dd,
      dd_mon_yyyy_hh_mm_ss,
      // dd_mm_yy_hh_mm_ss,
      // mm_dd_yy_hh_mm_ss,
      // dd_mm_yyyy_hh_mm_ss,
      // mm_dd_yyyy_hh_mm_ss
    };
    DateTime();
    DateTime(std::string);
    DateTime(std::tm &tm);
    // DateTime(std::time_t);
    DateTime(std::chrono::system_clock::time_point);
    DateTime(int year, int month, int day,
             int hour = 0, int minute = 0, int second = 0);
    void init(std::string);
    // void init(std::time_t);
    void set_time_t(std::time_t t);
    void set_ms(long long ms);
    date_format default_format;
    long long get_ms() const {
      return std::chrono::duration_cast<std::chrono::milliseconds>(
          datetime.time_since_epoch()).count();
    }
    std::time_t get_time() const {
      return std::chrono::system_clock::to_time_t(datetime);
    }
    std::time_t time_t() const {
      return get_time();
    }
    std::chrono::system_clock::time_point time_tp() const {
      return datetime;
    }
    std::tm get_time_as_tm() const {
      return convert(get_time());
    }
    std::string get_time_as_rfc7231() const;
    std::string get_time_as_iso8601_gmt() const;
    std::string to_string(date_format format) const;
    std::string to_string() const {
      return to_string(yyyy_mm_dd_hh_mm_ss);
    }
    inline friend std::ostream& operator<<
        (std::ostream& out, const DateTime& rhs) {
      return out << rhs.to_string();
    }
    inline friend bool operator== (const DateTime& lhs, const DateTime& rhs) {
      return lhs.datetime == rhs.datetime;
    }
    inline friend bool operator< (const DateTime& lhs, const DateTime& rhs) {
      return lhs.datetime < rhs.datetime;
    }
    std::chrono::system_clock::time_point period_start_date(
        std::chrono::system_clock::time_point base_tp, int frequency) const;
    std::chrono::system_clock::time_point period_end_date(
        std::chrono::system_clock::time_point base_tp, int frequency) const;
  };

} // namespace utils
} // namespace fdsd

#endif // DATE_UTILS_HPP

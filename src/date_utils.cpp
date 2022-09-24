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
#include "date_utils.hpp"
#include <locale>
#include <mutex>
#include <stdexcept>

using namespace fdsd::utils;

Logger DateTime::logger = Logger("date_utils", std::clog, Logger::info);

const std::regex DateTime::numeric_regex = std::regex("-?[0-9]+(\\.{1})?([0-9]+)?");
/// Regex to ensure the date part of a string looks valid
const std::regex DateTime::valid_yyyy_mm_dd_regex =
  std::regex("[0-9]{1,4}[^0-9]{1}[0-9]{2}[^0-9]{1}[0-9]{2}.*");

// const std::string invalid_date_format = "Invalid date format";

/// Mutex used to lock access to non-threadsafe functions
std::mutex g_datetime_mutex;

DateTime::DateTime() : default_format(yyyy_mm_dd_hh_mm_ss)
{
  datetime = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now());
}

DateTime::DateTime(std::string date, date_format format) : DateTime()
{
  init(date, format);
}

void DateTime::init(std::string date, date_format format)
{
  // std::cout << "DateTime converting \"" << date << "\"\n";
  if (std::regex_match(date, numeric_regex)) {
    init(std::stol(date));
    return;
  }
  std::tm tm {};
  switch (format){
    case dd_mon_yyyy_hh_mm_ss: {
      // std::cout << "Converting \"" << date << "\" as dd_mon_yyyy_hh_mm_ss\n";
      std::istringstream is(date);
      is >> std::get_time(&tm, "%d %b %Y %T");
      if (is.fail())
        logger << Logger::debug << "Parsing of date failed" << Logger::endl;
      std::ostringstream os;
      os << std::put_time(&tm, "%FT%T");
      date = os.str();
      break;
    }
    case yyyy_mm_dd_hh_mm_ss_z:
    case yyyy_mm_dd_hh_mm_ss:
    case yyyy_mm_dd: {
      break;
    }
  }

  // The date should now be in the format similar to yyyy_mm_dd..
  if (std::regex_match(date, valid_yyyy_mm_dd_regex)) {
    // std::cout << "Converting date: \"" << date << "\"\n";
    try {
      auto n = date.size();
      if (n > 3)
        tm.tm_year = std::stoi(date.substr(0, 4)) - year_offset;
      if (n > 6)
        tm.tm_mon = std::stoi(date.substr(5, 2)) - month_offset;
      if (n > 9)
        tm.tm_mday = std::stoi(date.substr(8, 2));
      if (format == yyyy_mm_dd_hh_mm_ss) {
        if (n > 12)
          tm.tm_hour = std::stoi(date.substr(11, 2));
        if (n > 15)
          tm.tm_min = std::stoi(date.substr(14, 2));
        if (n > 18)
          tm.tm_sec = std::stoi(date.substr(17, 2));
      }
    } catch (const std::invalid_argument& e) {
      logger << Logger::debug
             << "Error converting date: \""
             << date
             << "\": " << e.what() << Logger::endl;
    }
    // negative when no DST information available
    tm.tm_isdst = -1;
    std::lock_guard<std::mutex> lock(g_datetime_mutex);
    // std::cout << "year: " << tm.tm_year
    //           << ", month: " << tm.tm_mon
    //           << ", mday: " << tm.tm_mday
    //           << ", hours: " << tm.tm_hour
    //           << ", minutes: " << tm.tm_min
    //           << ", seconds: " << tm.tm_sec << '\n';
    datetime = std::mktime(&tm);
  } else {
    logger << Logger::debug
           << "Error converting date: \""
           << date
           << "\"" << Logger::endl;
    datetime = -1;
  }
}

DateTime::DateTime(std::tm &tm) : DateTime()
{
  // We treat all input times as though they are local, so strip off the time
  // zone before converting.
  // std::cout << "Input time is: " << std::put_time(&tm, "%FT%T%z") << '\n';
  std::ostringstream os;
  os << std::put_time(&tm, "%FT%T");
  // jstd::cout << "Assumed local time is: " << os.str() << '\n';
  // std::lock_guard<std::mutex> lock(g_datetime_mutex);
  // std::time_t t = std::mktime(&tm);
  // std::cout << "UTC:   " << std::put_time(std::gmtime(&t), "%FT%T%z") << '\n';
  // std::cout << "Local: " << std::put_time(std::localtime(&t), "%FT%T%z") << '\n';
  init(os.str());
}

DateTime::DateTime(const std::time_t t) : DateTime()
{
  init(t);
}

void DateTime::init(const std::time_t t)
{
  datetime = t;
}

DateTime::DateTime(const std::chrono::system_clock::time_point tp) {
  datetime = std::chrono::system_clock::to_time_t(tp);
}

DateTime::DateTime(int year, int month, int day,
                   int hour, int minute, int second)
{
  std::tm t{};
  t.tm_year = year - year_offset;
  t.tm_mon = month - month_offset;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
  // negative when no DST information available
  t.tm_isdst = -1;

  std::lock_guard<std::mutex> lock(g_datetime_mutex);
  datetime = std::mktime(&t);
}

std::tm DateTime::convert(std::time_t t) const
{
  std::lock_guard<std::mutex> lock(g_datetime_mutex);
  // /usr/share/zoneinfo/Etc/UTC
  // setenv("TZ", "/usr/share/zoneinfo/Europe/London", 1);
  std::tm r{};
  std::tm* tm = std::gmtime(&t);
  // std::tm* tm = std::localtime(&t);
  // std::cout << "converted " << t << " to year: " << (tm->tm_year + year_offset)
  //           << ", month: " << (tm->tm_mon + month_offset)
  //           << '\n';
  r.tm_year = tm->tm_year;
  r.tm_mon = tm->tm_mon;
  r.tm_mday = tm->tm_mday;
  r.tm_hour = tm->tm_hour;
  r.tm_min = tm->tm_min;
  r.tm_sec = tm->tm_sec;
  r.tm_isdst = tm->tm_isdst;
  std::mktime(&r);
  return r;
}

std::string DateTime::get_time_as_rfc7231()
{
  // https://httpwg.org/specs/rfc7231.html#http.date
  // Wed, 03 Nov 2021 14:47:53 GMT
  std::ostringstream ss;
  std::lock_guard<std::mutex> lock(g_datetime_mutex);
  ss << std::put_time(gmtime(&datetime), "%a, %d %b %Y %H:%M:%S GMT");
  return ss.str();
}

// This will let you change the thousands separator, but not remove it, except
// by increasing the grouping value beyond anything expected.
//
// However, it is often easier to imbue(std::locale("C")) which does not use a
// thousands separator.
//
// struct my_thousands_sep : std::numpunct<char> {
//   char do_thousands_sep() const { return ','; }    // Different thousands separator
//   std::string do_grouping() const { return "\3"; } // groups of 3 digits
// };

std::string DateTime::to_string(date_format format) const
{
  std::ostringstream s;
  switch (format) {
    case yyyy_mm_dd: {
      s << std::put_time(std::localtime(&datetime), "%F");
      break;
    }
    case yyyy_mm_dd_hh_mm_ss: {
      s << std::put_time(std::localtime(&datetime), "%FT%T");
      break;
    }
    case dd_mon_yyyy_hh_mm_ss: {
      s << std::put_time(std::localtime(&datetime), "%d %b %Y %T");
      break;
    }
    case yyyy_mm_dd_hh_mm_ss_z: {
      s << std::put_time(std::localtime(&datetime), "%F %T %Z");
      break;
    }
  }
  // // s.imbue(std::locale(s.getloc(), new no_thousands_sep));

  // std::tm tm = convert(datetime);
  // std::lock_guard<std::mutex> lock(g_datetime_mutex);
  // s << std::setw(4) << std::setfill('0')
  //   << std::to_string(tm.tm_year + year_offset) << '-'
  //   << std::setw(2) << (tm.tm_mon + month_offset) << '-'
  //   << std::setw(2) << tm.tm_mday;

  // if (format == yyyy_mm_dd_hh_mm_ss || format == no_tz_date_time) {
  //   s << 'T'
  //     << std::setw(2) << tm.tm_hour << ':'
  //     << std::setw(2) << tm.tm_min << ':'
  //     << std::setw(2) << tm.tm_sec;
  // }
  // if (format == yyyy_mm_dd_hh_mm_ss) {
  //   s << ".000+0000";
  // }
  // // if (tm.tm_isdst > 0)
  // //   s << " (DST)";
  return s.str();
}

/**
 * \param base_t the base date for the repeating period.  It can be today, the
 * past or the future.
 *
 * \param frequency - the frequency in days for the repeating period.
 *
 * \return the date that begins the current period, based on the passed base
 * date.
 */
std::time_t DateTime::period_start_date(std::time_t base_t, int frequency) const {
  const auto base_tp = std::chrono::system_clock::from_time_t(base_t);
  const auto freq_dur_hours = std::chrono::hours(frequency * 24);
  const auto this_tp = std::chrono::system_clock::from_time_t(datetime);
  auto diff = this_tp - base_tp;
  if (this_tp < base_tp)
    diff += std::chrono::hours(24);
  auto remainder = diff % freq_dur_hours;
  auto tp = this_tp - remainder;
  if (this_tp < base_tp)
    tp -= freq_dur_hours - std::chrono::hours(24);
  return std::chrono::system_clock::to_time_t(tp);
}

/**
 * \param base_t the base date for the repeating period.  It can be today, the
 * past or the future.
 *
 * \param frequency - the frequency in days for the repeating period.
 *
 * \return the next due date for a repeating period, based on the passed base
 * date.
 */
std::time_t DateTime::period_end_date(std::time_t base_t, int frequency) const {
  const auto base_tp = std::chrono::system_clock::from_time_t(base_t);
  const auto freq_dur_hours = std::chrono::hours(frequency * 24);
  const auto this_tp = std::chrono::system_clock::from_time_t(datetime);

  auto diff = this_tp - base_tp;
  if (this_tp < base_tp)
    diff += std::chrono::hours(24);
  const auto tp = this_tp - diff % freq_dur_hours +
    (this_tp < base_tp ?
     std::chrono::hours(0) :
     freq_dur_hours - std::chrono::hours(24));
  return std::chrono::system_clock::to_time_t(tp);
}

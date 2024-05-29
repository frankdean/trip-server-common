// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2024 Frank Dean <frank.dean@fdsd.co.uk>

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

// /// Regex to ensure the date part of a string looks valid
// const std::regex DateTime::valid_yyyy_mm_dd_regex =
//   std::regex("[0-9]{1,4}[^0-9]{1}[0-9]{2}[^0-9]{1}[0-9]{2}.*");

/// Regex for splitting RFC 822/1123 and RFC 850 formatted dates
const std::regex DateTime::rfc822_regex =
  std::regex("(\\D+), (\\d+)(?:-| )(\\D+)(?:-| )(\\d+) (\\d+):(\\d+):(\\d+) GMT");
/// Regex for splitting ANSIC C's asctime() formatted date
const std::regex DateTime::asctime_regex =
  std::regex("(?:\\D+) (\\D+)\\s+(\\d+) (\\d+):(\\d+):(\\d+) (\\d+)");

/// Regex for splitting ANSI C formatted date, required by RFC 1945 (HTTP/1.0)


/// Regex for splitting ISO 8601 Date and Time strings
/// e.g. 2022-10-31T12:36:09Z
const std::regex DateTime::iso8601_regex =
  std::regex("([0-9]{4})-?([0-9]{2})-?([0-9]{2})"
             "(?:[T\\s]([0-9]{2}):?([0-9]{2}):?([0-9]{2})?([\\.,][0-9]+)?"
             "(([-+\\s])([0-9]{2}):?([0-9]{2})?|Z)?)?");

// const std::string invalid_date_format = "Invalid date format";

/// Mutex used to lock access to non-threadsafe functions
std::mutex g_datetime_mutex;

DateTime::DateTime() : default_format(yyyy_mm_dd_hh_mm_ss)
{
  datetime = std::chrono::system_clock::now();
}

DateTime::DateTime(std::string date) : DateTime()
{
  init(date);
}

/**
 * \param date a date formated as an Internet date,
 * e.g. Mon, 31 Oct 2022 17:58:18 GMT
 *
 * \return the passed date in ISO8601 format (yyyy-mm-ddThh:mm:ss)
 */
std::string DateTime::convert_rfc822_to_iso8601(
    const std::smatch &m, std::string date)
{
  try {
    int year = std::stoi(m[4]);
    // See section 5.1, 'Fixed Solution' RFC 2626 https://www.ietf.org/rfc/rfc2626.txt
    if (year < 50) {
      year += 2000;
    } else if (year < 100) {
      year += 1900;
    }
    std::ostringstream os1;
    os1 << m[2] << ' ' << m[3] << ' ' << year << ' ' << m[5] << ':' << m[6] << ':' << m[7];
    std::istringstream is(os1.str());
    std::tm tm {};
    is >> std::get_time(&tm, "%d %b %Y %T");
    if (is.fail())
      logger << Logger::debug << "Parsing of date failed" << Logger::endl;
    std::ostringstream os;
    os << std::put_time(&tm, "%FT%T");
    date = os.str();
  } catch (const std::invalid_argument& e) {
    logger << Logger::debug
           << "Error converting date: \""
           << date
           << "\": " << e.what() << Logger::endl;
    datetime = std::chrono::system_clock::from_time_t(-1);
  }
  // The date should now be in the ISO 8601 format.
  return date;
}

/**
 * \param date a date formated as ANSI C's asctime() format
 * e.g. Mon Oct 17:58:18 2022
 *
 * \return the passed date in ISO8601 format (yyyy-mm-ddThh:mm:ss)
 */
std::string DateTime::convert_asctime_to_iso8601(
    const std::smatch &m, std::string date)
{
  try {
    std::ostringstream os1;
    os1 << m[2] << ' ' << m[1] << ' ' << m[6] << ' ' << m[3] << ':' << m[4] << ':' << m[5];
    std::istringstream is(os1.str());
    std::tm tm {};
    is >> std::get_time(&tm, "%d %b %Y %T");
    if (is.fail())
      logger << Logger::debug << "Parsing of date failed" << Logger::endl;
    std::ostringstream os;
    os << std::put_time(&tm, "%FT%T");
    date = os.str();
  } catch (const std::invalid_argument& e) {
    logger << Logger::debug
           << "Error converting date: \""
           << date
           << "\": " << e.what() << Logger::endl;
    datetime = std::chrono::system_clock::from_time_t(-1);
  }
  // The date should now be in the ISO 8601 format.
  return date;
}

/**
 * \param date a date formated as dd_mon_yyyy_hh_mm_ss
 *
 * \return the passed date in ISO8601 format (yyyy-mm-ddThh:mm:ss)
 */
std::string DateTime::convert_dd_mon_yyyy_hh_mm_ss_to_iso8601(
    const std::smatch &m, std::string date)
{
  try {
    std::istringstream is(date);
    std::tm tm {};
    is >> std::get_time(&tm, "%d %b %Y %T");
    if (is.fail())
      logger << Logger::debug << "Parsing of date failed" << Logger::endl;
    std::ostringstream os;
    os << std::put_time(&tm, "%FT%T");
    date = os.str();
  } catch (const std::invalid_argument& e) {
    logger << Logger::debug
           << "Error converting date: \""
           << date
           << "\": " << e.what() << Logger::endl;
    datetime = std::chrono::system_clock::from_time_t(-1);
  }
  // The date should now be in the ISO 8601 format.
  return date;
}

void DateTime::init(std::string date)
{
  // std::cout << "DateTime converting \"" << date << "\"\n";
  if (std::regex_match(date, numeric_regex)) {
    set_ms(std::stoll(date) * 1000);
    return;
  }
  // Use regular expressions to determine the date format, then convert it to an
  // intermediate ISO 8601 format, then finally to an internal date value.
  bool localtime = true;
  std::tm tm {};
  // The date should now be in the format similar to yyyy_mm_dd...
  std::smatch m;

  bool is_iso_8601 = std::regex_match(date, m, iso8601_regex);
  // If not ISO 8601, try first try converting as Internet format
  if (!is_iso_8601 && std::regex_match(date, m, rfc822_regex)) {
    date = convert_rfc822_to_iso8601(m, date);
    is_iso_8601 = std::regex_match(date, m, iso8601_regex);
  }
  // If not ISO 8601, then try converting as ANSI C's asctime() format
  if (!is_iso_8601 && std::regex_match(date, m, asctime_regex)) {
    date = convert_asctime_to_iso8601(m, date);
    is_iso_8601 = std::regex_match(date, m, iso8601_regex);
  }
  // If not ISO 8601, then try converting as dd_mon_yyyy_hh_mm_ss
  if (!is_iso_8601) {
    date = convert_dd_mon_yyyy_hh_mm_ss_to_iso8601(m, date);
    is_iso_8601 = std::regex_match(date, m, iso8601_regex);
  }
  // Convert ISO 8601 formatted date into internal value.
  if (is_iso_8601) {
    double fraction = 0;
    double zone_hours = 0;
    // try {
      tm.tm_year = std::stoi(m[1]) - year_offset;
      tm.tm_mon = std::stoi(m[2]) - month_offset;
      tm.tm_mday = std::stoi(m[3]);
      if (m[8] == "Z") {
        localtime = false;
      } else if (m[8] != "") {
        localtime = false;
        int sign = m[9] == "-" ? -1 : 1;
        if (m[10] != "")
          zone_hours = std::stoi(m[10]);
        if (m[11] != "") {
          double zone_minutes = std::stoi(m[11]);
          zone_hours += zone_minutes / 60;
        }
        zone_hours *= sign;
        // std::cout << "Zone hours: " << zone_hours << '\n';
      }
      if (m[4] != "")
        tm.tm_hour = std::stoi(m[4]);
      if (m[5] != "")
        tm.tm_min = std::stoi(m[5]);
      if (m[6] != "")
        tm.tm_sec = std::stoi(m[6]);
      if (m[7] != "")
        fraction = std::stod(m[7]);
      // std::cout << "Fraction: " << fraction << '\n';
    // } catch (const std::invalid_argument& e) {
    //   logger << Logger::debug
    //          << "Error converting date: \""
    //          << date
    //          << "\": " << e.what() << Logger::endl;
    // }
    std::lock_guard<std::mutex> lock(g_datetime_mutex);
    // std::cout << "year: " << tm.tm_year
    //           << ", month: " << tm.tm_mon
    //           << ", mday: " << tm.tm_mday
    //           << ", hours: " << tm.tm_hour
    //           << ", minutes: " << tm.tm_min
    //           << ", seconds: " << tm.tm_sec << '\n';
    // If we have input time zone information, use the timegm C function to
    // treat the input time as GMT/UTC

    std::time_t time_t;
    if (localtime) {
      tm.tm_isdst = -1;
      time_t = std::mktime(&tm);
    } else {
      time_t = ::timegm(&tm);
    }
    datetime = std::chrono::system_clock::from_time_t(time_t);
    // std::cout << "Time before adjustment " << to_string() << '\n';
    if (fraction > 0 || zone_hours != 0) {
      set_ms(get_ms() + fraction * 1000 - zone_hours * 3600000);
      // std::cout << "Time after adjustment " << to_string() << '\n';
    }
  } else {
    logger << Logger::debug
           << "Error converting date: \""
           << date
           << "\"" << Logger::endl;
    throw std::invalid_argument("Unable to parse date");
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

// DateTime::DateTime(const std::time_t t) : DateTime()
// {
//   init(t);
// }

// void DateTime::init(const std::time_t t)
// {
//   datetime = std::chrono::system_clock::from_time_t(t);
// }

void DateTime::set_time_t(std::time_t t)
{
  datetime = std::chrono::system_clock::from_time_t(t);
}

void DateTime::set_ms(int64_t ms)
{
  datetime = std::chrono::system_clock::time_point(
      std::chrono::milliseconds(ms));
}

DateTime::DateTime(const std::chrono::system_clock::time_point tp) {
  datetime = tp;
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
  datetime = std::chrono::system_clock::from_time_t(std::mktime(&t));
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

std::string DateTime::get_time_as_rfc7231() const
{
  // https://httpwg.org/specs/rfc7231.html#http.date
  // Wed, 03 Nov 2021 14:47:53 GMT
  std::ostringstream ss;
  std::lock_guard<std::mutex> lock(g_datetime_mutex);
  auto time_t = std::chrono::system_clock::to_time_t(datetime);
  ss << std::put_time(gmtime(&time_t), "%a, %d %b %Y %H:%M:%S GMT");
  return ss.str();
}

std::string DateTime::get_time_as_iso8601_gmt() const
{
  std::ostringstream ss;
  const int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      datetime.time_since_epoch()).count();
  const int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(
      datetime.time_since_epoch()).count();
  const double r = round((static_cast<double>(ms) / 1000 - seconds) * 1000);
  std::lock_guard<std::mutex> lock(g_datetime_mutex);
  auto time_t = std::chrono::system_clock::to_time_t(datetime);
  ss << std::put_time(gmtime(&time_t), "%FT%T");
  ss << "." << std::fixed << std::setprecision(0) << std::setw(3)
     << std::setfill('0') << r << 'Z';
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
  auto time_t = std::chrono::system_clock::to_time_t(datetime);
  switch (format) {
    case yyyy_mm_dd: {
      s << std::put_time(std::localtime(&time_t), "%F");
      break;
    }
    case yyyy_mm_dd_hh_mm_ss: {
      s << std::put_time(std::localtime(&time_t), "%FT%T");
      break;
    }
    case dd_mon_yyyy_hh_mm_ss: {
      s << std::put_time(std::localtime(&time_t), "%d %b %Y %T");
      break;
    }
    case yyyy_mm_dd_hh_mm_ss_z: {
      s << std::put_time(std::localtime(&time_t), "%F %T %Z");
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
 * \param base_tp the base date for the repeating period.  It can be today, the
 * past or the future.
 *
 * \param frequency - the frequency in days for the repeating period.
 *
 * \return the date that begins the current period, based on the passed base
 * date.
 */
std::chrono::system_clock::time_point DateTime::period_start_date(
    std::chrono::system_clock::time_point base_tp, int frequency) const {
  const auto freq_dur_hours = std::chrono::hours(frequency * 24);
  const auto this_tp = datetime;
  auto diff = this_tp - base_tp;
  if (this_tp < base_tp)
    diff += std::chrono::hours(24);
  auto remainder = diff % freq_dur_hours;
  auto tp = this_tp - remainder;
  if (this_tp < base_tp)
    tp -= freq_dur_hours - std::chrono::hours(24);
  return tp;
}

/**
 * \param base_tp the base date for the repeating period.  It can be today, the
 * past or the future.
 *
 * \param frequency - the frequency in days for the repeating period.
 *
 * \return the next due date for a repeating period, based on the passed base
 * date.
 */
std::chrono::system_clock::time_point DateTime::period_end_date(
    std::chrono::system_clock::time_point base_tp, int frequency) const {
  const auto freq_dur_hours = std::chrono::hours(frequency * 24);
  const auto this_tp = datetime;

  auto diff = this_tp - base_tp;
  if (this_tp < base_tp)
    diff += std::chrono::hours(24);
  const auto tp = this_tp - diff % freq_dur_hours +
    (this_tp < base_tp ?
     std::chrono::hours(0) :
     freq_dur_hours - std::chrono::hours(24));
  return tp;
}

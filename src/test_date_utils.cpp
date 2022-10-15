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
#include <iostream>
#include <regex>
#include <sstream>
#include "date_utils.cpp"

using namespace fdsd::utils;

bool test_construct_string_yyyy_mm_dd_hh_mm_ss()
{
  const std::string test_date = "2022-05-16 21:54:34";
  DateTime tm(test_date);
  auto date = tm.get_time_as_tm();
  // std::cout << "put_time: " << std::put_time(&date, "%F %T %Z") << '\n';
  std::ostringstream os;
  os << tm;
  // std::cout << "Date yyyy_mm_dd_hh_mm_ss: \"" << tm << "\"\n";
  const std::regex expected("2022-05-16T21:54:34(\\.000)?(\\+\\d{4})?");
  const bool retval = std::regex_match(os.str(), expected);
  // std::cout << "Converted to: \"" << tm << "\"\n";
  if (!retval)
    std::cout << "test_construct_string_yyyy_mm_dd_hh_mm_ss() failed: \""
              << os.str() << "\"\n";
  return retval;
}

bool test_construct_string_yyyy_mm_dd_hh_mm_ss_fraction_north_hemi_summer()
{
  const long expected_ms = 1655811709685; // Tuesday, 21 June 2022 11:41:49.685 GMT
  const std::string test_date = "2022-06-21 12:41:49.685+01";
  DateTime tm(test_date);
  // DateTime treats all input times as local times, ignoring any time zone
  // specifiers.  So, ignoring hours and minutes in the result, as these will
  // change depending on time zone
  const std::regex expected("2022-06-21T..:..:49.685(Z|\\+\\d{4})?");
  const bool retval = std::regex_match(tm.get_time_as_iso8601_gmt(), expected);
  // const bool retval = expected_ms == tm.get_ms();
  // std::cout << "Diff " << tm.get_ms() - expected_ms << '\n';
  if (!retval)
    std::cout
      << "test_construct_string_yyyy_mm_dd_hh_mm_ss_fraction_north_hemi_summer()"
      " failed, expected " << expected_ms << " but was "
      << tm.get_ms() << " \"" << tm << "\", ISO 8601: \""
      << tm.get_time_as_iso8601_gmt() << "\"\n"
      << "rfc7231: \"" << tm.get_time_as_rfc7231() << '\n';
  return retval;
}

bool test_construct_string_yyyy_mm_dd_hh_mm_ss_fraction_north_hemi_winter()
{
  const long expected_ms = 1665315662685; // Sunday, 9 October 2022 11:41:02.685
  const std::string test_date = "2022-10-09 11:41:02.685+01";
  DateTime tm(test_date);
  // DateTime treats all input times as local times, ignoring any time zone
  // specifiers.  So, ignoring hours and minutes in the result, as these will
  // change depending on time zone
  const std::regex expected("2022-10-09T..:..:02.685(Z|\\+\\d{4})?");
  const bool retval = std::regex_match(tm.get_time_as_iso8601_gmt(), expected);
  // const bool retval = expected_ms == tm.get_ms();
  // std::cout << "Diff " << tm.get_ms() - expected_ms << '\n';
  if (!retval)
    std::cout
      << "test_construct_string_yyyy_mm_dd_hh_mm_ss_fraction_north_hemi_winter()"
      " failed, expected " << expected_ms << " but was "
      << tm.get_ms() << " \"" << tm << "\", ISO 8601: \""
      << tm.get_time_as_iso8601_gmt() << "\"\n";
  return retval;
}

bool test_construct_yyyymmdd_bst()
{
  DateTime tm(2022, 3, 20);
  std::ostringstream os;
  os << tm;
  // std::cout << "Date BST: \"" << tm << "\"\n";
  const bool retval = std::regex_match(
      os.str(),
      std::regex("2022-03-20T00:00:00(\\.000)?(\\+\\d{4})?"));
  if (!retval)
    std::cout << "test_construct_yyyymmdd_bst() failed\n";
  return retval;
}

bool test_construct_yyyymmdd_dst()
{
  DateTime tm(2022, 7, 30);
  std::ostringstream os;
  os << tm;
  // std::cout << "Date DST: " << tm << '\n';
  const bool result = std::regex_match(
      os.str(),
      std::regex("2022-07-30T00:00:00(\\.000)?(\\+\\d{4})?"));
  if (!result)
    std::cout << "test_construct_yyyymmdd_dst() FAILED: \"" << os.str() << "\"\n";
  return result;
}

bool test_as_dd_mon_yyyy_hh_mm_ss()
{
  std::tm t = {};
  const std::string test_date = "2022-05-16 21:54:34";
  std::istringstream is(test_date);
  // is.imbue(std::locale("en_GB.utf-8"));
  is >> std::get_time(&t, "%Y-%m-%d %T");
  if (is.fail()) {
    std::cout << "test_as_dd_mon_yyyy_hh_mm_ss() Parsing of date failed\n";
    return false;
  }
  DateTime tm(t);
  std::string result = tm.to_string(DateTime::dd_mon_yyyy_hh_mm_ss);
  const bool retval = "16 May 2022 21:54:34" == result;
  if (!retval)
    std::cout << "test_as_dd_mon_yyyy_hh_mm_ss() failed: \""
              << result << "\"\n";
  return retval;
}

bool test_as_dd_mon_yyyy_hh_mm_ss_input()
{
  const std::string test_date = "16 May 2022 21:54:34";
  DateTime tm(test_date);
  const std::string result = tm.to_string();
  const std::string expected = "2022-05-16T21:54:34";
  const bool retval = expected  == result;
  if (!retval)
    std::cout << "test_as_dd_mon_yyyy_hh_mm_ss_input() failed.  "
      "Expected \""
              << expected << "\" but was \"" << result << "\"\n";
  return retval;
}

bool test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date()
{
  std::tm t = {};
  const std::string test_date = "2020-11-30 14:14:42";
  std::istringstream is(test_date);
  // is.imbue(std::locale("en_GB.utf-8"));
  is >> std::get_time(&t, "%Y-%m-%d %T");
  if (is.fail()) {
    std::cout << "test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date() Parsing of date failed\n";
    return false;
  }
  DateTime tm(t);
  std::string result = tm.to_string(DateTime::dd_mon_yyyy_hh_mm_ss);
  const bool retval = "30 Nov 2020 14:14:42" == result;
  if (!retval)
    std::cout << "test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date() failed: \""
              << result << "\"\n";
  return retval;
}

bool test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date_02()
{
  std::tm t = {};
  const std::string test_date = "2020-11-30 14:14:42";
  DateTime tm(test_date);
  std::string result = tm.to_string(DateTime::dd_mon_yyyy_hh_mm_ss);
  const bool retval = "30 Nov 2020 14:14:42" == result;
  if (!retval)
    std::cout << "test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date_02() failed: \""
              << result << "\"\n";
  return retval;
}

bool test_time_as_rfc7231()
{
  const std::string test_date = "2022-05-16T21:54:34.000+01";
  const std::string expected = "Mon, 16 May 2022 20:54:34 GMT";
  const std::time_t expected_epoch = 1652734474; // Unix epoch timestamp
  DateTime tm(test_date);
  const std::time_t result_epoch = tm.time_t();
  const auto diff = expected_epoch - result_epoch;
  if (diff != 0)
    std::cout << "Diff: " << diff << '\n';
  std::string result = tm.get_time_as_rfc7231();
  const bool retval = expected == result;
  if (!retval)
    std::cout << "test_time_as_rfc7231() failed, expected \""
              << expected << "\" but was \"" << result << "\"\n";
  return retval;
}

bool test_construct_string_yyyy_mm_dd()
{
  try {
    const std::string date_str("2022-08-05");
    DateTime date(date_str);
    const std::string s = date.to_string();
    const std::string result = s.substr(0, 10);
    const bool retval = result == date_str;
    if (!retval)
      std::cout<< "test_construct_string_yyyy_mm_dd()\n"
               << "expected: \""
               << date_str << "\" result: \""
               << result << "\"\n";
    return retval;
  } catch (const std::exception &e) {
    std::cout << "test_construct_string_yyyy_mm_dd() failed with exception: "
              << e.what() << '\n';
    throw;
  }
}

bool test_yyyy_mm_dd_hh_mm_ss_constructor()
{
  const std::string expected = "2022-05-16 21:54:34";
  DateTime date(2022, 5, 16, 21, 54, 34);
  const bool retval = date.to_string() == expected;
  if (!retval)
    std::cout << "test_yyyy_mm_dd_hh_mm_ss_constructor() failed\n"
              << "Expected: \"" << expected << "\" result: \""
              << date << "\"\n";
  return retval;
}

// Test to ensure it doesn't cause an abort()
bool test_bad_input()
{
  const std::string test_date = "16 XYZ 2022 21:54:34";
  DateTime tm(test_date);
  const std::string result = tm.to_string();
  const bool retval = std::regex_match(
      result,
      std::regex("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}"));
  if (!retval)
    std::cout << "test_bad_input() failed.\n";
  return retval;
}

bool test_date_as_time_t_string()
{
  const std::string expected = "2022-05-16T21:54:34";
  DateTime date(expected);
  std::time_t t = date.time_t();
  std::string s = std::to_string(t);
  DateTime test_date(s);
  const std::string result = test_date.to_string();
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_date_as_time_t_string() failed.  "
              << "Expected \"" << expected
              << "\" but was \"" << result << "\"\n";
  return retval;
}

bool test_date_as_time_t_string_negative_date()
{
  DateTime test_date("-172800");
  const std::string result = test_date.to_string();
  const bool retval = std::regex_match(
      result,
      std::regex("1969-12-30T[0-9]{2}:[0-9]{2}:00"));
  if (!retval)
    std::cout << "test_date_as_time_t_string_negative_date() failed: \""
              << result << "\"\n";
  return retval;
}

bool test_date_as_time_t_string_invalid_date_01()
{
  // const std::string expected = "1970-01-01T00:59:59";
  DateTime test_date("123-485");
  const std::string result = test_date.to_string();
  const bool retval = std::regex_match(
      result,
      std::regex("[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}"));
  if (!retval)
    std::cout << "test_date_as_time_t_string_invalid_date_01() failed: \""
              << result << "\"\n";
  return retval;
}

bool test_date_as_time_t_string_invalid_date_02()
{
  // const std::string expected = "1970-01-01T00:59:59";
  DateTime test_date("123.485.889");
  const std::string result = test_date.to_string();
  const bool retval = std::regex_match(
      result,
      std::regex("[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}"));
  if (!retval)
    std::cout << "test_date_as_time_t_string_invalid_date_02() failed: \""
              << result << "\"\n";
  return retval;
}

bool test_date_as_time_t_string_floating_point_date()
{
  const std::string expected = "2022-08-31T15:46:44.000Z";
  DateTime test_date("1661960804.567");
  const std::string result = test_date.get_time_as_iso8601_gmt();
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_date_as_time_t_string_floating_point_date() failed.  "
              << "Expected \"" << expected
              << "\" but was \"" << result << "\"\n";
  return retval;
}

bool test_get_ms()
{
  const long test_ms = 1665315662685; // Sunday, 9 October 2022 11:41:02.685
  const std::chrono::milliseconds ms(test_ms);
  const std::chrono::time_point<std::chrono::system_clock> tp(ms);
  const DateTime dt(tp);
  const bool retval = dt.get_ms() == test_ms;
  if (!retval)
    std::cout << "test_get_ms() failed, expected " << test_ms
              << " but was " << dt.get_ms() << '\n';
  return retval;
}

bool test_get_time_as_iso8601_gmt_01()
{
  const long test_ms = 1665315662685; // Sunday, 9 October 2022 11:41:02.685
  const DateTime dt{
      std::chrono::time_point<std::chrono::system_clock>(
          std::chrono::milliseconds(test_ms))};
  const std::string result = dt.get_time_as_iso8601_gmt();
  const std::string expected = "2022-10-09T11:41:02.685Z";
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_get_time_as_iso8601_gmt_01() failed, expected \""
              << expected << "\" but was: \"" << result << "\"\n";
  return retval;
}

bool test_get_time_as_iso8601_gmt_02()
{
  const long test_ms = 1665315662000; // Sunday, 9 October 2022 11:41:02.000
  const DateTime dt{
      std::chrono::time_point<std::chrono::system_clock>(
          std::chrono::milliseconds(test_ms))};
  const std::string result = dt.get_time_as_iso8601_gmt();
  const std::string expected = "2022-10-09T11:41:02.000Z";
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_get_time_as_iso8601_gmt_02() failed, expected \""
              << expected << "\" but was: \"" << result << "\"\n";
  return retval;
}

bool test_get_time_as_iso8601_gmt_03()
{
  const long test_ms = 1665315662005; // Sunday, 9 October 2022 11:41:02.005
  const DateTime dt{
      std::chrono::time_point<std::chrono::system_clock>(
          std::chrono::milliseconds(test_ms))};
  const std::string result = dt.get_time_as_iso8601_gmt();
  const std::string expected = "2022-10-09T11:41:02.005Z";
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_get_time_as_iso8601_gmt_03() failed, expected \""
              << expected << "\" but was: \"" << result << "\"\n";
  return retval;
}

bool test_get_time_as_iso8601_gmt_04()
{
  const long test_ms = 1665315662100; // Sunday, 9 October 2022 11:41:02.100
  const DateTime dt{
      std::chrono::time_point<std::chrono::system_clock>(
          std::chrono::milliseconds(test_ms))};
  const std::string result = dt.get_time_as_iso8601_gmt();
  const std::string expected = "2022-10-09T11:41:02.100Z";
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_get_time_as_iso8601_gmt_04() failed, expected \""
              << expected << "\" but was: \"" << result << "\"\n";
  return retval;
}


bool assert_match(
    const std::vector<std::smatch> &matches,
    int test,
    int element,
    std::string expected)
{
  const bool retval = matches[test][element] == expected;
  if (!retval)
    std::cout << "test_iso_regex() test number " << test
              << " failed for element "
              << element << ".  Expected \"" << expected
              << "\" but was \"" << matches[test][element] << "\"\n";
  return retval;
}

bool test_iso_regex()
{
  bool retval = true;
  std::smatch m;
  std::vector<std::string> targets = {
    { "2022-06-21 12:41:49.685+01" },
    { "2022-06-21T12:41:49.685+01" },
    { "2022-06-21T12:41:49.685+0130" },
    { "2022-06-21T12:41:49.685+01:30" },
    { "2022-06-21T12:41:49.685Z" },
    { "2022-06-21T12:41:49+01:30" },
    { "2022-06-21T12:41:49.685" },
    { "2022-06-21" },
    { "2016-10-09T00:00" },
    { "2016-10-10T00:00:00 0100" },
    { "2020-12-01T12:40:00-0530" }
  };
  std::vector<std::smatch> matches;
  int n = 0;
  for (const auto &target : targets) {
    const bool success = std::regex_match(
        target,
        m,
        DateTime::iso8601_regex);
    matches.push_back(m);
    if (!success) {
      std::cout << "test_iso_regex() test " << n << " failed to parse regex\n";
      retval = false;
    // } else {
    //   std::cout << "\nTest " << n << '\n';
    //   for (int i = 0; i < m.size(); i++) {
    //     std::cout << i << ": \"" << m[i] << "\"\n";
    //   }
    }
    n++;
  }
  // if (retval) retval = matches[0][10] == "01";
  if (retval) retval = assert_match(matches, 0, 7, ".685");
  if (retval) retval = assert_match(matches, 0, 9, "+");
  if (retval) retval = assert_match(matches, 0, 10, "01");
  if (retval) retval = assert_match(matches, 1, 9, "+");
  if (retval) retval = assert_match(matches, 1, 10, "01");
  if (retval) retval = assert_match(matches, 2, 9, "+");
  if (retval) retval = assert_match(matches, 2, 10, "01");
  if (retval) retval = assert_match(matches, 2, 11, "30");
  if (retval) retval = assert_match(matches, 3, 9, "+");
  if (retval) retval = assert_match(matches, 3, 10, "01");
  if (retval) retval = assert_match(matches, 3, 11, "30");
  if (retval) retval = assert_match(matches, 4, 8, "Z");
  if (retval) retval = assert_match(matches, 5, 7, "");
  if (retval) retval = assert_match(matches, 6, 8, "");
  if (retval) retval = assert_match(matches, 7, 4, "");
  if (retval) retval = assert_match(matches, 8, 1, "2016");
  if (retval) retval = assert_match(matches, 8, 2, "10");
  if (retval) retval = assert_match(matches, 8, 3, "09");
  if (retval) retval = assert_match(matches, 8, 4, "00");
  if (retval) retval = assert_match(matches, 8, 5, "00");
  if (retval) retval = assert_match(matches, 8, 6, "");
  if (retval) retval = assert_match(matches, 9, 9, " ");
  if (retval) retval = assert_match(matches, 10, 8, "-0530");
  return retval;
}

// Test to debug a specific failure
bool test_iso_regex_02()
{
  bool retval = true;
  std::smatch m;
  std::string target = "2016-10-10T00:00";
  // std::string target = "2020-12-01T12:40:00-0530";
  const bool success = std::regex_match(
      target,
      m,
      DateTime::iso8601_regex);
  if (!success) {
    std::cout << "test_iso_regex_02() failed to parse regex\n";
    retval = false;
  // } else {
  //   for (int i = 0; i < m.size(); i++) {
  //     std::cout << i << ": \"" << m[i] << "\"\n";
  //   }
  }
  return retval;
}

bool test_positive_time_zone_same_day()
{
  std::smatch m;
  DateTime time("2020-12-01T12:40:00+0530");
  const std::string result = time.get_time_as_iso8601_gmt();
  const std::string expected = "2020-12-01T07:10:00.000Z";
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_positive_time_zone_same_day() failed.  Expected \""
              << expected << "\", but was \"" << result << "\"\n";
  return retval;
}

bool test_negative_time_zone_same_day()
{
  std::smatch m;
  DateTime time("2020-12-01T12:40:00-0530");
  const std::string result = time.get_time_as_iso8601_gmt();
  const std::string expected = "2020-12-01T18:10:00.000Z";
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_negative_time_zone_same_day() failed.  Expected \""
              << expected << "\", but was \"" << result << "\"\n";
  return retval;
}

bool test_time_zone_previous_day()
{
  std::smatch m;
  DateTime time("2020-12-01T02:40:00+0530");
  const std::string result = time.get_time_as_iso8601_gmt();
  const std::string expected = "2020-11-30T21:10:00.000Z";
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_time_zone_previous_day() failed.  Expected \""
              << expected << "\", but was \"" << result << "\"\n";
  return retval;
}

bool test_time_zone_next_day()
{
  std::smatch m;
  DateTime time("2020-02-29T22:40:00-0530");
  // std::cout << time << '\n';
  const std::string result = time.get_time_as_iso8601_gmt();
  const std::string expected = "2020-03-01T04:10:00.000Z";
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_time_zone_next_day() failed.  Expected \""
              << expected << "\", but was \"" << result << "\"\n";
  return retval;
}

int main(void)
{
  try {
    return !(
        test_construct_string_yyyy_mm_dd_hh_mm_ss()
        && test_construct_string_yyyy_mm_dd_hh_mm_ss_fraction_north_hemi_summer()
        && test_construct_string_yyyy_mm_dd_hh_mm_ss_fraction_north_hemi_winter()
        && test_construct_yyyymmdd_bst()
        && test_construct_yyyymmdd_dst()
        && test_time_as_rfc7231()
        && test_construct_string_yyyy_mm_dd()
        && test_as_dd_mon_yyyy_hh_mm_ss()
        && test_as_dd_mon_yyyy_hh_mm_ss_input()
        && test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date()
        && test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date_02()
        && test_bad_input()
        && test_date_as_time_t_string()
        && test_date_as_time_t_string_negative_date()
        && test_date_as_time_t_string_invalid_date_01()
        && test_date_as_time_t_string_invalid_date_02()
        && test_date_as_time_t_string_floating_point_date()
        && test_get_ms()
        && test_get_time_as_iso8601_gmt_01()
        && test_get_time_as_iso8601_gmt_02()
        && test_get_time_as_iso8601_gmt_03()
        && test_get_time_as_iso8601_gmt_04()
        && test_iso_regex()
        && test_iso_regex_02()
        && test_positive_time_zone_same_day()
        && test_negative_time_zone_same_day()
        && test_time_zone_previous_day()
        && test_time_zone_next_day()
      );
  } catch (const std::exception &e) {
    std::cout << "test_date_utils failed with exception: "
              << e.what() << '\n';
    return EXIT_FAILURE;
  }
}

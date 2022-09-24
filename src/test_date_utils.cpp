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
  DateTime tm(test_date, DateTime::yyyy_mm_dd_hh_mm_ss);
  auto date = tm.get_time_as_tm();
  // std::cout << "put_time: " << std::put_time(&date, "%F %T %Z") << '\n';
  std::ostringstream os;
  os << tm;
  // std::cout << "Date yyyy_mm_dd_hh_mm_ss: \"" << tm << "\"\n";
  const std::regex expected("2022-05-16T21:54:34(\\.000)?(\\+\\d{4})?");
  const bool retval = std::regex_match(os.str(), expected);
  if (!retval)
    std::cout << "test_construct_string_yyyy_mm_dd_hh_mm_ss() failed: \""
              << os.str() << "\"\n";
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
  DateTime tm(test_date, DateTime::dd_mon_yyyy_hh_mm_ss);
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
  const std::string test_date = "2020-10-09 14:14:42";
  std::istringstream is(test_date);
  // is.imbue(std::locale("en_GB.utf-8"));
  is >> std::get_time(&t, "%Y-%m-%d %T");
  if (is.fail()) {
    std::cout << "test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date() Parsing of date failed\n";
    return false;
  }
  DateTime tm(t);
  std::string result = tm.to_string(DateTime::dd_mon_yyyy_hh_mm_ss);
  const bool retval = "09 Oct 2020 14:14:42" == result;
  if (!retval)
    std::cout << "test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date() failed: \""
              << result << "\"\n";
  return retval;
}

bool test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date_02()
{
  std::tm t = {};
  const std::string test_date = "2020-10-09 14:14:42";
  DateTime tm(test_date, DateTime::yyyy_mm_dd_hh_mm_ss);
  std::string result = tm.to_string(DateTime::dd_mon_yyyy_hh_mm_ss);
  const bool retval = "09 Oct 2020 14:14:42" == result;
  if (!retval)
    std::cout << "test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date_02() failed: \""
              << result << "\"\n";
  return retval;
}

bool test_time_as_rfc7231()
{
  const std::string test_date = "2022-05-16 21:54:34";
  DateTime tm(test_date, DateTime::yyyy_mm_dd_hh_mm_ss);
  std::string result = tm.get_time_as_rfc7231();
  return "Mon, 16 May 2022 20:54:34 GMT" == result;
}

bool test_construct_string_yyyy_mm_dd()
{
  const std::string date_str("2022-08-05");
  DateTime date(date_str, DateTime::yyyy_mm_dd);
  const std::string s = date.to_string();
  const std::string result = s.substr(0, 10);
  const bool retval = result == date_str;
  if (!retval)
    std::cout<< "test_construct_string_yyyy_mm_dd()\n"
             << "expected: \""
             << date_str << "\" result: \""
             << result << "\"\n";
  return retval;
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

bool test_date_with_time_zone()
{
  const std::string expected = "2022-05-16T21:54:34";
  const std::string test_date = expected + ".000+0300";
  DateTime date(test_date);
  const std::string result = date.to_string();
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_date_with_time_zone() failed.  "
              << "Expected \"" << expected
              << "\" but was \"" << result << "\"\n";
  return retval;
}

// Test to ensure it doesn't cause an abort()
bool test_bad_input()
{
  const std::string test_date = "16 XYZ 2022 21:54:34";
  DateTime tm(test_date, DateTime::dd_mon_yyyy_hh_mm_ss);
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
      std::regex("1969-12-30T0[01]{1}:00:00"));
  if (!retval)
    std::cout << "test_date_as_time_t_string_negative_date() failed\n";
  return retval;
}

bool test_date_as_time_t_string_invalid_date_01()
{
  const std::string expected = "1970-01-01T00:59:59";
  DateTime test_date("123-485");
  const std::string result = test_date.to_string();
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_date_as_time_t_string_invalid_date_01() failed.  "
              << "Expected \"" << expected
              << "\" but was \"" << result << "\"\n";
  return retval;
}

bool test_date_as_time_t_string_invalid_date_02()
{
  const std::string expected = "1970-01-01T00:59:59";
  DateTime test_date("123.485.889");
  const std::string result = test_date.to_string();
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_date_as_time_t_string_invalid_date_02() failed.  "
              << "Expected \"" << expected
              << "\" but was \"" << result << "\"\n";
  return retval;
}

bool test_date_as_time_t_string_floating_point_date()
{
  const std::string expected = "2022-08-31T16:46:44";
  DateTime test_date("1661960804.567");
  const std::string result = test_date.to_string();
  const bool retval = result == expected;
  if (!retval)
    std::cout << "test_date_as_time_t_string_floating_point_date() failed.  "
              << "Expected \"" << expected
              << "\" but was \"" << result << "\"\n";
  return retval;
}

int main(void)
{
  return !(
      test_construct_string_yyyy_mm_dd_hh_mm_ss()
      && test_construct_yyyymmdd_bst()
      && test_construct_yyyymmdd_dst()
      && test_time_as_rfc7231()
      && test_construct_string_yyyy_mm_dd()
      && test_as_dd_mon_yyyy_hh_mm_ss()
      && test_as_dd_mon_yyyy_hh_mm_ss_input()
      && test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date()
      && test_as_dd_mon_yyyy_hh_mm_ss_north_hemi_winter_date_02()
      && test_date_with_time_zone()
      && test_bad_input()
      && test_date_as_time_t_string()
      && test_date_as_time_t_string_negative_date()
      && test_date_as_time_t_string_invalid_date_01()
      && test_date_as_time_t_string_invalid_date_02()
      && test_date_as_time_t_string_floating_point_date()
    );
}

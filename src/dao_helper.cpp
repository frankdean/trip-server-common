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
#include "../config.h"
#include "dao_helper.hpp"
#include "date_utils.hpp"
#include <iomanip>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <sstream>

using namespace fdsd::utils;

/// Returns true if the map contains the passed key
bool dao_helper::contains(const std::map<std::string, std::string> &params,
                          std::string key) const
{
  auto f = params.find(key);
  if (f != params.end())
    return true;
  return false;
}

/// Returns the value associated with the passed key, or the default value if
/// it does not exist.
std::string dao_helper::get_value(
    const std::map<std::string, std::string> &params,
    std::string key, std::string default_value) const
{
  auto f = params.find(key);
  if (f != params.end())
    return f->second;
  return default_value;
}

int dao_helper::get_int(const std::map<std::string, std::string> &params,
                            std::string key, int default_value) const
{
  auto f = params.find(key);
  if (f != params.end()) {
    try {
      return std::stoi(f->second);
    } catch (const std::logic_error& e) {
      return default_value;
    }
  }
  return default_value;
}

long dao_helper::get_long(const std::map<std::string, std::string> &params,
                          std::string key, long default_value) const
{
  auto f = params.find(key);
  if (f != params.end()) {
    try {
      return std::stoi(f->second);
    } catch (const std::logic_error& e) {
      return default_value;
    }
  }
  return default_value;
}

dao_helper::result_order dao_helper::get_result_order(
    const std::map<std::string, std::string> &params,
    std::string key, dao_helper::result_order default_value) const
{
  auto f = params.find(key);
  if (f == params.end() || f->second != "DESC")
    return ascending;
  return descending;
}

std::time_t dao_helper::get_date(
    const std::map<std::string, std::string> &params,
    std::string key) const
{
  try {
    auto f = params.at(key);
    try {
      DateTime time(f);
      // std::cout << "dao_helper converted: \"" << f << "\" to \"" << time << "\"\n";
      return time.time_t();
    } catch (const std::logic_error& e) {
      std::cerr << "Erorr converting parameter with key: \""
                << key << "\" to date from \"" << f << "\"\n"
                << e.what() << '\n';
    }
  } catch (const std::out_of_range& e) {
    std::cerr << "Failed to find parameter with key: \""
              << key << "\"\n" << e.what() << '\n';
  }
  return std::time_t(nullptr);
}

std::string dao_helper::date_as_html_input_value(std::time_t time) const
{
  std::ostringstream os;
  os.imbue(std::locale("C"));
  os << std::put_time(std::localtime(&time), "%FT%T");
  return os.str();
}

std::time_t dao_helper::convert_libpq_date(std::string date)
{
  // std::istringstream is(date);
  // std::tm tm{};
  // is.imbue(std::locale("C"));
  // // Passed time format is "2017-01-15 17:04:58.000+00"
  // is >> std::get_time(&tm, "%Y-%m-%d %T    %z");
  // if (is.fail()) {
  //   std::cerr << "Error converting \"" << date << "\"\n";
  // }
  // std::cout << "Converted \"" << date << "\" to \""
  //           << std::put_time(&tm, "%FT%T%z") << "\"\n";
  // return std::mktime(&tm);
  DateTime retval(date);
  return retval.time_t();
}

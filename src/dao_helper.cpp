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
#include <chrono>
#include <iomanip>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <sstream>

using namespace fdsd::utils;

/// Returns true if the map contains the passed key
bool dao_helper::contains(const std::map<std::string, std::string> &params,
                          std::string key)
{
  auto f = params.find(key);
  if (f != params.end())
    return true;
  return false;
}

/// \return the value associated with the passed key, or the default value if
/// it does not exist.
std::string dao_helper::get_value(
    const std::map<std::string, std::string> &params,
    std::string key, std::string default_value)
{
  auto f = params.find(key);
  if (f != params.end())
    return f->second;
  return default_value;
}

/// \return a std::pair with the first value set to true if the key exists in
/// the passed map exists.  The second pair contains the value.
std::pair<bool, std::string> dao_helper::get_optional_value(
    const std::map<std::string, std::string> &params,
    std::string key)
{
  try {
    return std::make_pair(true, params.at(key));
  } catch (const std::out_of_range &e) {
    return std::make_pair(false, "");
  }
}

/**
 * \return a std::pair with the first value set to true if the key exists in
 * the passed map exists.  The second pair contains the value.
 *
 * \throws std::invalid_argument if the value cannot be converted to float.
 */
std::pair<bool, int> dao_helper::get_optional_int_value(
    const std::map<std::string, std::string> &params,
    std::string key)
{
  try {
    std::string s = params.at(key);
    return std::make_pair(true, std::stoi(s));
  } catch (const std::out_of_range &e) {
    return std::make_pair(false, 0);
  }
}

/**
 * \return a std::pair with the first value set to true if the key exists in
 * the passed map exists.  The second pair contains the value.
 *
 * \throws std::invalid_argument if the value cannot be converted to float.
 */
std::pair<bool, float> dao_helper::get_optional_float_value(
    const std::map<std::string, std::string> &params,
    std::string key)
{
  try {
    std::string s = params.at(key);
    return std::make_pair(true, std::stof(s));
  } catch (const std::out_of_range &e) {
    return std::make_pair(false, 0);
  }
}

/**
 * \return a std::pair with the first value set to true if the key exists in
 * the passed map exists.  The second pair contains the value.
 *
 * \throws std::invalid_argument if the value cannot be converted to float.
 */
std::pair<bool, double> dao_helper::get_optional_double_value(
    const std::map<std::string, std::string> &params,
    std::string key)
{
  try {
    std::string s = params.at(key);
    return std::make_pair(true, std::stod(s));
  } catch (const std::out_of_range &e) {
    return std::make_pair(false, 0);
  }
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
  DateTime retval(date);
  return retval.time_t();
}

/**
 * Converts a date from the format used by libpqxx.
 * \return the time since epoch in milliseconds
 */
std::chrono::system_clock::time_point
    dao_helper::convert_libpq_date_tz(std::string date)
{
  DateTime retval(date);
  return retval.time_tp();
}

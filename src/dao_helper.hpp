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
#ifndef DAO_HELPER_HPP
#define DAO_HELPER_HPP

#include <chrono>
#include <ctime>
#include <map>
#include <string>

namespace fdsd {
namespace utils {

struct dao_helper {
  enum result_order { descending, ascending };
  static bool contains(const std::map<std::string, std::string> &params,
                std::string key);
  static std::string get_value(const std::map<std::string, std::string> &params,
                               std::string key, std::string default_value = "");
  static std::pair<bool, int> get_optional_int_value(
    const std::map<std::string, std::string> &params,
    std::string key);
  static std::pair<bool, std::string> get_optional_value(
      const std::map<std::string, std::string> &params,
      std::string key);
  static std::pair<bool, float> get_optional_float_value(
      const std::map<std::string, std::string> &params,
      std::string key);
  static std::pair<bool, double> get_optional_double_value(
    const std::map<std::string, std::string> &params,
    std::string key);
  int get_int(const std::map<std::string, std::string> &params,
                  std::string key, int default_value = -1) const;
  long get_long(const std::map<std::string, std::string> &params,
                std::string key, long default_value = -1) const;
  dao_helper::result_order get_result_order(
      const std::map<std::string, std::string> &params,
      std::string key, dao_helper::result_order default_value = ascending) const;
  std::time_t get_date(
      const std::map<std::string, std::string> &params,
      std::string key) const;
  std::string date_as_html_input_value(std::time_t time) const;
  static std::time_t convert_libpq_date(std::string date);
  static std::chrono::system_clock::time_point
      convert_libpq_date_tz(std::string date);
};

} // namespace utils
} // namespace fdsd

#endif // DAO_HELPER_HPP

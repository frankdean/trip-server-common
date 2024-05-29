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
#include "configuration.hpp"
#include <fstream>
#include <iostream>
#ifdef __GNUG__
#include <cstring>
#include <cerrno>
#endif

using namespace fdsd::utils;

const std::string Configuration::pg_uri_key = "pg_uri";
const std::string Configuration::pg_pool_size_key = "pg_pool_size";
const std::string Configuration::worker_count_key = "worker_count";

Configuration::Configuration(std::string filename)
  : Configuration()
{
  std::ifstream cin(filename);
  // Throw exceptions containing specified flag(s)
  cin.exceptions(std::ifstream::badbit);
  if (!cin) {
    throw Configuration::FileNotFoundException(filename);
  }
  std::string s;
  int i = 0;
  while (std::getline(cin, s)) {
    // Ignore lines starting with '#'
    if (s.empty() || s[0] == '#')
      continue;
    auto i = s.find_first_of(' ');
    if (i != std::string::npos) {
      std::string key;
      std::string value;
      key = s.substr(0, i);
      value = s.substr(i + 1);
      config[key] = value;
    } else {
      std::cerr << "The configuration file contains an invalid entry: \""
                << s << "\"\n";
    }
  } // while
}

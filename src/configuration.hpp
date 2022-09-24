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
#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP
#include <exception>
#include <string>
#include <map>

namespace fdsd
{
namespace utils
{

class Configuration {

private:
  std::map<std::string, std::string> config;
public:

  class FileNotFoundException : public std::exception {
  private:
    std::string message;
  public:
    FileNotFoundException(std::string filename) {
      this->message = "configuration file not found: \"" + filename + '\"';
    }
    virtual const char* what() const throw() override {
      return message.c_str();
    }
  };

  static const std::string pg_uri_key;
  static const std::string pg_pool_size_key;
  static const std::string worker_count_key;
  Configuration() : config() {}
  Configuration(std::string filename);
  std::string get(std::string key, std::string default_value = "") const {
    auto f = config.find(key);
    if (f == config.end())
      return default_value;
    return f->second;
  }
};

} // namespace utils
} // namespace fdsd

#endif // CONFIGURATION_HPP

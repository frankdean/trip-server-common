// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2025 Frank Dean <frank.dean@fdsd.co.uk>

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU Affero General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
    for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef FDSD_GET_OPTIONS_HPP
#define FDSD_GET_OPTIONS_HPP

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <ostream>
#include <string>

namespace fdsd {
namespace utils {

struct GetOptions
{
#ifdef HAVE_GETOPT_H
  static const char short_opts[];
  static struct option long_options[];
#endif // HAVE_GETOPT_H
  std::string program_name;
  std::string listen_address;
  std::string port;
#ifdef ENABLE_STATIC_FILES
  std::string doc_root;
#endif
  class UnexpectedArgumentException : public std::exception {
  public:
    virtual const char* what() const throw() override {
      return "Unexepected argument";
    }
  };
  GetOptions() : program_name(),
                 listen_address("0.0.0.0"),
                 port("8080")
#ifdef ENABLE_STATIC_FILES
               ,doc_root(".")
#endif
    {}
  GetOptions(std::string listen_address,
             std::string port,
             std::string doc_root) : program_name(),
                                     listen_address(listen_address),
                                     port(port)
#ifdef ENABLE_STATIC_FILES
                                   ,doc_root(doc_root)
#endif
    {}
  virtual ~GetOptions() {}
  bool init(int argc, char* argv[]);
  std::string config_filename;
  virtual void show_version_info() const;
  virtual void usage(std::ostream& os) const;
#ifdef HAVE_GETOPT_H
  /// Indicates whether information output to the terminal should be verbose
  static int verbose_flag;
  static int debug_flag;
  virtual bool handle_option(int c);
  virtual const struct option* get_long_options() const;
  virtual const char* get_short_options() const;
#endif // HAVE_GETOPT_H
};

} // namespace utils
} // namespace fdsd

#endif // FDSD_GET_OPTIONS_HPP

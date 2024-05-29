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
#include "../config.h"
#include "get_options.hpp"
#include <iostream>

using namespace fdsd::utils;

#ifdef HAVE_GETOPT_H

#ifdef ALLOW_STATIC_FILES
const char GetOptions::short_opts[] = "hs:p:r:c:vV";
#else
const char GetOptions::short_opts[] = "hs:p:c:vV";
#endif

int GetOptions::verbose_flag = 0;

struct option GetOptions::long_options[] = {
  // Name                 Argument           Flag              Shortname
  {"help",                no_argument,       NULL,             'h'},
  {"listen",              required_argument, NULL,             's'},
  {"port",                required_argument, NULL,             'p'},
#ifdef ALLOW_STATIC_FILES
  {"root",                required_argument, NULL,             'r'},
#endif
  {"config-file",         required_argument, NULL,             'c'},
  {"verbose",             no_argument,       &GetOptions::verbose_flag,    1},
  {"version",             no_argument,       NULL,             'v'},
  {NULL, 0, NULL, 0}
};

/**
 * \return true if the application should continue, false if the application
 * should exit.
 */
bool GetOptions::handle_option(int c)
{
  // std::cout << "GetOptions::handle option: " << (char) c << '\n';
  switch (c) {
    case 0:
      break;
    case 's':
      listen_address = optarg;
      break;
    case 'p':
      port = optarg;
      break;
#ifdef ALLOW_STATIC_FILES
    case 'r':
      doc_root = optarg;
      break;
#endif
    case 'c':
      config_filename = optarg;
      break;
    case 'h':
      usage(std::cout);
      return false;
    case 'v':
      show_version_info();
      return false;
    case 'V':
      GetOptions::verbose_flag = 1;
      break;
    case '?':
      // getopt_long already printed an error message
      break;
    default:
      std::cerr
        << "Error An apparently valid option of '" << (char) c
        << "' does not have a handler.\n";
      abort();
  } // switch
  return true;
}
#endif // HAVE_GETOPT_H

void GetOptions::show_version_info() const
{
  std::cout << PACKAGE << " " << VERSION << '\n';
}

/**
 * \return true if the application should continue, false if the application
 * should exit.
 */
bool GetOptions::init(int argc, char* argv[])
{
  program_name = argv[0];
#ifdef HAVE_GETOPT_H
  int c;

  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, get_short_options(),
                    get_long_options(), &option_index);
    // Detect end of the options
    if (c == -1) break;

    if (!handle_option(c))
      return false;
  } // while

#else // not using getopt
  const int expected_args =
#ifdef ALLOW_STATIC_FILES
    4
#else
    3
#endif
    ;
  if (argc != expected_args) {
    usage(std::cerr);
    throw UnexpectedArgumentException();
  } else {
    listen_address = argv[1];
    port = argv[2];
#ifdef ALLOW_STATIC_FILES
    doc_root = argv[3];
#endif
  }
#endif // HAVE_GETOPT_H
  return true;
}

void GetOptions::usage(std::ostream& os) const
{
#ifdef HAVE_GETOPT_H
  os
    << "Usage:\n"
    << "  " << program_name << " [OPTIONS]\n\n"
    << "Options:\n"
    << "  -h, --help\t\t\t\tshow this help, then exit\n"
    << "  -v, --version\t\t\t\tshow version information, then exit\n"
    << "  -s, --listen=ADDRESS\t\t\tlisten address, e.g. 0.0.0.0\n"
    << "  -p, --port=PORT\t\t\tport number, e.g. 8080\n"
#ifdef ALLOW_STATIC_FILES
    << "  -r, --root=DIRECTORY\t\t\tdocument root directory\n"
#endif
    << "  -c, --config-filename=FILENAME\tconfiguration file name\n"
    << "  -V, --verbose\t\t\t\tverbose output\n";
#else
  os
    << "Usage: " << program_name << " <address> <port>"
#ifdef ALLOW_STATIC_FILES
    << " <doc_root>"
#endif
    << '\n'
    << "Example:\n"
    << "    " << program_name << " 0.0.0.0 8080"
#ifdef ALLOW_STATIC_FILES
    << " ."
#endif
    << '\n';
#endif // HAVE_GETOPT_H
}

#ifdef HAVE_GETOPT_H
const char* GetOptions::get_short_options() const
{
  return short_opts;
}

const struct option* GetOptions::get_long_options() const
{
  return long_options;
}
#endif // HAVE_GETOPT_H

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
#include "debug_utils.hpp"

#include <iomanip>

using namespace fdsd::utils;

void DebugUtils::write_line(std::basic_string<char>::const_iterator& line,
                            const std::string& s, std::ostream& os)
{
  int n = 16;
  os << "  ";
  do {
    char ch = *line;
    if (ch >= ' ' ) {
      os << ch;
    } else {
      os << '.';
    }
    line++;
    if (--n == 0) {
      break;
    }
  } while (line != s.cend());
}

void DebugUtils::hex_dump(const std::string& s, std::ostream& os)
{
  int count = 0;
  // Pointer to the beginning of the current line
  std::basic_string<char>::const_iterator line = s.cbegin();
  for (auto i = s.cbegin(); i != s.cend(); i++) {
    if (count % 16 == 0) {
      // os << '\n' << std::hex << std::setw(8) << std::setfill('0') << count << ": ";
      os << std::hex << std::setw(8) << std::setfill('0') << count << ": ";
    } else if (count % 2 == 0) {
      os  << ' ';
    }
    char ch = *i;
    os << std::hex << std::setw(2) << std::setfill('0') << (int) ch;
    if (count % 16 == 15) {
      write_line(line, s, os);
      if (i + 1 != s.cend())
        os << '\n';
    }
    count++;
  }
  int r = 16 - count % 16;
  if (r < 16) {
    while (r-- > 0) {
      if (count % 2 == 0)
        os << ' ';
      os << "  ";
      count++;
    }
    write_line(line, s, os);
  }
  os << std::dec << '\n';
}

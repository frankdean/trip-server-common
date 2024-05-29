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
#ifndef DEBUG_UTILS_HPP
#define DEBUG_UTILS_HPP

#include <ostream>

namespace fdsd
{
namespace utils
{

class DebugUtils {
private:
  static void write_line(std::basic_string<char>::const_iterator& line,
                         const std::string& s, std::ostream& os);
public:
  static void hex_dump(const std::string& s, std::ostream& os);
};

} // namespace utils
} // namespace fdsd

#endif // DEBUG_UTILS_HPP

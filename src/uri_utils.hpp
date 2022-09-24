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
#ifndef URI_UTILS_HPP
#define URI_UTILS_HPP

#include <map>
#include <string>
#include <vector>

namespace fdsd
{
namespace web
{

class UriUtils {
public:
  static const std::string gen_delims;
  static const std::string sub_delims;
  static const std::string unsafe_characters;
  static const std::string reserved_characters;
  static const std::string unreserved_characters;
  static std::string uri_decode(std::string s, bool is_form_url_encoded = true);
  static std::string uri_encode(std::string s, bool is_form_url_encoded = true);
  static std::string uri_encode_rfc_1738(std::string s);
  static std::pair<std::string, std::string> split_pair(std::string source, std::string with);
  static std::vector<std::string> split_params(std::string qs, std::string with);
  static std::map<std::string, std::string> get_query_params(const std::string uri);
};

} // namespace web
} // namespace fdsd

#endif // URI_UTILS_HPP

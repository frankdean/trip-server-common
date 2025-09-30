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
#include "uri_utils.hpp"
#include <cctype>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

using namespace fdsd::web;

const std::string UriUtils::gen_delims =
  ":/?#[]@";

const std::string UriUtils::sub_delims =
  "!$&'()*+,;=";

// From RFC 1738
const std::string UriUtils::unsafe_characters = " <>\"#%{}|\\^~[]`";

// From RFC 1630
// const std::string UriUtils::safe_chars = "$-_@.&+-";
// const std::string UriUtils::extra_chars = "!*\""'(|),";
// const std::string UriUtils::reserved_chars = "=;/#?: ";

const std::string UriUtils::reserved_characters =
  gen_delims + sub_delims;

const std::string UriUtils::unreserved_characters =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789-._~";

/**
 * Decodes URI encoded strings.
 *
 * https://en.wikipedia.org/wiki/Percent-encoding
 *
 * \param s the URI encoded string to be decoded.
 *
 * \param is_form_url_encoded if true, plus signs, `+` in the string will be
 * replaced with spaces.
 */
std::string UriUtils::uri_decode(std::string s, bool is_form_url_encoded)
{
  std::string retval;
  std::locale loc("C");
  for (auto i = s.begin(); i != s.end(); i++) {
    char c = *i;
    // std::cout << "c: " << *i << '\n';
    if (c == '%') {
      auto hold = i;
      char a = *(++i);
      if (i == s.end()) {
        // std::cerr << "Warning: URI is invalid (01)" << std::endl;
        retval += c;
        i = hold;
      } else {
        char b = *(++i);
        if (i == s.end()) {
          // std::cerr << "Warning: URI is invalid (02)" << std::endl;
          retval += c;
          i = hold;
        } else {
          if (std::isxdigit(a, loc) && std::isxdigit(b, loc)) {
            std::string n;
            n += a;
            n += b;
            retval += char(std::stoul(n, nullptr, 16));
          } else {
            // std::cerr << "Warning: invalid percent encoded value" << std::endl;
            retval += c;
            i = hold;
          }
        }
      }
    } else if (is_form_url_encoded && *i == '+') {
      retval += ' ';
    } else {
      retval += *i;
    }
  } // for
  return retval;
}


/**
 * Encodes URI encoded strings.
 *
 * https://en.wikipedia.org/wiki/Percent-encoding
 *
 * \param s the string to be encoded.
 *
 * \param is_form_url_encoded if true, spaces in the string will be replaced
 * with plus signs, `+`.
 */
std::string UriUtils::uri_encode(std::string s, bool is_form_url_encoded)
{
  std::ostringstream retval;
  for (auto i = s.begin(); i != s.end(); i++) {
    char c = *i;
    int v = c;
    if (c == ' ') {
      retval << (is_form_url_encoded ? "+" : "%20");
    } else if (unreserved_characters.find(c) == std::string::npos) {
      retval << "%" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << v;
    } else {
      retval << c;
    }
  } // for
  return retval.str();
}

/**
 * Intended to encode URI strings according to RFC 1738.  The rules have not
 * been properly evaluated at this time and the implementation of this method is
 * likely to change.
 *
 * - https://datatracker.ietf.org/doc/html/rfc1630
 * - https://datatracker.ietf.org/doc/html/rfc1738
 *
 * \param s the string to be encoded.
 *
 */
std::string UriUtils::uri_encode_rfc_1738(std::string s)
{
  std::ostringstream retval;
  for (auto i = s.begin(); i != s.end(); i++) {
    char c = *i;
    int v = c;
    if (c == ' ') {
      retval << "%20";
    } else if (v <= 0x1F || v >= 0x7F || unsafe_characters.find(c) != std::string::npos) {
      retval << "%" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << v;
    } else {
      retval << c;
    }
  } // for
  return retval.str();
}

/**
 * Splits a string into two based on the specified separating string.
 *
 * \param source the string to split.
 * \param with the string that separates the pair.
 * \return a std::pair of strings containing the split strings.
 */
std::pair<std::string, std::string> UriUtils::split_pair(std::string source, std::string with)
{

  std::string::size_type pos = source.find_first_of(with);
  if (pos == std::string::npos) {
    return std::make_pair(source, "");
  }
  std::string p1 = source.substr(0, pos);
  std::string p2 = std::string(source, pos + 1);
  return std::make_pair(p1, p2);
}

/**
 * Splits a string into a vector of strings based on a specified separating
 * string.
 *
 * \param qs the string to split.
 * \param with the string that separates the pair.
 * \return a vector of the split strings.
 */
std::vector<std::string> UriUtils::split_params(std::string qs, std::string with)
{
  std::vector<std::string> retval;
  std::string::size_type pos = 0;
  while (pos != std::string::npos) {
    pos = qs.find_first_of(with);
    if (pos != std::string::npos) {
      retval.push_back(qs.substr(0, pos));
      qs = std::string(qs, pos + 1);
    }
  }
  if (!qs.empty())
    retval.push_back(qs);
  return retval;
}

/**
 * Extracts query parameters from a URI.
 *
 * RFC 3986
 *  3.4.  Query
 *
 *     The query component contains non-hierarchical data that, along with
 *     data in the path component (Section 3.3), serves to identify a
 *     resource within the scope of the URI's scheme and naming authority
 *     (if any).  The query component is indicated by the first question
 *     mark ("?") character and terminated by a number sign ("#") character
 *     or by the end of the URI.
 *
 * \param uri the URI to extract the parameters from.
 * \return a std::map<std::string, std::string> as key-value pairs.  If the
 * URI does not contain a '?' character an empty string is returned.
 */
std::map<std::string, std::string> UriUtils::get_query_params(const std::string uri) {
  std::map<std::string, std::string> retval;
  std::string s;
  const std::string::size_type pos = uri.find_first_of('?');
  if (pos != std::string::npos) {
    const std::string::size_type fpos = uri.find_first_of('#');
    const std::string::size_type end =
      (fpos == std::string::npos) ? uri.length() - pos : fpos - pos - 1;
    s = uri.substr(pos + 1, end);
    // std::cout << "Found ? at " << pos << " with end at " << end << '\n';
  } else {
    return retval;
  }
  // std::cout << "Remainder: \"" << s << "\"\n";
  std::vector<std::string> query_params = split_params(s, "&");
  for (std::string qp : query_params) {
    auto nv = split_pair(qp, "=");
    retval[UriUtils::uri_decode(nv.first, false)] = UriUtils::uri_decode(nv.second, false);
  }
  return retval;
}

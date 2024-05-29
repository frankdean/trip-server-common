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
#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace fdsd
{
namespace web
{

bool istr_compare_predicate(unsigned char a, unsigned char b);
bool istr_compare(std::string const& s1, std::string const& s2);

enum class HTTPMethod {
  get,
  post,
  head,
  put,
  delete_it,
  options,
  patch,
  unknown
};

// struct my_moneypunct : std::moneypunct_byname<char, false> {
//   my_moneypunct(const char* name) : moneypunct_byname(name) {}
//   int do_frac_digits() const { return 3; }
//   char_type do_thousands_sep() const { return ','; }
//   char_type do_decimal_point() const { return '.'; }
//   pattern do_pos_format() const { return { {sign, symbol, none, value} } ;}
//   pattern do_neg_format() const { return { {sign, symbol, none, value} } ;}
// };

template <class char_type,
          class iter_type = std::ostreambuf_iterator<char_type>>
class my_money_put : public std::money_put<char_type> {
private:
  std::string name;
protected:
  virtual iter_type
      do_put(iter_type out, bool intl, std::ios_base& str, char_type fill,
             long double units) const override {
    std::ostringstream ss;
    ss.imbue(std::locale(name));
    // ss.setf(str.showbase | str.adjustfield | str.floatfield | str.showbase | str.showpoint | str.showpos | str.skipws);
    if (str.flags() & std::ios_base::showbase) {
      ss << std::showbase;
    }
    ss << std::put_money(units);
    std::string s = ss.str();
    // The currency symbol may be a multi-byte unicode character, so search using other characters
    auto decimal_point = s.find_first_of(".,");
    if (decimal_point != std::string::npos) {
      auto first_numeral = s.find_first_of("0123456789");
      if (first_numeral != std::string::npos) {
        if (decimal_point < first_numeral)
          s.insert(decimal_point, 1, '0');
      }
    }
    std::copy(s.begin(), s.end(), std::ostreambuf_iterator<char_type>(out));
    return out;
  }
public:
  my_money_put(std::string local_name) : std::money_put<char_type>(), name(local_name) {}
  virtual ~my_money_put() {}
};

class PayloadTooLarge : std::exception {
public:
  PayloadTooLarge() : std::exception() {}
};

typedef std::map<std::string, std::string> param_map;

class HTTPServerRequest {
public:
  struct multipart_type {
    std::map<std::string, std::string> headers;
    std::string body;
  };
  enum ContentType {
    x_www_form_urlencoded,
    multipart_form_data,
    application_json,
    unknown
  };

private:
  enum multipart_state_type {
    ready,
    header,
    body
  };
  /// Used when parsing a multipart form
  multipart_state_type multipart_state;
  /// The key marking the multipart form parts
  std::string boundary_key;
  /// Holds the current multipart form part during parsing
  multipart_type current_part;
  bool _keep_alive;
  param_map query_params;
  param_map post_params;
  void handle_multipart_form_data(const std::string &s);
  void handle_x_www_form_urlencoded(const std::string &s);
public:
  HTTPServerRequest();
  HTTPServerRequest(const std::string &http_request);
  static std::map<std::string, HTTPMethod> request_methods;
  HTTPMethod method;
  ContentType content_type;
  /// Multipart form elements, keyed by name
  std::map<std::string, multipart_type> multiparts;
  void handle_content_line(const std::string &s);
  std::string method_to_str() const;
  param_map get_post_params() const {
    return post_params;
  }
  const std::string get_post_param(std::string name) const;
  std::optional<long> get_optional_post_param_long(std::string name) const;
  std::optional<double>
      get_optional_post_param_double(std::string name) const;
  std::optional<std::string>
      get_optional_post_param(std::string name, bool trim = true) const;
  std::map<long, std::string>
      extract_array_param_map(std::string array_name) const;
  std::string get_header(const std::string &name) const {
    return iget_map_entry(name, headers);
  }
  static std::string iget_map_entry(
      const std::string &name,
      const std::map<std::string, std::string> &map);
  long get_content_length();
  std::string user_id;
  // The body content of the request, excluding headers etc.
  std::string content;
  std::string uri;
  std::string protocol;
  param_map headers;
  param_map get_query_params() const {
    return query_params;
  }
  void set_query_params(param_map params) {
    query_params = params;
  }
  const std::string get_query_param(std::string name) const;
  const std::string get_param(std::string name) const;
  std::string get_cookie(std::string cookie_name) const;
};

} // namespace web
} // namespace fdsd

#endif // HTTP_REQUEST_HPP

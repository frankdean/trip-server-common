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
#include "http_request.hpp"
#include "http_response.hpp"
#include "uri_utils.hpp"
#include <cassert>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <locale>
#include <map>
#include <sstream>

using namespace fdsd::web;

bool fdsd::web::istr_compare_predicate(unsigned char a, unsigned char b)
{
  return std::tolower(a) == std::tolower(b);
}

bool fdsd::web::istr_compare(std::string const& s1, std::string const& s2)
{
  if (s1.length() == s2.length()) {
    return std::equal(s1.begin(), s1.end(),
                      s2.begin(), istr_compare_predicate);
  }
  return false;
}

HTTPServerRequest::HTTPServerRequest(std::string http_request)
  : method(HTTPMethod::unknown),
    user_id(),
    content(),
    uri("/"),
    protocol(),
    headers(),
    query_params(),
    _keep_alive(false)
{
  // std::cout << ">>>>\n"
  //           << http_request
  //           << "<<<<"
  //           << std::endl;
  // std::cout << "Current content before: \"" << content << "\"\n";
  std::istringstream in(http_request);
  // std::stringstream b;
  std::string s;
  int count = 0;
  // False once we have encountered the blank line denoting the end of headers
  bool is_header = true;
  while (!in.eof()) {
    try {
      std::getline(in, s);
      // std::cout << "Line size: " << s.size() << '\n';
      if (s.size() > 0) {
        if (s.size() > 0 && (int)s.back() == 13) s.pop_back();
        // std::cout << "Last character is " << s.back() << std::endl;
        count++;
        // std::cout << "> " << count << " \"" << s << "\"\n";
        if (is_header && s.empty()) {
          is_header = false;
        } else if (is_header) {
          std::string::size_type p;
          if (count == 1) {
            p = s.find(' ');
            if (p != std::string::npos) {
              auto search = request_methods.find(s.substr(0, p));
              if (search != request_methods.end()) {
                method = search->second;
              }
              // std::cout << "Method type is: \"" << method_to_str() << "\"\n";
              s.erase(0, p+1);
              // std::cout << "> After erase: " << count << " \"" << s << "\"\n";
              p = s.find(' ');
              if (p != std::string::npos) {
                uri = s.substr(0, p);
                s.erase(0, p+1);
              }
              // std::cout << "HTTP_REQUEST Path: \"" << uri << "\"\n";
              query_params = UriUtils::get_query_params(uri);
              // std::cout << "Query parameters:\n";
              // for (auto qp = query_params.begin(); qp != query_params.end(); ++qp) {
              //   std::cout << qp->first << '=' << qp->second << '\n';
              // }
              protocol = s;
              // std::cout << "Protocol: \"" << protocol << "\"\n";
            } else {
              std::cerr << "Warning: invalid request at line " << count << '\n';
            }
          } else {
            p = s.find(": ");
            if (p != std::string::npos) {
              std::string key = s.substr(0, p);
              std::string value = p < s.size() ? s.substr(p+2) : "";
              // std::cout << "Header: \"" << key << "\" -> \"" << value << "\"\n";
              headers[key] = value;
            } else {
              std::cerr << "Warning: invalid header at line " << count << '\n';
            }
          }
        } else {
          // body
          // std::cout << "\nAdding \"" << s << "\" to content\n";
          content += s;
        }
      }
    } catch (std::ios_base::failure& e) {
      std::cerr << "Error parsing HTTP request: " << e.what() << '\n';
      // GCC 8.3.0-6 on Debian 10 throws an error on the last line
#ifndef __GNUG__
      std::cerr << "ERROR: " << e.what() << '\n';
#endif
    }
  } // while
  // content = b.str();
  // std::cout << "Body: " << content << "\n\nEnd\n";
}

std::map<std::string, HTTPMethod> HTTPServerRequest::request_methods = {
  { "DELETE", HTTPMethod::delete_it },
  { "GET", HTTPMethod::get },
  { "HEAD", HTTPMethod::head },
  { "OPTIONS", HTTPMethod::options },
  { "PATCH", HTTPMethod::patch },
  { "POST", HTTPMethod::post },
  { "PUT", HTTPMethod::put },
};

std::string HTTPServerRequest::method_to_str() const
{
  switch (method) {
    case HTTPMethod::delete_it:
      return "DELETE";
    case HTTPMethod::get:
      return "GET";
    case HTTPMethod::head:
      return "HEAD";
    case HTTPMethod::options:
      return "OPTIONS";
    case HTTPMethod::patch:
      return "PATCH";
    case HTTPMethod::post:
      return "POST";
    case HTTPMethod::put:
      return "PUT";
    case HTTPMethod::unknown:
      return "Unknown";
  }
  return "Unknown";
}

std::map<std::string, std::string> HTTPServerRequest::get_post_params() const {
  std::map<std::string, std::string> retval;
  std::vector<std::string> query_params = UriUtils::split_params(content, "&");
  UriUtils u;
  for (std::string qp : query_params) {
    auto nv = UriUtils::split_pair(qp, "=");
    retval[u.uri_decode(nv.first)] = u.uri_decode(nv.second);
  }
  return retval;
}

std::string HTTPServerRequest::get_cookie(const std::string cookie_name) const {
  // std::cout << "\nSearching for Cookie named: \"" << cookie_name << "\"\nAll headers:\n\n";
  // for (const auto& header : headers) {
  //   std::cout << '"'<< header.first << "\" -> \"" << header.second << "\"\n";
  // }
  auto f = headers.find("Cookie");
  if (f == headers.end()) {
    // std::cout << "Failed to find any cookies\n";
    return "";
  }
  std::vector<std::string> cookies = UriUtils::split_params(f->second, ";");
  for (const std::string cp : cookies) {
    auto nv = UriUtils::split_pair(cp, "=");
    if (nv.first.front() == ' ') {
      nv.first.erase(0, 1);
    }
    // std::cout << "Cookie: \"" << nv.first << "\" -> \"" << nv.second << "\"\n";
    // retval[uri_decode(nv.first)] = uri_decode(nv.second);
    if (nv.first == cookie_name)
      return nv.second;
  }
  return "";
}

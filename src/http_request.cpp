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
#include "http_request.hpp"
// #include "debug_utils.hpp"
#include "http_response.hpp"
#include "uri_utils.hpp"
#include "dao_helper.hpp"
#include <cassert>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <locale>
#include <map>
// #include <regex>
#include <sstream>

using namespace fdsd::web;
using namespace fdsd::utils;

bool fdsd::web::istr_compare(std::string const& s1, std::string const& s2)
{
  if (s1.length() == s2.length()) {
    return std::equal(s1.begin(), s1.end(),
                      s2.begin(), [] (unsigned char a, unsigned char b) {
                        return std::tolower(a) == std::tolower(b);
                      });

  }
  return false;
}

HTTPServerRequest::HTTPServerRequest()
  :
    multipart_state(multipart_state_type::ready),
    boundary_key(),
    current_part(),
    _keep_alive(false),
    post_params(),
    method(HTTPMethod::unknown),
    content_type(ContentType::unknown),
    user_id(),
    content(),
    uri("/"),
    protocol(),
    headers(),
    query_params()
{
}

HTTPServerRequest::HTTPServerRequest(const std::string &http_request)
  :
    multipart_state(multipart_state_type::ready),
    boundary_key(),
    current_part(),
    _keep_alive(false),
    post_params(),
    method(HTTPMethod::unknown),
    content_type(ContentType::unknown),
    user_id(),
    content(),
    uri("/"),
    protocol(),
    headers(),
    query_params()
{
  // std::cout << ">>>>\n"
  //           << http_request
  //           << "<<<<"
  //           << std::endl;
  // DebugUtils::hex_dump(http_request, std::clog);
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
          handle_content_line(s);
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

const std::string HTTPServerRequest::get_post_param(std::string name) const {
  auto search = post_params.find(name);
  if (search != post_params.end()) {
    return search->second;
  }
  return "";
}

std::optional<long>
    HTTPServerRequest::get_optional_post_param_long(std::string name) const {
  auto search = post_params.find(name);
  if (search != post_params.end() && !search->second.empty()) {
    try {
      return std::stol(search->second);
    } catch (const std::invalid_argument &e) {
      std::cerr << e.what() << " converting post param \""
                << name << "\" -> \"" << search->second << "\"\n";
      throw;
    }
  }
  return std::optional<long>();
}
std::optional<double>
    HTTPServerRequest::get_optional_post_param_double(std::string name) const {
  auto search = post_params.find(name);
  if (search != post_params.end() && !search->second.empty()) {
    try {
      return std::stod(search->second);
    } catch (const std::invalid_argument &e) {
      std::cerr << e.what() << " converting post param \""
                << name << "\" -> \"" << search->second << "\"\n";
      throw;
    }
  }
  return std::optional<double>();
}

std::optional<std::string>
    HTTPServerRequest::get_optional_post_param(std::string name, bool trim) const {
  auto search = post_params.find(name);
  if (search != post_params.end() && !search->second.empty()) {
    if (!trim)
      return search->second;
    std::string s = search->second;
    dao_helper::trim(s);
    if (!s.empty())
      return s;
  }
  return std::optional<std::string>();
}

const std::string HTTPServerRequest::get_query_param(std::string name) const
{
  auto search = query_params.find(name);
  if (search != query_params.end()) {
    return search->second;
  }
  return "";
}

/// Returns the given parameter from either the query or post parameters,
/// prioritizing POST parameters over GET parameters.
const std::string HTTPServerRequest::get_param(std::string name) const
{
  std::string retval;
  retval = get_post_param(name);
  if (retval.empty())
    retval = get_query_param(name);
  return retval;
}

void HTTPServerRequest::handle_multipart_form_data(const std::string &s)
{
  // std::cout << "Figuring out what to do with \"" << s << "\"\n";
  const bool is_boundary = (s == "--" + boundary_key);
  // if (is_boundary)
  //   std::cout << "Is a boundary string\n";
  switch (multipart_state) {
    case ready:
      // std::cout << "Starting header\n";
      multipart_state = header;
      break;
    case header:
      // std::cout << "Handling as a header line\n";
      if (s.empty()) {
        // std::cout << "Changed state to body\n";
        multipart_state = body;
      } else {
        const auto n = s.find(": ");
        if (n != std::string::npos) {
          const std::string key = s.substr(0, n);
          std::string value = s.size() > n+2 ? s.substr(n+2) : "";
          // std::cout << "local header: \"" << key << "\" -> \"" << value << "\"\n";
          current_part.headers[key] = value;
        // } else {
        //   std::cout << "Failed to find header key in header line: \"" << s << "\"\n";
        }
      }
      break;
    case body:
      const bool is_terminating_boundary = (s == "--" + boundary_key + "--");
      // if (is_terminating_boundary)
      //   std::cout << "Is a terminating boundary string\n";
      if (is_boundary || is_terminating_boundary) {
        const std::string content_disposition =
          iget_map_entry("Content-Disposition", current_part.headers);
        std::string name;
        if (!content_disposition.empty()) {
          std::vector<std::string> elements = UriUtils::split_params(content_disposition, ";");
          for (auto &ele : elements) {
            if (ele.front() == ' ')
              ele.erase(0, 1);
            // std::cout << ">:\"" << ele << "\"\n";
            const auto p = UriUtils::split_pair(ele, "=");
            if (p.first == "name") {
              name = p.second;
              if (name.front() == '"')
                name.erase(name.begin());
              if (name.back() == '"')
                name.erase(name.end() -1);
            }
          }
        }
        if (!name.empty()) {
          // std::cout << "\nSearch for header: \"Content-Type\"\n";
          const std::string type = iget_map_entry("Content-Type", current_part.headers);
          // std::cout << "MULTIPART CONTENT TYPE: \"" << type << "\" BODY: \"" << current_part.body << "\"\n";
          if (type.empty()) {
            post_params[name] = current_part.body;
          } else {
            // Handle as a standard post parameter
            multiparts[name] = current_part;
          }
        } else {
          std::cerr << "Warning: could not find a name for the disposition content\n";
        }
        current_part.body.clear();
        current_part.headers.clear();
        multipart_state = header;
      } else {
        // std::cout << "Appending to body\n";
        current_part.body.append(s);
      }
      break;
  }
}

void HTTPServerRequest::handle_x_www_form_urlencoded(const std::string &s)
{
  const std::vector<std::string> params = UriUtils::split_params(s, "&");
  UriUtils u;
  for (const std::string &p : params) {
    const auto nv = UriUtils::split_pair(p, "=");
    post_params[u.uri_decode(nv.first)] = u.uri_decode(nv.second);
    // std::cout << "handle_x_www_form_urlencoded: \"" << u.uri_decode(nv.first) << "\" -> \"" << nv.second << "\"\n";
  }
}

void HTTPServerRequest::handle_content_line(const std::string &s)
{
  if (content_type == ContentType::unknown) {
    const std::string type = get_header("content-type");
    if (type.find("multipart/form-data") != std::string::npos) {
      content_type = ContentType::multipart_form_data;
      // std::cout << "Have a multipart form with type: \"" << type << "\"\n";
      std::vector<std::string> elements = UriUtils::split_params(type, ";");
      for (const auto &ele : elements) {
        // std::cout << "Element: \"" << ele << "\"\n";
        auto parts = UriUtils::split_pair(ele, "=");
        // std::cout << "Part \"" << parts.first << "\" -> \"" << parts.second << "\"\n";
        if (parts.first.front() == ' ')
          parts.first.erase(0, 1);
        if (parts.first == "boundary") {
          boundary_key = parts.second;
          // std::cout << "Boundary set to: \"" << boundary_key << "\"\n";
        }
      }
    } else if (type.find("application/x-www-form-urlencoded") != std::string::npos) {
      content_type = ContentType::x_www_form_urlencoded;
    } else if (type.find("application/json") != std::string::npos) {
      content_type = ContentType::application_json;
    } else if (method != HTTPMethod::get) {
      if (type.empty()) {
        std::cerr << "Content type for uri \"" << uri << "\" is not specified\n";
      } else {
        std::cerr << "Warning: cannot determine content type for request \"" << uri << "\" from type \""
                  << type << "\"\n";
      }
    }
  }
  switch (content_type) {
    case ContentType::multipart_form_data:
      handle_multipart_form_data(s);
      break;
    case ContentType::x_www_form_urlencoded:
      handle_x_www_form_urlencoded(s);
      break;
    case ContentType::application_json:
    case ContentType::unknown:
      content += s;
      break;
  }
}

std::string HTTPServerRequest::iget_map_entry(
    const std::string &name,
    const std::map<std::string, std::string> &map)
{
  for (const auto& i : map)
    if (istr_compare(i.first, name))
      return i.second;
  return "";
}

long HTTPServerRequest::get_content_length() {
  const std::string s = get_header("Content-Length");
  try {
    if (!s.empty())
      return std::stol(s);
  } catch (const std::invalid_argument& e) {
    // return -1;
  } catch (const std::out_of_range& e) {
    // return -1;
  }
  return -1;
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

/**
 * Helps simulate posting an array of parameters with names styled with a name
 * and square brackets, e.g. `my_name[1]`, `my_name[2]` etc.  This method
 * extracts a map of the post parameters matching that pattern with the passed
 * name, e.g. passing `my_name` would return a map as { {1, `my_first_value`},
 * {2, `my_second_value`} }
 *
 * \param array_name the name of the array style parameters
 *
 * \return a map of parameters matching array_name[n] with a key of n and the
 * corresponding post parameter value.
 *
 * \throws std::out_of_range if the numeric key value is too big to be stored as
 * an int
 *
 * \throws std::invalid_argument if the value denoted by square brackets cannot
 * be converted to an int
 */
std::map<long, std::string>
    HTTPServerRequest::extract_array_param_map(std::string array_name) const
{
  std::map<long, std::string> result_map;

  // Using regular expressions does not work reliably on macOS with Clang -
  // seems to be a bug when constructing the regular expression.  Works fine
  // using a staticly defined regex, but otherwise just fails to match at
  // runtime even though the same code passes unit tests.

  // try {
  //   const std::string expression = array_name + "\\[(\\d+)\\]";
  //   std::cout << "Regex: \"" << expression << "\"\n";
  //   const std::regex regex(expression);
  //   for (const auto &p : post_params) {
  //     std::smatch m;
  //     const std::string target = p.first;
  //     std::cout << "Checking target \"" << target << "\" -> \"" << p.second << "\"\n";
  //     if (std::regex_match(target, m, regex)) {
  //       std::cout << "There are " << m.size() << " matches \"" << m[0] << "\n";
  //       for (auto x = 0; x < m.size(); x++) {
  //         std::cout << "  submatch " << x << ": \"" << m[x] << "\"\n";
  //       }
  //       const int i = std::stoi(m[1]);
  //       std::cout << "Matched " << i << " \"" << m[1] << "\" with \"" << p.second << "\"\n";
  //       result_map[i] = p.second;
  //     } else {
  //       std::cout << "No match\n";
  //     }
  //   }
  // } catch (const std::regex_error &e) {
  //   std::cerr << "Exception extracting array from parameter map: "
  //             << e.what() << '\n';
  //   throw;
  // }

  for (const auto &p : post_params) {
    const std::string target = p.first;
    const std::string start_match = array_name + "[";
    if (target.find(start_match) == 0) {
      const auto len = start_match.size();
      const auto end = target.find(']', len);
      if (end != std::string::npos) {
        const std::string index = target.substr(len, end - len);
        const long i = std::stol(index);
        result_map[i] = p.second;
      }
    }
  }

  return result_map;
}

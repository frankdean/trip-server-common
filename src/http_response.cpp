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
#include "http_response.hpp"
#include "date_utils.hpp"
#include "../config.h"
#include "get_options.hpp"
#include <ostream>
#include <string>
#include <syslog.h>

using namespace fdsd::utils;
using namespace fdsd::web;

Logger HTTPServerResponse::logger("HTTPServerResponse",
                                  std::clog,
                                  Logger::info);

HTTPServerResponse::HTTPServerResponse() :
  headers(),
  content(),
  keep_alive(false),
  status_code(HTTPStatus::ok)
{
  set_header("Last-Modified", gmt());
  set_header("Date", gmt());
  set_header("Server", PACKAGE_NAME "/" PACKAGE_VERSION);
}

// void HTTPServerResponse::reset_content()
// {
//   content.clear();
//   content.str("");
//   // content.seekp(0); <-- This does not clear the contents, only moves the write position, effectively overwriting existing contents
// }

std::string HTTPServerResponse::gmt() const
{
  DateTime date_time;
  return date_time.get_time_as_rfc7231();
}

/// Escapes string with HTML entities
std::string HTTPServerResponse::x(std::string s)
{
  std::string retval;
  retval.reserve(s.length());
  for (auto i = s.begin(); i != s.end(); ++i) {
    switch(*i) {
      case '&':
        retval.append("&amp;");
        break;
      case '<':
        retval.append("&lt;");
        break;
      case '>':
        retval.append("&gt;");
        break;
      case '"':
        retval.append("&quot;");
        break;
      case '\'':
        retval.append("&#039;");
        break;
      default:
        retval.push_back(*i);
    }
  }
  return retval;
}

std::string HTTPServerResponse::get_status_message(HTTPStatus code) const
{
  std::string message;
  switch (code) {
    case HTTPStatus::ok:
      message.append("OK");
      break;
    case HTTPStatus::found:
      message.append("Found");
      break;
    case HTTPStatus::see_other:
      message.append("See Other");
      break;
    case HTTPStatus::not_modified:
      message.append("Not Modified");
      break;
    case HTTPStatus::bad_request:
      message.append("Bad Request");
      break;
    case HTTPStatus::unauthorized:
      message.append("Unauthorized");
      break;
    case HTTPStatus::forbidden:
      message.append("Forbidden");
      break;
    case HTTPStatus::not_found:
      message.append("Not Found");
      break;
    case HTTPStatus::payload_too_large:
      message.append("Payload Too Large");
      break;
    case HTTPStatus::internal_server_error:
      message.append("Internal Server Error");
      break;
    default:
      if (GetOptions::verbose_flag)
        std::cerr << "WARNING: unhandled standard response for status code" << (int) code << '\n';
      syslog(LOG_ERR,
             "No message has been coded in standard response handler for status code %d",
             (int) code);
      break;
  }
  return message;
}

/**
 * Sets the content with the status message corresponding to the passed HTTP
 * status code.
 */
void HTTPServerResponse::generate_standard_response(HTTPStatus code)
{
  content << "    <p>HTTP " << code << " &ndash; " << get_status_message(code) << "</p>\n";
}

std::string HTTPServerResponse::add_etag_header()
{
  auto hash = std::hash<std::string>{}(content.str());
  const std::string etag = std::to_string(hash);
  set_header("Etag", etag);
  return etag;
}

void HTTPServerResponse::get_http_response_message(std::ostream& os) const
{
  os << "HTTP/1.1 " << (int) status_code;
  os << ' ' << get_status_message(status_code) << "\r\n";
  for (const auto& elem : headers) {
    // std::cout << "Writing header \""
    //           << elem.name
    //           << "\" -> \""
    //           << elem.value
    //           << "\"\n";
    os << elem.name << ": " << elem.value << "\r\n";
  }
  os << "\r\n";
  if (status_code != HTTPStatus::found)
    os << content.str();
}

void HTTPServerResponse::set_header(std::string name, std::string value)
{
  bool exists = false;
  for (auto& i : headers) {
    if (istr_compare(i.name, name)) {
      exists = true;
      logger
        << Logger::debug
        << "The \"" << name
        << "\" header already exists with the value \"" << i.value
        << "\" updated with value \"" << value << "\""
        << Logger::endl;
      i.value = value;
      // std::cout << "Header: " << i.name << " -> " << i.value << '\n';
      break;
    }
  }
  if (!exists) {
    add_header(name, value);
  }
  // std::cout << "Headers after update:\n";
  // for (const auto& i : headers) {
  //   std::cout << "Header: " << i.name << " -> " << i.value << '\n';
  // }
}

void HTTPServerResponse::set_cookie(const std::string name, const std::string value, int max_age)
{
  std::ostringstream ss;
  ss.imbue(std::locale("C"));
  ss
    << name << '=' << value << "; ";

  if (max_age >= 0)
    ss << "Max-Age=" << max_age << "; ";
  ss << "Path=/; ";
  ss << "SameSite=Strict; HttpOnly"; // + "; Secure"

  // if (Application::verbose)
  //   std::cout << "Setting cookie to \"" << ss.str() << "\"\n";
  add_header("Set-Cookie", ss.str());
}

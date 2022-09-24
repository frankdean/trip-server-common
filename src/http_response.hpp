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
#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "logger.hpp"
#include <string>
#include <sstream>
#include <vector>

namespace fdsd
{
namespace web
{

bool istr_compare(const std::string& s1, const std::string& s2);

  enum HTTPStatus : int {
  ok = 200,
  found = 302,
  see_other = 303,
  bad_request = 400,
  unauthorized = 401,
  forbidden = 403,
  not_found = 404,
  internal_server_error = 500
};

struct response_header {
  std::string name;
  std::string value;
};

struct UserMessage {
  /// The type of the error message.
  enum Type {
    info,
    warn,
    error
  } type;
  UserMessage()
    :
    UserMessage("", "", info)
    {}
  UserMessage(std::string message)
    :
    UserMessage(message, "", info)
    {}
  UserMessage(std::string message, Type type)
    :
    UserMessage(message, "", type)
    {}
  UserMessage(std::string message, std::string code)
    :
    UserMessage(message, code, info)
    {}
  UserMessage(std::string message, std::string code, Type type)
    :
    message(message),
    code(code),
    type(type)
    {}
  ~UserMessage()
    {}
  /// A code to identify retrieve a message, e.g. to display it against a
  /// specific input field that is relevant to the error.
  std::string code;
  /// The message to display to the user.
  std::string message;
};

class HTTPServerResponse {
private:
  /// A vector containing name-value-pairs of Strings.  We cannot use a map
  /// for response headers as headers such as Set-Cookie may be specified
  /// multiple times in the response.
  std::vector<response_header> headers;
  std::vector<UserMessage> messages;
  static utils::Logger logger;

protected:
public:
  HTTPServerResponse();
  ~HTTPServerResponse() {}
  std::string gmt() const;
  /// Escapes string with HTML entities
  static std::string x(std::string s);
  std::ostringstream content;
  // void reset_content();
  bool keep_alive;
  HTTPStatus status_code;
  /// Generates a standard response based on the passed status code.
  virtual void generate_standard_response(HTTPStatus code);
  /// Returns the current body, wrapped with status code and HTTP headers.
  virtual void get_http_response_message(std::ostream& os) const;
  virtual void add_etag_header();
  std::string get_status_message(HTTPStatus code) const;
  void set_cookie(std::string name, std::string value, int max_age = -1);
  /// Sets a header, replacing any header of the same name
  void set_header(std::string name, std::string value);
  /// Adds a new header, allowing duplicates of the same name, e.g. Set-Cookie
  void add_header(std::string name, std::string value) {
    response_header h;
    h.name = name;
    h.value = value;
    headers.push_back(h);
  }
  /// Returns the value of the header with name, or an empty string if not
  /// found.
  std::string get_header(std::string name) const {
    for (const auto& i : headers) {
      if (istr_compare(i.name, name))
        return i.value;
    }
    return "";
  }
  void add_message(UserMessage message) {
    messages.push_back(message);
  }
  /**
   * Fetches the message containing the passed identifier.  If the message
   * does not exist, an info message type is returned with an empty code and
   * message.
   * @param string code the message identifier.
   * @return the message identified by the passed code.
   */
  UserMessage get_message(std::string code) const;
  // TODO - Move to Request Handler
  virtual void append_messages_as_html();
  virtual void create_error_page(std::string page_title);
};

} // namespace web
} // namespace fdsd

#endif // HTTP_RESPONSE_HPP

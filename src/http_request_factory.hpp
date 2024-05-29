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
#ifndef HTTP_REQUEST_FACTORY_HPP
#define HTTP_REQUEST_FACTORY_HPP

#include "logger.hpp"
#include "http_request_handler.hpp"
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace fdsd
{
namespace web
{

class HTTPServerRequest;
class HTTPServerResponse;


class HTTPRequestFactory
{
private:
  /// An optional prefix to all the application URLs.
  std::string uri_prefix;
  static fdsd::utils::Logger logger;
  long maximum_request_size;
public:
  HTTPRequestFactory(std::string uri_prefix);
  virtual ~HTTPRequestFactory() {}
  virtual std::unique_ptr<BaseRequestHandler>
      create_request_handler(HTTPServerRequest &request,
                             HTTPServerResponse& response) const;
  virtual std::unique_ptr<HTTPServerResponse>
      create_response_object() const;
  std::string get_uri_prefix() const { return uri_prefix; }
  virtual long get_maximum_request_size() const {
    return maximum_request_size;
  }
protected:
  std::vector<std::shared_ptr<BaseRequestHandler>> pre_login_handlers;
  std::vector<std::shared_ptr<HTTPRequestHandler>> post_login_handlers;
  virtual std::string get_session_id_cookie_name() const = 0;
  virtual std::string get_user_id(std::string session_id) const = 0;
  virtual bool is_login_uri(std::string uri) const = 0;
  virtual std::unique_ptr<HTTPRequestHandler> get_login_handler() const = 0;
  virtual bool is_logout_uri(std::string uri) const = 0;
  virtual std::unique_ptr<HTTPRequestHandler> get_logout_handler() const = 0;
  virtual bool is_application_prefix_uri(std::string uri) const = 0;
  virtual std::unique_ptr<HTTPRequestHandler> get_not_found_handler() const = 0;
  void refresh_session(const HTTPServerRequest& request,
                       HTTPServerResponse& response) const;
  virtual std::unique_ptr<BaseRequestHandler> manage_session_state(HTTPServerRequest &request,
                           HTTPServerResponse& response) const;
  virtual std::unique_ptr<BaseRequestHandler> handle_pre_login(HTTPServerRequest &request,
                       HTTPServerResponse& response) const;
  virtual std::unique_ptr<BaseRequestHandler> handle_post_login(HTTPServerRequest &request,
                        HTTPServerResponse& response) const;
  virtual bool is_valid_session(std::string session_id, std::string user_id) const = 0;
};

} // namespace web
} // namespace fdsd

#endif // HTTP_REQUEST_FACTORY_HPP

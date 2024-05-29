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
#include "../config.h"
#include "http_request_factory.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "http_request_handler.hpp"
#include "logger.hpp"

using namespace fdsd::utils;
using namespace fdsd::web;

Logger HTTPRequestFactory::logger("HTTPRequestFactory", std::clog, Logger::info);

/**
 * Creates an instance of the request factory.
 *
 * \param uri_prefix the prefix to use for the entire application.  Defaults
 * to an empty string.
 */
HTTPRequestFactory::HTTPRequestFactory(std::string uri_prefix)
  : uri_prefix(uri_prefix),
    maximum_request_size(1024 * 1024 * 12),
    pre_login_handlers(),
    post_login_handlers()
{
}

/**
 * If the user has requested login or logout, returns the relevant handler.
 * Otherwise, if the user is logged in and the session is still valid, it
 * refreshes the session and returns nullptr.
 */
std::unique_ptr<BaseRequestHandler>
    HTTPRequestFactory::manage_session_state(HTTPServerRequest &request,
                                             HTTPServerResponse& response) const
{
  if (is_login_uri(request.uri)) {
    if (logger.is_level(Logger::debug))
      logger << Logger::debug << "Returning a login handler for \""
             << request.uri << "\"" << Logger::endl;
    auto h = get_login_handler();
    return h;
  }

  if (is_logout_uri(request.uri)) {
    if (logger.is_level(Logger::debug))
      logger << Logger::debug << "Returning a logout handler for \""
             << request.uri << "\"" << Logger::endl;
    auto h = get_logout_handler();
    return h;
  }

  std::string session_id = request.get_cookie(get_session_id_cookie_name());
  std::string user_id = get_user_id(session_id);

  if (logger.is_level(Logger::debug))
    logger << Logger::debug <<
      "Checking whether session is valid for user ID: \""
           << user_id
           << "\" with session ID: \""
           << session_id << Logger::endl;

  if ((session_id.empty() ||
       !is_valid_session(session_id, user_id)) &&
      !is_application_prefix_uri(request.uri)) {

    if (logger.is_level(Logger::debug))
      logger << Logger::debug
             << "The user is not logged in and the URL is not prefixed with "
        "the application URL.  Handling as not found.\n" << Logger::endl;

    auto h = get_not_found_handler();
    if (logger.is_level(Logger::debug))
      logger << Logger::debug
             << "Created \"" << h->get_handler_name() << "\" handler\n";
    return h;
  } else {
    if (logger.is_level(Logger::debug))
      logger << Logger::debug << "Refreshing session for user ID: \""
             << user_id << "\"" << Logger::endl;
    refresh_session(request, response);
    request.user_id = user_id;
  }
  return nullptr;
}

std::unique_ptr<BaseRequestHandler>
    HTTPRequestFactory::create_request_handler(HTTPServerRequest &request,
                                               HTTPServerResponse& response) const
{
  auto rh = manage_session_state(request, response);
  if (rh !=  nullptr) {
    if (logger.is_level(Logger::debug))
      logger << Logger::debug
             << "Created a \"" << rh->get_handler_name() << "\" handler"
             << Logger::endl;
    return rh;
  }

  rh = handle_post_login(request, response);
  if (rh != nullptr) {
    if (logger.is_level(Logger::debug))
      logger << Logger::debug
             << "Created a \"" << rh->get_handler_name() << "\" handler"
             << Logger::endl;
    return rh;
  }

  rh = handle_pre_login(request, response);
  if (rh !=  nullptr) {
    if (logger.is_level(Logger::debug))
      logger << Logger::debug
             << "Created a \"" << rh->get_handler_name() << "\" handler"
             << Logger::endl;
    return rh;
  }

  if (logger.is_level(Logger::debug))
    logger << Logger::debug
           << "Cannot handle URI: \"" << request.uri << "\" handling as not found"
           << Logger::endl;

  auto h = get_not_found_handler();
  if (logger.is_level(Logger::debug))
    logger << Logger::debug
           << "Created \"" << h->get_handler_name() << "\" handler\n";
  return h;
}

std::unique_ptr<HTTPServerResponse>
    HTTPRequestFactory::create_response_object() const
{
  return std::unique_ptr<HTTPServerResponse>(new HTTPServerResponse);
}

void HTTPRequestFactory::refresh_session(
    const HTTPServerRequest& request,
    HTTPServerResponse& response) const
{
  // persist_session();
  // std::string cookie_name = get_session_id_cookie_name();
  // std::string session_id = request.get_cookie(cookie_name);
}

std::unique_ptr<BaseRequestHandler>
    HTTPRequestFactory::handle_pre_login(HTTPServerRequest &request,
                                         HTTPServerResponse& response) const
{
  for (const auto& h : pre_login_handlers) {
    if (logger.is_level(Logger::debug))
      logger << Logger::debug << "Checking whether \"" << h->get_handler_name()
             << "\" can handle request without login for \"" << request.uri
             << '"'<< Logger::endl;
    if (h->can_handle(request)) {
      if (logger.is_level(Logger::debug))
        logger << Logger::debug << h->get_handler_name()
               << " can handle request for \"" << request.uri << '"'
               << Logger::endl;
      return h->new_instance();
    }
  }
  return nullptr;
}

std::unique_ptr<BaseRequestHandler>
    HTTPRequestFactory::handle_post_login(HTTPServerRequest &request,
                                          HTTPServerResponse& response) const
{
  for (const auto& h : post_login_handlers) {
    if (logger.is_level(Logger::debug))
      logger << Logger::debug << "Checking whether \"" << h->get_handler_name()
             << "\" can handle request for \"" << request.uri << '"'
             << Logger::endl;
    if (h->can_handle(request)) {
      if (logger.is_level(Logger::debug))
        logger << Logger::debug << h->get_handler_name()
               << " can handle request for \"" << request.uri << '"'
               << Logger::endl;
      return h->new_instance();
    }
  }
  return nullptr;
}

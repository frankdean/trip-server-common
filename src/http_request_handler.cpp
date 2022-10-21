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
#include "../config.h"
#include "http_request_handler.hpp"
#include "date_utils.hpp"
#include "file_utils.hpp"
#include "get_options.hpp"
#include "http_response.hpp"
#include "http_request.hpp"
#include "session.hpp"
#include "uri_utils.hpp"
#include "uuid.hpp"
#include <fstream>
#include <vector>
#ifdef HAVE_BOOST_LOCALE
#include <boost/locale.hpp>
#endif

using namespace fdsd::utils;
using namespace fdsd::web;
#ifdef HAVE_BOOST_LOCALE
using namespace boost::locale;
#endif

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types

struct mime_type {
  const std::string extension;
  const std::string mime_type;
} mime_type_map[] = {
  { "css", "text/css; charset=UTF-8" },
  { "geojson",  "application/geojson; charset=UTF-8" },
  { "gif",  "image/gif" },
  { "gpx",  "application/gpx+xml" },
  { "htm",  "text/html; charset=UTF-8" },
  { "html", "text/html; charset=UTF-8" },
  { "kml",  "application/vnd.google-earth.kml+xml" },
  { "pdf", "application/pdf" },
  { "jpg",  "image/jpeg" },
  { "js",   "text/javascript; charset=UTF-8" },
  { "json",  "application/json; charset=UTF-8" },
  { "mjs",   "text/javascript; charset=UTF-8" },
  { "png",  "image/png" },
  { "txt",  "text/plain; charset=UTF-8" },
  { "yaml", "application/x-yaml"},
  { "yml", "application/x-yaml"}
};

void BaseRequestHandler::redirect(const HTTPServerRequest& request,
                                  HTTPServerResponse& response,
                                  std::string location) const
{
  response.status_code = HTTPStatus::found;
  response.set_header("Location", location);
}

/// Pads the string to the passed length.  The string is not shortened.
void BaseRequestHandler::pad_left(std::string &s,
                                   std::string::size_type length,
                                   char c) const
{
  int n = length - s.length();
  std::string padding;
  if (n > 0) {
    while (padding.length() < n)
      padding.push_back(c);
    s = padding + s;
  }
  // if (s.length() != length)
  //   std::cout << "Error in padding \"" << s << "\"\n";
}

/// Pads the string to the passed length.  The string is not shortened.
void BaseRequestHandler::pad_right(std::string &s,
                                   std::string::size_type length,
                                   char c) const
{
  while (s.length() < length)
    s.push_back(c);
}

/// Returns the mime type for the specified extension.
std::string BaseRequestHandler::get_mime_type(std::string extension) const
{
  for (const auto& m : mime_type_map) {
    if (m.extension == extension)
      return m.mime_type;
  }
  return "application/octet-stream";
}

/// Escapes string with HTML entities
std::string BaseRequestHandler::x(std::string s) const
{
  return HTTPServerResponse::x(s);
}

std::string BaseRequestHandler::get_redirect_uri(
    const HTTPServerRequest& request) const
{
  std::ostringstream os;
  os << request.uri;
  for (auto qp = request.query_params.begin(); qp !=
         request.query_params.end(); ++qp) {
    os << (qp == request.query_params.begin() ? '?' : '&')
       << qp->first << '=' << qp->second;
  }
  return os.str();
}

void CssRequestHandler::handle_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Accept-Ranges
  response.set_header("Accept-Ranges", "none");
  append_stylesheet_content(request, response);
  response.add_etag_header();
  response.set_header("Content-Length", std::to_string(response.content.str().length()));
}

#ifdef ALLOW_STATIC_FILES

/**
 * Handles reading a file referenced from the passed request's URI value.  The
 * contents of the file are appended to the response stream.
 *
 * \param request with the `uri` value set to the file to be read.
 *
 * \param response to which to write the file's contents.
 */
void FileRequestHandler::append_body_content(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  std::string uri = request.uri;
  FileUtils::strip_prefix(get_uri_prefix(), uri);
  FileUtils::strip_query_params(uri);
  std::string relative_path = UriUtils::uri_decode(uri, false);

  if (!relative_path.empty() && relative_path.substr(0, 1) ==
      FileUtils::path_separator)
    relative_path.erase(0, 1);

  std::string full_path = document_root + relative_path;
  // std::cout << "Full path to file: \"" << full_path << "\"\n";

  if (!FileUtils::is_file(full_path)) {
    response.status_code = HTTPStatus::not_found;
    create_full_html_page_for_standard_response(response);
    return;
  }
  std::ifstream cin(full_path);
  // if (cin)
  response.content << cin.rdbuf();
  response.add_etag_header();
  // Set the Last-Modified header with the file's timestamp
  auto file_info = FileUtils::get_file_details(full_path);
  response.set_header("Last-Modified", file_info.datetime.get_time_as_rfc7231());
  response.set_header("Content-Type", get_mime_type(FileUtils::get_extension(full_path)));
  set_content_headers(response);
}

void FileRequestHandler::set_content_headers(HTTPServerResponse& response) const
{
  // Content-Type is set in append_body_content() as we do not know mime type here
  response.set_header("Cache-Control", "no-cache");
  response.set_header("Content-Length", std::to_string(response.content.str().length()));
}

#ifdef ALLOW_DIRECTORY_LISTING
/**
 * Reads the contents of a directory, using the URI in the passed request
 * object.
 *
 * \param request with the `uri` value set to the directory to be read.
 *
 * \param response to which to write the file's contents.
 */
void FileRequestHandler::handle_directory(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) const
{
  std::string uri = request.uri;
  FileUtils::strip_prefix(get_uri_prefix(), uri);
  FileUtils::strip_query_params(uri);
  std::string relative_path = UriUtils::uri_decode(uri, false);
  std::string full_path = document_root + relative_path;
  auto dir_list = FileUtils::get_directory(full_path);
  response.content
    << "    <h1>Index of /"
    << relative_path
    << "</h1>\n    <hr/>\n<pre>\n";
  if (!dir_list.empty()) {

    // std::cout << "application_prefix_url: \"" << application_prefix_url << "\"\n";
    // std::cout << "document_root: \"" << document_root << "\"\n";
    // std::cout << "relative_path: \"" << relative_path << "\"\n";

    for (auto& e : dir_list) {

      // std::cout << "file \""
      //           << e.name
      //           << "\" is a "
      //           << FileUtils::get_type(e.type)
      //           << '\n';

      if (!(e.type == FileUtils::directory ||
            e.type == FileUtils::regular_file ||
            e.type == FileUtils::symbolic_link) ||
          e.name == "." || e.name.empty() ||
          (e.name != ".." && e.name.substr(0, 1) == ".")) {
        continue;
      }

      std::string size;
      std::string url(UriUtils::uri_encode(e.name, false));

      if (e.type == FileUtils::directory) {
        url += FileUtils::path_separator;
        e.name += FileUtils::path_separator;
        size = "-";
      } else {
        size = std::to_string(e.size);
      }
      std::string name(x(e.name) + "</a>");

      // std::cout << "URL: \"" << url << "\"\n";
      // std::cout << "Name: \"" << name << "\"\n";
      std::string date(e.datetime.to_string());
      pad_right(name, 44);
      pad_left(date, 38);
      pad_left(size, 20);
      response.content
        << "<a href=\"" << url << "\">"
        << name
        << date
        << size
        << "\n";
    }
    response.content << "</pre>\n";

  } else {
    response.content
      << "    <p>empty directory</p>\n";
  }
}
#endif

void FileRequestHandler::handle_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  std::string relative_path = UriUtils::uri_decode(request.uri, false);
  FileUtils::strip_prefix(get_uri_prefix(), relative_path);
  FileUtils::strip_query_params(relative_path);
  // std::cout << "handle_request() relative path: \"" << relative_path << "\"\n";

  // Forbid hidden files and directories
  if (relative_path.find("/.") != std::string::npos ||
      // Enable the following rule to stop directory traversal, should hidden
      // files or directories be allowed in the future.  (The rule above also
      // prohibits directory traversal.)
      //   relative_path.find("/../") != std::string::npos ||
      relative_path.find("../") == 0) {
    response.status_code = HTTPStatus::forbidden;
    create_full_html_page_for_standard_response(response);
    return;
  }
  // std::cout << "Path is OK\n";
  std::string full_path = document_root + relative_path;
#ifdef ALLOW_DIRECTORY_LISTING
  if (FileUtils::is_directory(full_path)) {
    try {
      append_doc_type(response.content);
      append_html_start(response.content);
      response.content <<
        "  <head>\n"
        "    <meta charset=\"UTF-8\" >\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" >\n"
        "    <title>" << "Index of: /" + relative_path << "</title>\n"
        "  </head>\n"
        "  <body>\n";
      handle_directory(request, response);
      response.content << "  </body>\n";
      append_html_end(response.content);
      set_content_headers(response);
      response.set_header("Content-Type", get_mime_type("html"));
      return;
    } catch (const FileUtils::DirectoryAccessFailedException& e) {
      response.content.clear();
      response.content.str("");
      response.status_code = HTTPStatus::internal_server_error;
      create_full_html_page_for_standard_response(response);
      return;
    }
  }
#endif
  append_body_content(request, response);
}

bool FileRequestHandler::can_handle(const HTTPServerRequest& request) const
{
  // std::cout << "can_handle() - uri: \"" << request.uri << "\"\n";
  std::string uri = request.uri;
  FileUtils::strip_prefix(get_uri_prefix(), uri);
  FileUtils::strip_query_params(uri);
  std::string relative_path = UriUtils::uri_decode(uri, false);
  // std::cout << "Can handle relative path? \"" << relative_path << "\"\n";

  std::string full_path = document_root + relative_path;
  // std::cout << "Can handle full path: ? \"" << full_path << "\"\n";
  if (FileUtils::is_directory(full_path)) {
#ifndef ALLOW_DIRECTORY_LISTING
    // std::cout << '"' << full_path << "\" is a directory... returning false\n";
    return false;
#else
    // std::cout << '"' << full_path << "\" is a directory... returning true\n";
    return true;
#endif
  }
  if (FileUtils::is_file(full_path))
    return true;
  return false;
}
#endif // ALLOW_STATIC_FILES

Logger HTTPRequestHandler::logger(
  "HTTPRequestHandler",
  std::clog,
  Logger::info);

void BaseRequestHandler::append_doc_type(std::ostream& os) const
{
  os << "<!DOCTYPE html>\n";
}

void BaseRequestHandler::append_html_start(std::ostream& os) const
{
  os << "<html lang=\"" << get_html_lang() << "\">\n";
}

void BaseRequestHandler::append_head_start(std::ostream& os) const
{
  os << "  <head>\n";
}

void BaseRequestHandler::append_head_section(std::ostream& os) const
{
  os <<
    "    <meta charset=\"UTF-8\" >\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" >\n";
}

void BaseRequestHandler::append_head_title_section(std::ostream& os) const
{
  if (!get_page_title().empty())
    os << "    <title>"
       << get_page_title()
       << "    </title>\n";
}

void BaseRequestHandler::append_head_content(std::ostream& os) const
{
  os <<
    "    <style>.footer {padding: 10px; background-color: #f5f5f5;}</style>\n";
}

void BaseRequestHandler::append_head_end(std::ostream& os) const
{
  os << "  </head>\n";
}

void BaseRequestHandler::append_body_start(std::ostream& os) const
{
  os << "  <body>\n";
}

void BaseRequestHandler::append_footer_content(std::ostream& os) const
{
  os
    << "    <p class=\"footer\">"
    << PACKAGE << " version " << VERSION << "</p>\n";
}

void BaseRequestHandler::append_body_end(std::ostream& os) const
{
  os << "  </body>\n";
}

void BaseRequestHandler::append_html_end(std::ostream& os) const
{
  os << "</html>\n";
}

void BaseRequestHandler::set_content_headers(HTTPServerResponse& response) const
{
  response.set_header("Content-Length",
                      std::to_string(response.content.str().length()));
  response.set_header("Content-Type", get_mime_type("html"));
  response.set_header("Cache-Control", "no-cache");
}

void BaseRequestHandler::create_full_html_page_for_standard_response(
    HTTPServerResponse& response)
{
  const std::string status_message =
    response.get_status_message(response.status_code);
  set_page_title(status_message);
  append_doc_type(response.content);
  append_html_start(response.content);
  append_head_start(response.content);
  append_head_section(response.content);
  append_head_title_section(response.content);
  append_head_content(response.content);
  append_head_end(response.content);
  append_body_start(response.content);
  response.content
    << "<h1>" << status_message << "</h1>\n";
  append_body_end(response.content);
  append_html_end(response.content);
  set_content_headers(response);
}

void BaseRequestHandler::handle_bad_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  response.content.clear();
  response.content.str("");
  response.status_code = HTTPStatus::bad_request;
  create_full_html_page_for_standard_response(response);
}

void HTTPRequestHandler::handle_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  if (logger.is_level(Logger::debug))
    logger << Logger::debug
           << get_handler_name()
           << " handling request for \""
           << request.uri << '\"'
           << Logger::endl;
  int length = response.content.str().length();
  if (length > 0) {
    logger << Logger::warn
           << "Content exists with "
           << length << " characters\n"
           << response.content.str()
           << Logger::endl;
  }
  preview_request(request, response);
  if (response.status_code != HTTPStatus::found) {
    append_doc_type(response.content);
    append_html_start(response.content);
    append_head_start(response.content);
    append_head_section(response.content);
    append_head_title_section(response.content);
    append_head_content(response.content);
    append_head_end(response.content);
    append_body_start(response.content);
    append_header_content(response.content);
    logger << Logger::debug
           << "Status code before do_handle_request "
           << response.status_code << Logger::endl;
    try {
      do_handle_request(request, response);
    } catch (const BadRequestException &e) {
      handle_bad_request(request, response);
    } catch (const std::invalid_argument &e) {
      handle_bad_request(request, response);
    } catch (const std::out_of_range &e) {
      handle_bad_request(request, response);
    }
    logger << Logger::debug
           << "Status code after do_handle_request "
           << response.status_code << Logger::endl;
    if (response.status_code != HTTPStatus::found) {
      append_footer_content(response.content);
      append_pre_body_end(response.content);
      append_body_end(response.content);
      append_html_end(response.content);
      set_content_headers(response);
    }
  }
  logger << Logger::debug
         << "Finished handle_request"
         << Logger::endl;
}

void CssRequestHandler::set_content_headers(HTTPServerResponse& response) const
{
  response.set_header("Content-Length", std::to_string(response.content.str().length()));
  response.set_header("Content-Type", get_mime_type("css"));
  response.set_header("Cache-Control", "no-cache");
}

void BaseAuthenticatedRequestHandler::append_login_body(
    std::ostream& os,
    bool login_success) const
{
  os << "<div id=\"login\" class=\"container\">\n";
  if (!login_success) {
    os << "<div><h2>Error &ndash; login failed &ndash; try again...</h2></div>\n";
  }
  os << "  <div class=\"container\">\n"
    "    <form name=\"form\" class=\"form-signin\" action=\"" + get_login_uri() + "\" method=\"POST\">\n"
    "      <h2 class=\"form-signin-heading\">Login</h2>\n"
    "      <table>\n"
    "        <tr>\n"
    "          <td>\n"
    // Label prompting a user to enter their username
    "            <label for=\"input-email\" class=\"sr-only\">" <<
#ifdef HAVE_BOOST_LOCALE
    translate("Username")
#else
    "Username"
#endif
     << "</label>\n"
    "          </td>\n"
    "          <td>\n"
    // Placeholder prompting user to enter their username
    "            <input id=\"input-email\" type=\"text\" class=\"form-control\" placeholder=\"" <<
#ifdef HAVE_BOOST_LOCALE
    translate("Username")
#else
    "Username"
#endif
     << "\" name=\"email\" size=\"25\" />\n"
    "          </td>\n"
    "        </tr>\n"
    "        <tr>\n"
    "          <td>\n"
    // Label prompting a user to enter their password
    "            <label for=\"input-password\" class=\"sr-only\">" <<
#ifdef HAVE_BOOST_LOCALE
    translate("Password")
#else
    "Password"
#endif
     << "</label>\n"
    "          </td>\n"
    "          <td>\n"
    // Placeholder prompting user to enter their password
    "            <input id=\"input-password\" type=\"password\" class=\"form-control\" placeholder=\"" <<
#ifdef HAVE_BOOST_LOCALE
    translate("Password")
#else
    "Password"
#endif
     << "\" name=\"password\" size=\"25\" />\n"
    "          </td>\n"
    "        </tr>\n"
    "        <tr>\n"
    "          <td>&nbsp;</td>\n"
    // Button label prompting the user to login
    "          <td><button id=\"btn-submit\" value=\"Submit\" class=\"btn btn-lg btn-success my-3\">" <<
#ifdef HAVE_BOOST_LOCALE
    translate("Login")
#else
    "Login"
#endif
     << "</button></td>\n"
    "        </tr>\n"
    "      </table>\n"
    "    </form>\n"
    "  </div>\n"
    "</div>\n";
}

Logger AuthenticatedRequestHandler::logger(
  "AuthenticatedRequestHandler",
  std::clog,
  Logger::info);

void AuthenticatedRequestHandler::preview_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response)
{
  // Skip, if the user isn't logged in.
  // Will fail in AuthenticatedRequestHandler::do_handle_request().
  session_id = request.get_cookie(get_session_id_cookie_name());
  user_id = get_session_manager()->get_session_user_id(session_id);

  if (logger.is_level(Logger::debug))
    logger << Logger::debug
           << "Got session ID: \"" << session_id << "\" from request cookie.  "
           << "The session ID belongs to user ID \""
           << user_id << '"' << Logger::endl;

  if (!user_id.empty())
    do_preview_request(request, response);
}

void AuthenticatedRequestHandler::do_handle_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) const
{
  if (user_id.empty()) {

    if (logger.is_level(Logger::debug))
      logger << Logger::debug
             << "User \"" << user_id << "\" not logged in"
             << Logger::endl;

    response.status_code = HTTPStatus::unauthorized;

    if (request.method == HTTPMethod::get) {

      if (logger.is_level(Logger::debug))
        logger << Logger::debug
               <<"Requested URI: "
               << request.uri << Logger::endl;

      std::string redirect_uri = get_redirect_uri(request);
      // Handle invalid looking redirects
      if (!redirect_uri.empty() &&
          redirect_uri.find(get_uri_prefix()) == 0 &&
          redirect_uri != get_login_uri()) {

        if (logger.is_level(Logger::debug))
          logger << Logger::debug
                 << "Setting redirect cookie to: \"" << redirect_uri << '"'
                 << Logger::endl;

        response.set_cookie(get_login_redirect_cookie_name(), redirect_uri);
      }
    }
    append_login_body(response.content, true);

    if (logger.is_level(Logger::debug))
      logger << Logger::debug
             << "Redirecting to specified login URL \""
             << get_login_uri()
             << '"' << Logger::endl;

    redirect(request, response, get_login_uri());
    return;
  }

    if (logger.is_level(Logger::debug))
      logger << Logger::debug
             << "Calling handle_authenticated_request"
             << Logger::endl;

  handle_authenticated_request(request, response);
}

Logger HTTPLoginRequestHandler::logger("HTTPLoginRequestHandler",
                                       std::clog, fdsd::utils::Logger::info);

void HTTPLoginRequestHandler::do_handle_request(
    const HTTPServerRequest& request,
    fdsd::web::HTTPServerResponse& response) const
{
  if (request.method == HTTPMethod::get) {
    std::string redirect_uri = get_redirect_uri(request);

    // Handle invalid looking redirects
    if (!redirect_uri.empty() &&
        redirect_uri.find(get_uri_prefix()) == 0 &&
        redirect_uri != get_login_uri()) {
      if (GetOptions::verbose_flag)
        std::cout << "Setting redirect cookie to: \"" << redirect_uri << "\"\n";
      response.set_cookie(get_login_redirect_cookie_name(), redirect_uri);
    }
    append_login_body(response.content, true);
  } else if (request.method == HTTPMethod::post) {
    std::map<std::string, std::string> post_params = request.get_post_params();
    // std::cout << "Content: \"" << request.content << "\"\n";
    std::string email = post_params["email"];
    std::string password = post_params["password"];
    if (validate_password(email, password)) {
      std::string session_id = UUID::generate_uuid();
      get_session_manager()->save_session(session_id,
                                          get_user_id_by_email(email));
      // Even though we'll expire the session after an hour, no need to set a
      // cookie expiration as using an ancient cookie on an expired session
      // will stil fail on login.  This also means we don't need to keep
      // updating the session cookie.
      response.set_cookie(get_session_id_cookie_name(), session_id);
      std::string redirect_url =
        request.get_cookie(get_login_redirect_cookie_name());
      if (GetOptions::verbose_flag) {
#ifdef HAVE_BOOST_LOCALE
        // Message showing the server administrator the URL a browser is being redirected to after login
        std::cout << "Login redirect cookie: " << redirect_url << '\n';
#else
        std::cout << "Login redirect cookie: " << redirect_url << '\n';
#endif
      }
      // Immediately expire the cookie
      response.set_cookie(get_login_redirect_cookie_name(), request.uri, 0);
      if (redirect_url.empty() || redirect_url ==
          get_login_uri() || redirect_url.front() != '/') {
        std::string default_uri = get_default_uri();
        if (GetOptions::verbose_flag)
          std::cout << "Redirect URL was empty, redirecting to: "
                    << default_uri << '\n';

        redirect_url = default_uri;
      }
      if (GetOptions::verbose_flag) {
#ifdef HAVE_BOOST_LOCALE
        // Message showing the server administrator the URL a browser is being redirected to
        std::cout << format(translate("Redirecting to: {1}")) % redirect_url << '\n';
#else
        std::cout << "Redirecting to: " << redirect_url << '\n';
#endif
      }
      redirect(request, response, redirect_url);
    } else {
      if (GetOptions::verbose_flag)
        std::cout << "Login failure, bad credentials\n";
      append_login_body(response.content, false);
      response.status_code = HTTPStatus::unauthorized;
    }
  } else {
    if (GetOptions::verbose_flag)
      std::cout << "Login bad request\n";
    response.generate_standard_response(HTTPStatus::bad_request);
  }
}

void HTTPLogoutRequestHandler::do_handle_request(
    const HTTPServerRequest& request,
    fdsd::web::HTTPServerResponse& response) const
{
  std::string session_id = request.get_cookie(get_session_id_cookie_name());
  get_session_manager()->invalidate_session(session_id);
  // Immediately expire the cookies
  if (GetOptions::verbose_flag)
    std::cout << "Logout\n";
  // std::cout << "Expiring existing login redirect cookie\n";
  response.set_cookie(get_login_redirect_cookie_name(), "", 0);
  if (GetOptions::verbose_flag)
    std::cout << "Logout redirecting to default uri \""
              << get_default_uri() << "\"\n";
  redirect(request, response, get_default_uri());
}

Logger HTTPNotFoundRequestHandler::logger("HTTPNotFoundRequestHandler",
                                   std::clog,
                                   fdsd::utils::Logger::info);

void HTTPNotFoundRequestHandler::do_handle_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response) const
{
  response.status_code = HTTPStatus::not_found;
  response.generate_standard_response(HTTPStatus::not_found);
}

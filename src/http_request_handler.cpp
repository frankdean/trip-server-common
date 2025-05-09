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
#include <sstream>
#include <vector>
#include <syslog.h>
#ifdef HAVE_BOOST_LOCALE
#include <boost/locale.hpp>
#endif
#ifdef HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H
#include <pqxx/pqxx>
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
  { "js.map",   "text/javascript; charset=UTF-8" },
  { "json",  "application/json; charset=UTF-8" },
  { "mjs",   "text/javascript; charset=UTF-8" },
  { "png",  "image/png" },
  { "svg", "image/svg+xml" },
  { "txt",  "text/plain; charset=UTF-8" },
  { "yaml", "application/x-yaml"},
  { "yml", "application/x-yaml"}
};

void BaseRequestHandler::redirect(const HTTPServerRequest& request,
                                  HTTPServerResponse& response,
                                  std::string location) const
{
  (void)request; // unused
  response.content.clear();
  response.content.str("");
  response.status_code = HTTPStatus::found;
  response.set_header("Location", location);
}

/// Pads the string to the passed length.  The string is not shortened.
void BaseRequestHandler::pad_left(std::string &s,
                                   std::string::size_type length,
                                   char c) const
{
  std::string::size_type n = length - s.length();
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

// Escapes string with HTML entities
// std::string BaseRequestHandler::x(std::string s) const
// {
//  return HTTPServerResponse::x(s);
// }

/// Escapes string with HTML entities
std::string BaseRequestHandler::x(std::optional<std::string> s)
{
  return s.has_value() ? HTTPServerResponse::x(s.value()) : "";
}

std::string BaseRequestHandler::get_redirect_uri(
    const HTTPServerRequest& request) const
{
  std::ostringstream os;
  os << request.uri;
  if (request.uri.find('?') == std::string::npos) {
    auto query_params = request.get_query_params();
    for (auto qp = query_params.begin(); qp !=
           query_params.end(); ++qp) {
      os << (qp == query_params.begin() ? '?' : '&')
         << qp->first << '=' << qp->second;
    }
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

#ifdef ENABLE_STATIC_FILES

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
  const std::string new_etag = response.add_etag_header();
  // Set the Last-Modified header with the file's timestamp
  auto file_info = FileUtils::get_file_details(full_path);

  const std::string request_etag = request.get_header("If-None-Match");
  if (request_etag.empty() && (
          request.method == HTTPMethod::get ||
          request.method == HTTPMethod::head)) {
    const std::string if_modified_since = request.get_header("If-Modified-Since");
    if (!if_modified_since.empty()) {
      DateTime if_modified_since_date(if_modified_since);
      // std::cout << "If modified since: \"" << if_modified_since
      //           << "\" converted to: \"" << if_modified_since_date << "\"\n";
      // std::cout << "Comparing with file datetime: " << file_info.datetime << '\n';
      if (!(if_modified_since_date < file_info.datetime)) {
        response.status_code = HTTPStatus::not_modified;
        return;
      }
    }
  }
  response.set_header("Cache-Control", "no-cache");
  response.set_header("Last-Modified", file_info.datetime.get_time_as_rfc7231());
  if (!request_etag.empty() && new_etag == request_etag) {
    response.content.clear();
    response.content.str("");
    response.status_code = HTTPStatus::not_modified;
  } else {
    response.set_header("Content-Type", get_mime_type(FileUtils::get_extension(full_path)));
    set_content_headers(response);
  }
}

void FileRequestHandler::set_content_headers(HTTPServerResponse& response) const
{
  // Content-Type is set in append_body_content() as we do not know mime type here
  response.set_header("Content-Length", std::to_string(response.content.str().length()));
}

#ifdef ENABLE_DIRECTORY_LISTING
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
  // Relative path handling is much easier if the browser is using a URL ending
  // with the path separator as we lose the last path element in the way the
  // files are referenced.
  if(request.uri.back() != '/')
    throw InvalidDirectoryPathException("Path to a directory must end with a forward slash");
  std::string relative_path = UriUtils::uri_decode(uri, false);
  std::string full_path = document_root + relative_path;
  auto dir_list = FileUtils::get_directory(full_path);
  response.content
    << "    <h1>Index of /"
    << relative_path
    << "</h1>\n    <hr/>\n<pre>\n";
  if (!dir_list.empty()) {

    // std::cout << "application_prefix_url: \"" << get_uri_prefix() << "\"\n";
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
#ifdef ENABLE_DIRECTORY_LISTING
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
    } catch (const InvalidDirectoryPathException& e) {
      redirect(request, response, request.uri + '/');
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
#ifndef ENABLE_DIRECTORY_LISTING
    // Reject if it's a directory, but the path doesn't end with a forward
    // slash.
    if(request.uri.back() != '/')
      throw InvalidDirectoryPathException(
          "Path to a directory must end with a forward slash");
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
#endif // ENABLE_STATIC_FILES

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
    os << "    <title>" << get_page_title() << "</title>\n";
}

const std::string BaseRequestHandler::css_stylesheet =
  ".footer {padding: 10px; padding-top: 0.5rem!important; padding-bottom: 1.5rem!important;} .text-bg-secondary {color: #fff; background-color: RGBA(108,117,125);} .px-2 {padding-left: .5rem; padding-right: .5rem;} .py-2 {padding-top: .5rem; padding-bottom: .5rem;} .mt-5 {margin-top: 3rem;} .small {font-size: .875em;} .link-light {color: RGBA(249,250,251);}";

void BaseRequestHandler::append_head_content(std::ostream& os) const
{
  os <<
    "    <style>" << BaseRequestHandler::css_stylesheet << "</style>\n";
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
  std::string package_name = PACKAGE;
  std::transform(package_name.begin(),
                 package_name.end(),
                 package_name.begin(),
                 ::toupper);
  os <<
    "    <div class=\"footer px-2 py-2 mt-5 text-bg-secondary\">\n"
    "      <div class=\"small\" style=\"float: left\">" << package_name << " " << VERSION << "</div>\n"
    "      <div class=\"small\" style=\"float: right\"><a href=\"" << TRIP_SOURCE_URL << "\" class=\"link-light\" target=\"_blank\">" << "source code" << "</a></div>\n"
    "    </div>\n";
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

UserMessage BaseRequestHandler::get_message(std::string code) const
{
  UserMessage retval;
  for (const auto& m : messages) {
    if (m.code == code) {
      retval = m;
      break;
    }
  }
  return retval;
}

void BaseRequestHandler::append_messages_as_html(std::ostream& os) const
{
  if (messages.size() == 0)
    return;
  for (const auto &m : messages) {
    os
      << "  <div class=\"";
    switch (m.type) {
      case UserMessage::info:
        os << "alert alert-info";
        break;
      case UserMessage::warn:
        os << "alert alert-warning";
        break;
      case UserMessage::error:
        os << "alert alert-danger";
        break;
      case UserMessage::success:
        os << "alert alert-success";
        break;
      case UserMessage::light:
        os << "alert alert-light";
        break;
      case UserMessage::dark:
        os << "alert alert-dark";
        break;
      case UserMessage::primary:
        os << "alert alert-primary";
        break;
      case UserMessage::secondary:
        os << "alert alert-secondary";
        break;
    }
    os
      << "\" role=\"alert\">\n"
      << m.message << "\n</div>\n";
  }
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
    << "<div class=\"container-fluid\">\n"
    << "<h1>Error&nbsp;" << response.status_code << "&mdash;" << status_message << "</h1>\n"
    << "</div>\n";
  append_footer_content(response.content);
  append_pre_body_end(response.content);
  append_body_end(response.content);
  append_html_end(response.content);
  set_content_headers(response);
}

void BaseRequestHandler::handle_request_failure(
    const HTTPServerRequest& request,
    HTTPServerResponse& response,
    HTTPStatus status_code)
{
  if (GetOptions::verbose_flag)
    std::cout << "Request failed.  Responding with HTTP status code: " << status_code << '\n';
  (void)request; // unused
  response.content.clear();
  response.content.str("");
  response.status_code = status_code;
  create_full_html_page_for_standard_response(response);
}

void BaseRequestHandler::handle_forbidden_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  (void)request; // unused
  handle_request_failure(request, response, HTTPStatus::forbidden);
}

void BaseRequestHandler::handle_bad_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  (void)request; // unused
  handle_request_failure(request, response, HTTPStatus::bad_request);
}

void BaseRequestHandler::append_element_disabled_flag(std::ostream &os,
                                                      bool append)
{
  if (append)
    os << " disabled";
}

void BaseRequestHandler::append_element_selected_flag(std::ostream &os,
                                                      bool append)
{
  if (append)
    os << " selected";
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
  if (response.status_code != HTTPStatus::found &&
      response.status_code != HTTPStatus::forbidden &&
      response.status_code != HTTPStatus::bad_request &&
      response.status_code != HTTPStatus::internal_server_error
    ) {
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
    } catch (const ForbiddenException &e) {
      handle_forbidden_request(request, response);
    } catch (const BadRequestException &e) {
      handle_bad_request(request, response);
    } catch (const std::invalid_argument &e) {
      handle_bad_request(request, response);
    } catch (const std::out_of_range &e) {
      handle_bad_request(request, response);
    } catch (const PayloadTooLarge &e) {
      response.content.clear();
      response.content.str("");
      response.status_code = HTTPStatus::payload_too_large;
      create_full_html_page_for_standard_response(response);
#ifdef HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H
    } catch (const pqxx::broken_connection &e) {
      // Let worker handle the error
      throw;
#endif
    } catch (const std::exception &e) {
      std::ostringstream os;
      os << typeid(e).name()
         << " exception occurred handling request: "
         << e.what();
      std::cerr << os.str() << '\n';
      syslog(LOG_ERR, "%s", os.str().c_str());
      response.content.clear();
      response.content.str("");
      response.status_code = HTTPStatus::internal_server_error;
      create_full_html_page_for_standard_response(response);
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
    "            <input id=\"input-email\" type=\"email\" autocomplete=\"username\" class=\"form-control\" placeholder=\"" <<
#ifdef HAVE_BOOST_LOCALE
    translate("Username")
#else
    "Username"
#endif
     << "\" name=\"email\" size=\"25\" >\n"
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
    "            <input id=\"input-password\" type=\"password\" autocomplete=\"current-password\" class=\"form-control\" placeholder=\"" <<
#ifdef HAVE_BOOST_LOCALE
    translate("Password")
#else
    "Password"
#endif
     << "\" name=\"password\" size=\"25\" >\n"
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
  try {
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
  } catch (const ForbiddenException &e) {
    handle_forbidden_request(request, response);
  } catch (const BadRequestException &e) {
    handle_bad_request(request, response);
  } catch (const std::invalid_argument &e) {
    handle_bad_request(request, response);
  } catch (const std::out_of_range &e) {
    handle_bad_request(request, response);
  } catch (const std::exception &e) {
    if (GetOptions::verbose_flag)
      std::cout << "Exception occurred handling request: " << e.what() << '\n';
    handle_request_failure(request, response,
                           HTTPStatus::internal_server_error);
  }
}

void AuthenticatedRequestHandler::do_handle_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response)
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
    fdsd::web::HTTPServerResponse& response)
{
  if (request.method == HTTPMethod::get) {
    std::string redirect_uri = get_redirect_uri(request);

    // Handle invalid looking redirects
    if (!redirect_uri.empty() &&
        redirect_uri.find(get_uri_prefix()) == 0 &&
        redirect_uri != get_login_uri()) {
      // if (GetOptions::verbose_flag)
      //   std::cout << "Setting redirect cookie to: \"" << redirect_uri << "\"\n";
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
// #ifdef HAVE_BOOST_LOCALE
//         // Message showing the server administrator the URL a browser is being redirected to after login
//         std::cout << "Login redirect cookie: " << redirect_url << '\n';
// #else
//         std::cout << "Login redirect cookie: " << redirect_url << '\n';
// #endif
      }
      // Immediately expire the cookie
      response.set_cookie(get_login_redirect_cookie_name(), request.uri, 0);
      if (redirect_url.empty() || redirect_url ==
          get_login_uri() || redirect_url.front() != '/') {
        std::string default_uri = get_default_uri();
        // if (GetOptions::verbose_flag)
        //   std::cout << "Redirect URL was empty, redirecting to: "
        //             << default_uri << '\n';

        redirect_url = default_uri;
      }
      if (GetOptions::verbose_flag) {
// #ifdef HAVE_BOOST_LOCALE
//         // Message showing the server administrator the URL a browser is being redirected to
//         std::cout << format(translate("Redirecting to: {1}")) % redirect_url << '\n';
// #else
//         std::cout << "Redirecting to: " << redirect_url << '\n';
// #endif
      }
      redirect(request, response, redirect_url);
    } else {
      // if (GetOptions::verbose_flag)
      //   std::cout << "Login failure, bad credentials\n";
      append_login_body(response.content, false);
      response.status_code = HTTPStatus::unauthorized;
    }
  } else {
    // if (GetOptions::verbose_flag)
    //   std::cout << "Login bad request\n";
    response.generate_standard_response(HTTPStatus::bad_request);
  }
}

void HTTPLogoutRequestHandler::do_handle_request(
    const HTTPServerRequest& request,
    fdsd::web::HTTPServerResponse& response)
{
  std::string session_id = request.get_cookie(get_session_id_cookie_name());
  get_session_manager()->invalidate_session(session_id);
  // Immediately expire the cookies
  // if (GetOptions::verbose_flag)
  //   std::cout << "Logout\n";
  // std::cout << "Expiring existing login redirect cookie\n";
  response.set_cookie(get_login_redirect_cookie_name(), "", 0);
  // if (GetOptions::verbose_flag)
  //   std::cout << "Logout redirecting to default uri \""
  //             << get_default_uri() << "\"\n";
  redirect(request, response, get_default_uri());
}

Logger HTTPNotFoundRequestHandler::logger("HTTPNotFoundRequestHandler",
                                   std::clog,
                                   fdsd::utils::Logger::info);

void HTTPNotFoundRequestHandler::do_handle_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  (void)request; // unused
  response.status_code = HTTPStatus::not_found;
  response.generate_standard_response(HTTPStatus::not_found);
}

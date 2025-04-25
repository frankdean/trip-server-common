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
#include "logger.hpp"
#include "pg_pool.hpp"
#include "session.hpp"
#include "application.hpp"
#include "configuration.hpp"
#include "http_request.hpp"
#include "http_request_handler.hpp"
#include "http_request_factory.hpp"
#include "http_response.hpp"
#include "session.hpp"
#include "get_options.hpp"
#include <cstdlib>
#include <memory>
#include <iostream>
#include <syslog.h>

using namespace fdsd::web;
using namespace fdsd::utils;

struct ExampleGetOptions : public GetOptions {
#ifdef HAVE_GETOPT_H
  static const char short_opts[];
  static struct option long_options[];
#endif // HAVE_GETOPT_H
  static int expire_sessions;
  static int example_flag;
  std::string example_value;
  void confirm_force();
  virtual void usage(std::ostream& os) const override;
#ifdef HAVE_GETOPT_H
  virtual bool handle_option(int c) override;
  virtual const struct option* get_long_options() const override;
  virtual const char* get_short_options() const override;
#endif // HAVE_GETOPT_H
};

int ExampleGetOptions::expire_sessions = 0;
int ExampleGetOptions::example_flag = 0;

#ifdef HAVE_GETOPT_H

#ifdef ENABLE_STATIC_FILES
const char ExampleGetOptions::short_opts[] = "hs:p:r:c:vVft:e";
#else
const char ExampleGetOptions::short_opts[] = "hs:p:c:vVft:e";
#endif

struct option ExampleGetOptions::long_options[] = {
  // Name                 Argument           Flag              Shortname
  {"help",                no_argument,       NULL,             'h'},
  {"listen",              required_argument, NULL,             's'},
  {"port",                required_argument, NULL,             'p'},
#ifdef ENABLE_STATIC_FILES
  {"root",                required_argument, NULL,             'r'},
#endif
  {"config-file",         required_argument, NULL,             'c'},
  {"verbose",             no_argument,       &GetOptions::verbose_flag,    1},
  {"version",             no_argument,       NULL,             'v'},
  {"test-example",        required_argument, NULL,             't'},
  {"flag-example",        no_argument,       &example_flag,    1},
  {"expire_sessions",     no_argument,       &expire_sessions, 1},
  {NULL, 0, NULL, 0}
};

/**
 * \return true if the application should continue, false if the application
 * should exit.
 */
bool ExampleGetOptions::handle_option(int c)
{
  // std::cout << "ExampleGetOptions::handle option: " << (char) c << '\n';
  switch (c) {
    case 'e':
      expire_sessions = 1;
      break;
    case 't':
      // Do something for --test-example option
      break;
    default:
      return GetOptions::handle_option(c);
  } // switch
  return true;
}
#endif // HAVE_GETOPT_H

void ExampleGetOptions::usage(std::ostream& os) const
{
#ifdef HAVE_GETOPT_H
  os
    << "Usage:\n"
    << "  " << program_name << " [OPTIONS]\n\n"
    << "Options:\n"
    << "  -h, --help\t\t\t\tshow this help, then exit\n"
    << "  -v, --version\t\t\t\tshow version information, then exit\n"
    << "  -s, --listen=ADDRESS\t\t\tlisten address, e.g. 0.0.0.0\n"
    << "  -p, --port=PORT\t\t\tport number, e.g. 8080\n"
#ifdef ENABLE_STATIC_FILES
    << "  -r, --root=DIRECTORY\t\t\tdocument root directory\n"
#endif
    << "  -c, --config-file=FILENAME\t\tconfiguration file name\n"
    << "  -V, --verbose\t\t\t\tverbose output\n"
    << "  -t, --test-example\t\t\ttest value\n"
    << "  -f, --flag-example\t\t\tset exampleflag\n";
#else
  os
    << "Usage: " << program_name << " <address> <port>"
#ifdef ENABLE_STATIC_FILES
    << " <doc_root>"
#endif
    << '\n'
    << "Example:\n"
    << "    http-server-sync 0.0.0.0 8080"
#ifdef ENABLE_STATIC_FILES
    << " ."
#endif
    << '\n';
#endif // HAVE_GETOPT_H
}

#ifdef HAVE_GETOPT_H
const char* ExampleGetOptions::get_short_options() const
{
  return short_opts;
}

const struct option* ExampleGetOptions::get_long_options() const
{
  return long_options;
}
#endif // HAVE_GETOPT_H


class ExampleCssHandler : public CssRequestHandler {
protected:
  virtual void append_stylesheet_content(
      const fdsd::web::HTTPServerRequest& request,
      fdsd::web::HTTPServerResponse& response) const override;
public:
  static const std::string example_css_url;
  ExampleCssHandler(std::string uri_prefix) : CssRequestHandler(uri_prefix) {}
  virtual std::string get_handler_name() const override {
    return "ExampleCssHandler";
  }
  virtual std::unique_ptr<fdsd::web::BaseRequestHandler>
      new_instance() const override {
    return std::unique_ptr<ExampleCssHandler>(
        new ExampleCssHandler(get_uri_prefix()));
  }
  virtual bool can_handle(
      const fdsd::web::HTTPServerRequest& request) const override {
    return request.uri == get_uri_prefix() + example_css_url;
  }
};

const std::string ExampleCssHandler::example_css_url = "/example.css";

void ExampleCssHandler::append_stylesheet_content(
    const fdsd::web::HTTPServerRequest& request,
    fdsd::web::HTTPServerResponse& response) const
{
  response.set_header("Last-Modified", "Sun, 31 Jul 2022 14:53:24 GMT");
  response.content << BaseRequestHandler::css_stylesheet;
}

class ExampleServerResponse : public HTTPServerResponse {
  std::string uri_prefix;
protected:
  virtual std::string get_uri_prefix() const {
    return uri_prefix;
  }
public:
  ExampleServerResponse(std::string uri_prefix) :
    HTTPServerResponse(),
    uri_prefix(uri_prefix) {}
};

class ExampleRequestHandler : public HTTPRequestHandler
{
protected:
  virtual void append_head_content(std::ostream& os) const override;
public:
  static const std::string default_url;
  static const std::string success_url;
  ExampleRequestHandler(std::string uri_prefix) :
    HTTPRequestHandler(uri_prefix) {
    set_page_title("Hello World!");
  }
  virtual std::string get_handler_name() const override {
    return "ExampleRequestHandler";
  }
  virtual std::unique_ptr<BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ExampleRequestHandler>(
        new ExampleRequestHandler(get_uri_prefix()));
  }
  virtual bool can_handle(const HTTPServerRequest& request) const override {
    std::string prefix = get_uri_prefix();
    // Act as default handler for the application root URL or URLs that do not
    // start with the root.
    return request.uri == prefix ||
      (prefix.length() > 1 && request.uri.find(prefix) != 0);
  }
  virtual void do_handle_request(
        const HTTPServerRequest& request,
        HTTPServerResponse& response) override;
};

const std::string ExampleRequestHandler::default_url = "";
const std::string ExampleRequestHandler::success_url = "/success";

void ExampleRequestHandler::append_head_content(std::ostream& os) const
{
  os <<
    "    <link rel=\"stylesheet\" href=\""
          << get_uri_prefix()
          << ExampleCssHandler::example_css_url
          << "\"/>\n";
}

void ExampleRequestHandler::do_handle_request(
    const HTTPServerRequest& request,
    HTTPServerResponse& response)
{
  response.content
    <<
    "<h1>Hello World!</h1>\n"
    "<p>Click <a href=\""
    << get_uri_prefix() << ExampleRequestHandler::success_url
    << "\">here</a> to test authentication</p>\n";
}

class ExampleLoginRequestHandler : public fdsd::web::HTTPLoginRequestHandler {
protected:
  virtual std::string get_page_title() const override {
    return "Login";
  }
  virtual bool validate_password(const std::string email,
                                 const std::string password) const override;
  virtual std::string get_user_id_by_email(
      const std::string email) const override;
  virtual std::string get_default_uri() const override {
    return get_uri_prefix();
  }
  virtual std::string get_login_uri() const override {
    return get_uri_prefix() + ExampleLoginRequestHandler::login_url;
  }
  virtual std::string get_session_id_cookie_name() const override {
    return ExampleLoginRequestHandler::session_id_cookie_name;
  }
  virtual SessionManager* get_session_manager() const override {
    return SessionManager::get_session_manager();
  }
  virtual std::string get_login_redirect_cookie_name() const override {
    return ExampleLoginRequestHandler::login_redirect_cookie_name;
  }
public:
  static const std::string login_url;
  static const std::string login_redirect_cookie_name;
  static const std::string session_id_cookie_name;
  static const std::string test_user_id;
  static const std::string test_username;
  static const std::string test_password;
  ExampleLoginRequestHandler(std::string uri_prefix) :
    fdsd::web::HTTPLoginRequestHandler(uri_prefix) {}
  virtual std::unique_ptr<BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ExampleLoginRequestHandler>(
        new ExampleLoginRequestHandler(get_uri_prefix()));
  }
  virtual bool can_handle(const HTTPServerRequest& request) const override {
    return request.uri == get_uri_prefix() +
      ExampleLoginRequestHandler::login_url;
  }
  virtual std::string get_handler_name() const override {
    return "ExampleLoginRequestHandler";
  }
};

const std::string ExampleLoginRequestHandler::login_url =
  "/login";
const std::string ExampleLoginRequestHandler::login_redirect_cookie_name =
  "example-login-redirect";
const std::string ExampleLoginRequestHandler::session_id_cookie_name =
  "EXAMPLE_SESSION_ID";
const std::string ExampleLoginRequestHandler::test_user_id = "10e8d704-cf88-4bd0-995a-fcf4a341da9f";
const std::string ExampleLoginRequestHandler::test_username = "example@example.test";
const std::string ExampleLoginRequestHandler::test_password = "bavmubmoj";

bool ExampleLoginRequestHandler::validate_password(
    const std::string email,
    const std::string password) const
{
  return (email == test_username
          && password == test_password);
}

std::string ExampleLoginRequestHandler::get_user_id_by_email(
    std::string email) const
{
  if (email == test_username)
    return test_user_id;
  return "";
}

class ExampleLogoutRequestHandler : public fdsd::web::HTTPLogoutRequestHandler {
protected:
  virtual std::string get_login_uri() const override {
    return get_uri_prefix() + ExampleLoginRequestHandler::login_url;
  }
  virtual std::string get_default_uri() const override {
    return get_uri_prefix();
  }
  virtual std::string get_session_id_cookie_name() const override {
    return ExampleLoginRequestHandler::session_id_cookie_name;
  }
  virtual SessionManager* get_session_manager() const override {
    return SessionManager::get_session_manager();
  }
  virtual std::string get_login_redirect_cookie_name() const override {
    return ExampleLoginRequestHandler::login_redirect_cookie_name;
  }
  virtual std::string get_page_title() const override {
    return "Logout";
  }
  virtual std::unique_ptr<BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ExampleLogoutRequestHandler>(
        new ExampleLogoutRequestHandler(get_uri_prefix()));
  }
  virtual bool can_handle(const HTTPServerRequest& request) const override {
    return request.uri == get_uri_prefix() +
      ExampleLogoutRequestHandler::logout_url;
  }
  virtual std::string get_handler_name() const override {
    return "ExampleLogoutRequestHandler";
  }
public:
  static const std::string logout_url;
  ExampleLogoutRequestHandler(std::string uri_prefix) :
    fdsd::web::HTTPLogoutRequestHandler(uri_prefix) {}
};

const std::string ExampleLogoutRequestHandler::logout_url =
  "/logout";

class ExampleAuthenticatedRequestHandler : public AuthenticatedRequestHandler {
protected:
  virtual std::string get_page_title() const override {
    return "Login Success!";
  }
  virtual std::string get_login_uri() const override {
    return get_uri_prefix() + ExampleLoginRequestHandler::login_url;
  }
  virtual std::string get_session_id_cookie_name() const override {
    return ExampleLoginRequestHandler::session_id_cookie_name;
  }
  virtual SessionManager* get_session_manager() const override {
    return SessionManager::get_session_manager();
  }
  virtual std::string get_login_redirect_cookie_name() const override {
    return ExampleLoginRequestHandler::login_redirect_cookie_name;
  }
  virtual std::string get_default_uri() const override {
    return get_uri_prefix();
  }
  virtual void do_preview_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) override {}
  virtual void handle_authenticated_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) override;
public:
  ExampleAuthenticatedRequestHandler(std::string uri_prefix) :
    AuthenticatedRequestHandler(uri_prefix) {}
  virtual std::string get_handler_name() const override {
    return "ExampleAuthenticatedRequestHandler";
  }
  virtual bool can_handle(const HTTPServerRequest& request) const override {
    return request.uri == get_uri_prefix() + ExampleRequestHandler::success_url;
  }
  virtual std::unique_ptr<BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<ExampleAuthenticatedRequestHandler>(
        new ExampleAuthenticatedRequestHandler(get_uri_prefix()));
  }
};

void ExampleAuthenticatedRequestHandler::handle_authenticated_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response)
{
  response.content
    << "<h1>Successfully logged in!</h1>\n"
    "<p>Click <a href=\""
    + ExampleLogoutRequestHandler::logout_url
    + "\">here</a> to logout</p>";
}

class ExampleNotFoundHandler : public HTTPNotFoundRequestHandler {
protected:
  virtual std::string get_default_uri() const override {
    return get_uri_prefix();
  }
  virtual std::string get_handler_name() const override {
    return "ExampleNotFoundHandler";
  }
  virtual void do_handle_request(
      const fdsd::web::HTTPServerRequest& request,
      fdsd::web::HTTPServerResponse& response) override {
    // If the url is effectively a root url for the application, redirect.
    if (compare_request_regex(request.uri, "($|/$)") ||
        request.uri.empty() ||
        request.uri == "/") {
      redirect(request, response, get_default_uri());
    } else {
      HTTPNotFoundRequestHandler::do_handle_request(request, response);
    }
  }
public:
  ExampleNotFoundHandler(std::string uri_prefix) :
    HTTPNotFoundRequestHandler(uri_prefix) {}
};

class ExampleRequestFactory : public HTTPRequestFactory {
private:
  std::string root_directory;
public:
  ExampleRequestFactory(std::string root_directory, std::string uri_prefix);
  virtual std::unique_ptr<fdsd::web::HTTPServerResponse>
      create_response_object() const override;
protected:
  virtual std::string get_session_id_cookie_name() const override;
  virtual std::string get_user_id(std::string session_id) const override;
  virtual bool is_login_uri(std::string uri) const override;
  virtual std::unique_ptr<HTTPRequestHandler> get_login_handler() const override;
  virtual bool is_logout_uri(std::string uri) const override;
  virtual std::unique_ptr<HTTPRequestHandler> get_logout_handler() const override;
  virtual bool is_application_prefix_uri(std::string uri) const override;
  virtual std::unique_ptr<HTTPRequestHandler> get_not_found_handler() const override;
  virtual bool is_valid_session(std::string session_id, std::string user_id) const override;
};

std::unique_ptr<HTTPServerResponse>
    ExampleRequestFactory::create_response_object() const
{
  // return std::unique_ptr<HTTPServerResponse>(new HTTPServerResponse);
  return std::unique_ptr<ExampleServerResponse>(
      new ExampleServerResponse(get_uri_prefix()));
}

ExampleRequestFactory::ExampleRequestFactory(
    std::string root_directory, std::string uri_prefix)
  : HTTPRequestFactory(uri_prefix),
    root_directory(root_directory)
{
  pre_login_handlers.push_back(
      std::make_shared<ExampleCssHandler>(
          ExampleCssHandler(get_uri_prefix())));
  pre_login_handlers.push_back(
      std::make_shared<ExampleLogoutRequestHandler>(
          ExampleLogoutRequestHandler(get_uri_prefix())));
  pre_login_handlers.push_back(
      std::make_shared<ExampleRequestHandler>(
          ExampleRequestHandler(get_uri_prefix())));
#ifdef ENABLE_STATIC_FILES
  pre_login_handlers.push_back(
      std::make_shared<FileRequestHandler>(
          FileRequestHandler(get_uri_prefix(),
                             root_directory)));
#endif
  post_login_handlers.push_back(
      std::make_shared<ExampleAuthenticatedRequestHandler>(
          ExampleAuthenticatedRequestHandler(get_uri_prefix())));
}

std::string ExampleRequestFactory::get_session_id_cookie_name() const
{
  return ExampleLoginRequestHandler::session_id_cookie_name;
}

bool ExampleRequestFactory::is_login_uri(std::string uri) const
{
  return uri.find(ExampleLoginRequestHandler::login_url) != std::string::npos;
}

std::unique_ptr<HTTPRequestHandler>
    ExampleRequestFactory::get_login_handler() const
{
  return std::unique_ptr<HTTPRequestHandler>(
      new ExampleLoginRequestHandler(get_uri_prefix()));
}

bool ExampleRequestFactory::is_logout_uri(std::string uri) const
{
  return uri.find(ExampleLogoutRequestHandler::logout_url) != std::string::npos;
}

std::unique_ptr<HTTPRequestHandler>
    ExampleRequestFactory::get_logout_handler() const
{
  return std::unique_ptr<HTTPRequestHandler>(
      new ExampleLogoutRequestHandler(get_uri_prefix()));
}

std::unique_ptr<HTTPRequestHandler>
    ExampleRequestFactory::get_not_found_handler() const
{
  return std::unique_ptr<HTTPRequestHandler>(
      new ExampleNotFoundHandler(get_uri_prefix()));
}

std::string ExampleRequestFactory::get_user_id(std::string session_id) const
{
  if (!session_id.empty())
    return SessionManager::get_session_manager()->get_session_user_id(session_id);

  return "";
}

bool ExampleRequestFactory::is_application_prefix_uri(std::string uri) const
{
  return !uri.empty() && uri.find(get_uri_prefix()) == 0;
}

bool ExampleRequestFactory::is_valid_session(std::string session_id,
                                            std::string user_id) const
{
  std::optional<std::string> user =
    SessionManager::get_session_manager()->get_user_id_for_session(session_id);
  return user.has_value() && user.value() == user_id;
}

class ExampleApplication : public Application {
private:
  std::string config_filename;
  std::string application_prefix_url;
  std::string document_root;
protected:
  virtual std::shared_ptr<HTTPRequestFactory> get_request_factory() const override;
  std::string get_config_filename() const { return config_filename; }
public:
  ExampleApplication(std::string listen_address,
                     std::string port,
                     std::string application_prefix_url,
                     std::string locale = "");
  std::string get_db_connect_string() {
    return "";
  }
  void initialize_user_sessions(bool expire_sessions);
  void set_config_filename(std::string const config_filename) {
    this->config_filename = config_filename;
  }
  void set_document_root(std::string directory) { document_root = directory; }
  std::string get_document_root() const { return document_root; }
};

ExampleApplication::ExampleApplication(std::string listen_address,
                                       std::string port,
                                       std::string application_prefix_url,
                                       std::string locale) :
  Application(
    listen_address,
    port),
  config_filename(),
  document_root("")
{
  if (application_prefix_url.empty()) {
    application_prefix_url = "/";
  } else if (application_prefix_url.length() > 1 &&
             application_prefix_url.substr(
                 application_prefix_url.length() -1, 1) == "/") {
    application_prefix_url.erase(application_prefix_url.length() -1, 1);
  }
  this->application_prefix_url = application_prefix_url;
}

std::shared_ptr<HTTPRequestFactory>
    ExampleApplication::get_request_factory() const
{
  ExampleRequestFactory factory(get_document_root(), application_prefix_url);
  return std::make_shared<ExampleRequestFactory>(factory);
}

void ExampleApplication::initialize_user_sessions(bool expire_sessions)
{
  if (expire_sessions) {
    SessionManager::get_session_manager()->clear_sessions();
    SessionManager::get_session_manager()->persist_sessions();
  } else {
    SessionManager::get_session_manager()->load_sessions();
  }
}

int main (int argc, char *argv[])
{
  openlog(PACKAGE_NAME, LOG_PID, LOG_USER);
  Logger logger("example", std::clog, Logger::info);
  const std::string default_application_uri_prefix = "/example";
  try {
    ExampleGetOptions options;
    try {
      if (!options.init(argc, argv))
        return EXIT_SUCCESS;
    } catch (const GetOptions::UnexpectedArgumentException & e) {
      return EXIT_FAILURE;
    }

    ExampleApplication application(
      options.listen_address,
      options.port,
      default_application_uri_prefix);

    if (!options.config_filename.empty())
      application.set_config_filename(options.config_filename);

    application.read_config_file(options.config_filename);
#ifdef ENABLE_STATIC_FILES
    if (!options.doc_root.empty()) {
      if (options.doc_root.substr(options.doc_root.length() -1, 1) != "/")
        options.doc_root.append("/");
      application.set_document_root(options.doc_root);
    }
#endif // ENABLE_STATIC_FILES

#ifdef HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H
    // Initialize the global database pool and user session managers
    auto pool_manager = std::make_shared<PgPoolManager>(
        application.get_db_connect_string(),
        std::stoi(
            application.get_config_value(Configuration::pg_pool_size_key,
                                         "24"))
      );
    // ExamplePgDao::set_pool_manager(&pool_manager);
#endif
    SessionManager session_manager;
    SessionManager::set_session_manager(&session_manager);
    application.initialize_user_sessions(options.expire_sessions);

    application.initialize_workers(
        std::stoi(
            application.get_config_value(Configuration::worker_count_key,
                                         "20"))
#ifdef HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H
        , pool_manager
#endif
      );

    logger << Logger::info
           << PACKAGE << " version " << VERSION
           << " listening at http://"
           << options.listen_address << ':' << options.port
           << default_application_uri_prefix
           << Logger::endl;
#ifdef ENABLE_STATIC_FILES
    logger
      << Logger::info
      << "This application has been built with the option to serve "
      "static files from the \"" << options.doc_root
      << "\" directory."
#ifdef ENABLE_DIRECTORY_LISTING
      << "\nAdditionally, listing directories under the document root is "
      "enabled."
#endif
      << Logger::endl;
#endif
    application.run();
  } catch (const std::exception& e) {
    logger << Logger::alert
           << "Fatal: " << e.what() << Logger::endl;
    return EXIT_FAILURE;
  }

  logger << Logger::info << "Bye!" << Logger::endl;
  return EXIT_SUCCESS;
}

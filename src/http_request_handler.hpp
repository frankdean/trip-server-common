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
#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP

#include "http_request.hpp"
#include "logger.hpp"
#include <memory>
#include <ostream>
#include <regex>
#include <string>

namespace fdsd
{
namespace web
{

class HTTPServerRequest;
class HTTPServerResponse;
class SessionManager;
enum HTTPStatus : int;

class BadRequestException : public std::exception {
  std::string message;
public:
  BadRequestException(std::string message) {
    this->message = message;
  }
  virtual const char* what() const throw() override {
    return message.c_str();
  }
};

struct UserMessage {
  /// The type of the error message.
  enum Type {
    info,
    warn,
    error, // danger in Bootstrap parlance
    success,
    light,
    dark,
    primary,
    secondary
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

class BaseRequestHandler {
private:
  /// An optional prefix to all the application URLs.
  std::string uri_prefix;
  std::string page_title;
  std::string html_lang;
protected:
  std::vector<UserMessage> messages;
  virtual void redirect(const HTTPServerRequest& request,
                        HTTPServerResponse& response,
                        std::string location) const;
  /// Escapes string with HTML entities
  std::string x(std::string s) const;
  virtual std::string get_redirect_uri(const HTTPServerRequest& request) const;
  virtual void append_doc_type(std::ostream& os) const;
  virtual void append_html_start(std::ostream& os) const;
  virtual void append_head_start(std::ostream& os) const;
  virtual void append_head_section(std::ostream& os) const;
  virtual void append_head_title_section(std::ostream& os) const;
  virtual void append_head_content(std::ostream& os) const;
  virtual void append_head_end(std::ostream& os) const;
  virtual void append_body_start(std::ostream& os) const;
  virtual void append_header_content(std::ostream& os) const {}
  virtual void append_footer_content(std::ostream& os) const;
  virtual void append_pre_body_end(std::ostream& os) const {}
  virtual void append_body_end(std::ostream& os) const;
  virtual void append_html_end(std::ostream& os) const;
  virtual void set_content_headers(HTTPServerResponse& response) const;
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
  virtual void append_messages_as_html(std::ostream& os) const;
  virtual void handle_bad_request(const HTTPServerRequest& request,
                                  HTTPServerResponse& response);
  /**
   * Checks whether the passed URL matches the requested wanted_postfix.
   *
   * \param full_request_url the full request URL including the application
   * prefix.
   *
   * \param wanted_postfix the portion of the URL to compare.
   *
   * \return true if the passed full URL portion starts with the passed
   * wanted_postfix, ignoring the application prefix URL.
   */
  bool compare_request_url(std::string full_request_url,
                           std::string wanted_postfix) const {
    const std::string wanted_url = get_uri_prefix() + wanted_postfix;
    return !full_request_url.empty() &&
      full_request_url.compare(0, wanted_url.length(), wanted_url) == 0;
  }
  /**
   * Checks whether the passed URL matches the passed regular expression.
   *
   * \param full_request_url the full request URL including the application
   * prefix.
   *
   * \param wanted_postfix_regex the portion of the URL to compare.
   *
   * \return true if the passed full URL portion starts with the passed
   * wanted_postfix_regex, ignoring the application prefix URL.
   */
  bool compare_request_regex(std::string full_request_url,
                           std::string wanted_postfix_regex) const {
    return std::regex_match(
        full_request_url,
        std::regex(get_uri_prefix() + wanted_postfix_regex));
  }
  void append_element_disabled_flag(std::ostream &os,
                                    bool append);
  void append_element_selected_flag(std::ostream &os,
                                    bool append);

  template <typename T>
  void append_value(std::ostream &os, bool append, const T& value) {
    if (append)
      os << value;
  }

public:
  /// \param uri_prefix the prefix to use for the entire application.
  /// Defaults to an empty string.
  BaseRequestHandler(std::string uri_prefix) :
    uri_prefix(uri_prefix),
    page_title(),
    html_lang("en-GB"),
    messages() {}
  virtual ~BaseRequestHandler() {}
  std::string get_uri_prefix() const { return uri_prefix; }
  virtual std::string get_page_title() const { return page_title; }
  void set_page_title(std::string title) { page_title = title; }
  virtual std::string get_html_lang() const { return html_lang; }
  void set_html_lang(std::string html_lang) { this->html_lang = html_lang; }
  void pad_left(std::string &s,
                std::string::size_type length,
                char c = ' ') const;
  void pad_right(std::string &s,
                 std::string::size_type length,
                 char c = ' ') const;
  virtual std::string get_handler_name() const = 0;
  virtual std::unique_ptr<BaseRequestHandler> new_instance() const = 0;
  virtual bool can_handle(const HTTPServerRequest& request) const = 0;
  virtual void handle_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) = 0;
  std::string get_mime_type(std::string extension) const;
  void create_full_html_page_for_standard_response(
      HTTPServerResponse& response);
};

class CssRequestHandler : public BaseRequestHandler {
protected:
  virtual void append_stylesheet_content(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) const = 0;
  virtual void set_content_headers(HTTPServerResponse& response) const override;
public:
  CssRequestHandler(std::string uri_prefix) : BaseRequestHandler(uri_prefix) {}
  virtual ~CssRequestHandler() {}
  virtual void handle_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) override;
};

#ifdef ALLOW_STATIC_FILES
class FileRequestHandler : public BaseRequestHandler {
private:
  std::string document_root;
protected:
  virtual void append_body_content(
      const HTTPServerRequest& request,
      HTTPServerResponse& response);
  virtual void set_content_headers(HTTPServerResponse& response) const override;
#ifdef ALLOW_DIRECTORY_LISTING
  virtual void handle_directory(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) const;
#endif
public:
  FileRequestHandler(std::string uri_prefix,
                     std::string document_root)
    : BaseRequestHandler(uri_prefix),
      document_root(document_root) {}
  virtual ~FileRequestHandler() {}
  virtual void handle_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) override;
  virtual std::string get_handler_name() const override {
    return "FileRequestHandler";
  }
  virtual std::unique_ptr<BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<FileRequestHandler>(
        new FileRequestHandler(get_uri_prefix(), document_root));
  }
  virtual bool can_handle(const HTTPServerRequest& request) const override;
};
#endif

class HTTPRequestHandler : public BaseRequestHandler {
private:
  static fdsd::utils::Logger logger;
protected:
  virtual void do_handle_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) = 0;
  /// Called before any content has been written to the response stream.
  /// Can be used to capture data needed for the page title before
  /// Actually writing to the response stream.
  virtual void preview_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) {}
public:
  HTTPRequestHandler(std::string uri_prefix) : BaseRequestHandler(uri_prefix) {}
  virtual ~HTTPRequestHandler() {}
  virtual void handle_request(
        const HTTPServerRequest& request,
        HTTPServerResponse& response) override;
};

class SessionAwareRequestHandler : public HTTPRequestHandler {
protected:
  virtual std::string get_login_uri() const = 0;
  virtual std::string get_default_uri() const { return get_login_uri(); }
  virtual std::string get_session_id_cookie_name() const = 0;
  virtual SessionManager* get_session_manager() const = 0;
  virtual std::string get_login_redirect_cookie_name() const = 0;
public:
  SessionAwareRequestHandler(std::string uri_prefix) :
    HTTPRequestHandler(uri_prefix) {}
  virtual ~SessionAwareRequestHandler() {}
};

class BaseAuthenticatedRequestHandler : public SessionAwareRequestHandler {
protected:
  virtual void append_login_body(std::ostream& os, bool login_success) const;
public:
  BaseAuthenticatedRequestHandler(std::string uri_prefix) :
    SessionAwareRequestHandler(uri_prefix) {}
  virtual ~BaseAuthenticatedRequestHandler() {}
};

/// Ensures user is authorised before calling handler method.
class AuthenticatedRequestHandler : public BaseAuthenticatedRequestHandler {
  static fdsd::utils::Logger logger;
  std::string session_id;
  std::string user_id;
  virtual void preview_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) override;
protected:
  virtual void do_handle_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) override;
  virtual void handle_authenticated_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) = 0;
  virtual void do_preview_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) = 0;
  std::string get_user_id() const {
    return user_id;
  }
  std::string get_session_id() const {
    return session_id;
  }
public:
  AuthenticatedRequestHandler(std::string uri_prefix) :
    BaseAuthenticatedRequestHandler(uri_prefix),
    session_id(),
    user_id() {}
  virtual ~AuthenticatedRequestHandler() {}
};

class HTTPLoginRequestHandler : public BaseAuthenticatedRequestHandler {
private:
  static fdsd::utils::Logger logger;
protected:
  virtual bool validate_password(std::string email, std::string password) const = 0;
  virtual std::string get_user_id_by_email(std::string email) const = 0;
  virtual void do_handle_request(
      const fdsd::web::HTTPServerRequest& request,
      fdsd::web::HTTPServerResponse& response) override;
public:
  HTTPLoginRequestHandler(std::string uri_prefix) :
    BaseAuthenticatedRequestHandler(uri_prefix) {}
  virtual ~HTTPLoginRequestHandler() {}
};

class HTTPLogoutRequestHandler : public SessionAwareRequestHandler {
protected:
  virtual void do_handle_request(
      const fdsd::web::HTTPServerRequest& request,
      fdsd::web::HTTPServerResponse& response) override;
public:
  HTTPLogoutRequestHandler(std::string uri_prefix) :
    SessionAwareRequestHandler(uri_prefix) {}
  virtual ~HTTPLogoutRequestHandler() {}
//   virtual std::unique_ptr<HTTPRequestHandler> clone() const override {
//     return std::make_unique<HTTPLogoutRequestHandler>(*this);
//   }
};

class HTTPNotFoundRequestHandler : public HTTPRequestHandler {
private:
  static fdsd::utils::Logger logger;
protected:
  virtual std::string get_page_title() const override { return "Page not found"; }
  virtual std::string get_default_uri() const { return "/"; }
  virtual std::unique_ptr<BaseRequestHandler> new_instance() const override {
    return std::unique_ptr<HTTPNotFoundRequestHandler>(new HTTPNotFoundRequestHandler(get_uri_prefix()));
  }
  virtual void do_handle_request(
      const HTTPServerRequest& request,
      HTTPServerResponse& response) override;
public:
  HTTPNotFoundRequestHandler(std::string uri_prefix) :
    HTTPRequestHandler(uri_prefix) {}
  virtual ~HTTPNotFoundRequestHandler() {}
  virtual std::string get_handler_name() const override {
    return "HTTPNotFoundRequestHandler";
  }
  virtual bool can_handle(
      const fdsd::web::HTTPServerRequest& request) const override { return false; }
};

} // namespace web
} // namespace fdsd

#endif // HTTP_REQUEST_HANDLER_HPP

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
#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include "socket.hpp"
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

namespace fdsd {
namespace web {

bool istr_compare(const std::string& s1, const std::string& s2);

struct HttpOptions {
  virtual ~HttpOptions() {}
  std::map<std::string, std::string> headers;
  std::string protocol = "http:";
  std::string host = "localhost";
  std::string port = "80";
  std::string path = "/";
  std::string method = "GET";
  void add_header(std::string key, std::string value);
  virtual std::string to_string() const;
  inline friend std::ostream& operator<<
      (std::ostream& out, const HttpOptions& rhs) {
    return out << rhs.to_string();
  }
};

class HttpClient : public SocketUtils {
  std::map<std::string, std::string> headers;
  void parse_response(std::vector<char> &response);
protected:
  HttpOptions options;
public:
  HttpClient();
  HttpClient(HttpOptions options);
  std::vector<char> body;
  int status_code;
  void perform_request();
  /// Returns the value of the header with name, or an empty string if not
  /// found.
  std::string get_header(std::string name) const {
    for (const auto& i : headers) {
      if (istr_compare(i.first, name))
        return i.second;
    }
    return "";
  }
};

struct addrinfo_result_type {
  std::string ip_address;
  std::string port;
  int protocol;
  std::string to_string() const;
  inline friend std::ostream& operator<<
      (std::ostream& out, const addrinfo_result_type& rhs) {
    return out << rhs.to_string();
  }
};

/// Wrapper to getaddrinfo to ensure freeaddrinfo is called even after
/// exception etc.
class GetAddrInfo {
  std::string host;
  std::string port;
  struct addrinfo *infop = nullptr;
public:
  GetAddrInfo(std::string host, std::string port);
  ~GetAddrInfo();
  int connect();
  std::vector<addrinfo_result_type> addresses;
  class ConnectionFailure : public std::exception {
  private:
    std::string message;
  public:
    ConnectionFailure(std::string message) {
      this->message = message;
    }
    virtual const char* what() const throw() override {
      return message.c_str();
    }
  };
};

} // namespace trip
} // namespace fdsd

#endif // HTTP_CLIENT_HPP

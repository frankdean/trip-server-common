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
#include "http_client.hpp"
#include "get_options.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <thread>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

using namespace fdsd::utils;
using namespace fdsd::web;

void HttpOptions::add_header(std::string key, std::string value)
{
  headers[key] = value;
}

std::string HttpOptions::to_string() const
{
  return "protocol: \"" + protocol + "\", "
    "proxyHost: \"" + proxyHost + "\", "
    " host: \"" + host + "\", "
    " port: \"" + port + "\", "
    " path: \"" + path + "\", "
    " method: \"" + method + '"';
}

HttpClient::HttpClient() : headers(), options(), body(), status_code() {}
HttpClient::HttpClient(HttpOptions options) : headers(),
                                              options(options),
                                              body(),
                                              status_code() {}

void HttpClient::parse_response(std::vector<char> &response)
{
  auto i = response.begin();
  auto line_begin = response.begin();
  auto previous = response.end();
  auto body_begin = response.end();
  bool first_line = true;
  std::vector<std::string> response_line;
  while (i != response.end()) {
    // std::cout << *i;
    if (*i == '\n' && *previous == '\r') {
      // std::cout << "# " << (previous - line_begin) << " ";
      if (previous - line_begin <= 0) {
        // std::cout << "> blank line\n";
        body_begin = i + 1;
        break;
      } else {
        // header
        auto h1 = line_begin;
        auto h2 = previous - 1;
        auto p1 = previous;
        if (first_line) {
          std::string s;
          std::for_each(h1, i - 1, [&response_line, &s] (const char c) {
            if (c == ' ') {
              if (!s.empty())
                response_line.push_back(s);
              s.clear();
            } else {
              s.push_back(c);
            }
          });
          if (!s.empty())
            response_line.push_back(s);
          first_line = false;
        } else {
          while (h1 <= h2) {
            if (*h1 == ' ' && p1 < previous && *p1 == ':') {
              std::string name;
              std::string value;
              std::for_each(line_begin, p1, [&name] (const char c) {
                name.push_back(c);
              });
              std::for_each(h1 + 1, i - 1, [&value] (const char c) {
                value.push_back(c);
              });
              // std::cout << "Header: \"" << name << "\" -> \"" << value << "\"\n";
              if (!name.empty()) {
                headers[name] = value;
              }
            }
            p1 = h1;
            h1++;
          } // while
        }
      }
      line_begin = i + 1;
    }
    previous = i;
    i++;
  } // while
  // for (auto const& r : response_line) {
  //   std::cout << "First response line: \"" << r << "\"\n";
  // }
  status_code = 400;
  if (response_line.size() > 2) {
    try {
      status_code = std::stoi(response_line[1]);
    } catch (const std::invalid_argument& e ) {
      // ignore
    } catch (const std::out_of_range& e ) {
      // ignore
    }
  }
  if (GetOptions::debug_flag) {
    std::cout << "Response status code: " << status_code << '\n';
    std::cout << '\n';
    for (const auto &h : headers)
      std::cout << "Header: \"" << h.first << "\" -> \"" << h.second << "\"\n";
  }
  if (body_begin != response.end()) {
    body = std::vector<char>(body_begin, response.end());
    // std::for_each(body.begin(), body.end(),
    //               [](const char c) {
    //                 std::cout << std::hex << std::setw(2)
    //                           << std::setfill('0') << (int) c << ' ' << c << '\n';
    //               });
    // std::cout << '\n';
    // std::cout << "Body size: " << body.size() << '\n';
  }
  try {
    std::string length_str = headers.at("Content-Length");
    try {
      std::string::size_type expected_length = std::stol(length_str);
      if (GetOptions::verbose_flag && expected_length != body.size()) {
        std::cerr << "Content-Length: " << expected_length
                  << ", but body size is: " << body.size() << '\n';
      }
    } catch (const std::invalid_argument& e) {
      if (GetOptions::verbose_flag)
        std::cerr << "The Content-Length header is not numeric: \""
                  << length_str << "\"\n";
    } catch (const std::out_of_range& e) {
      if (GetOptions::verbose_flag)
        std::cerr << "The Content-Length header is too large a value to handle: \""
                  << length_str << "\"\n";
    }
  } catch (const std::out_of_range& e) {
    // std::cerr << "The Content-Length header is missing from the response\n";
  }
}

void HttpClient::perform_request()
{
  // When proxying the request we use the proxyHost address for the request.
  // The caller should set the "Host" header to the host it's being proxied to,
  // e.g. tile.openstreetmap.org not localhost
  GetAddrInfo address_info(options.proxyHost.empty() ? options.host : options.proxyHost, options.port);
  if (GetOptions::debug_flag)
    for (const auto address : address_info.addresses)
      std::cout << address << '\n';
  int fd_skt = address_info.connect();
  std::ostringstream request;
  request
    << options.method << " "
    << options.path
    << " HTTP/1.0\r\n";
  // std::cout << "There are " << options.headers.size() << " headers\n";
  for (const auto &h : options.headers)
    request << h.first << ": " << h.second << "\r\n";
  request
    << "\r\n";
  if (GetOptions::debug_flag)
    std::cout << "Request:\n" << request.str() << "\n- - - end request - - -\n";
  if (write(fd_skt, request.str().c_str(), request.str().length()) <0 )
    throw std::runtime_error("Failure writing to socket");

  char buf[1024];
  ssize_t nread;
  std::vector<char> response;
  int again = 0;
  const int again_before_sleep = 5;
  const int again_limit = 10000;
  while (again <= again_limit) {
    nread = read(fd_skt, buf, sizeof(buf));
    // std::cout << "Read " << nread << " bytes\n";
    if (nread > 0) {
      for (int i = 0; i < nread; response.push_back(buf[i++]));
      if (nread < (ssize_t) sizeof(buf)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        set_flag(fd_skt, O_NONBLOCK);
        again++;
      }
    } else if (nread == 0) {
      // m_eof = true;
      break;
    } else if (nread == -1) {
      // std::cout << "error " << errno << '\n';
      switch (errno) {
        case EAGAIN:
          // A little thread sleep helps the next ::read() succeed.
          if (again > again_before_sleep)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
          again++;
          break;
        case EINTR:
          again = again_limit;
          break;
        case EBADF:
          if (GetOptions::verbose_flag)
            std::cerr << " Bad file descriptor (EBADF)"
                    << " File descriptor\n";
          syslog(LOG_NOTICE,
                 "Bad file descripitor reading from %s",
                 options.host.c_str());
          again = again_limit;
          break;
        default:
          throw std::runtime_error("Unexpected error reading socket (" +
                                   std::to_string(errno) + ")");
      }
    } else {
      throw std::runtime_error("Unexpected response reading socket");
    }
  } // while
  if (again >= again_limit) {
    if (GetOptions::verbose_flag)
      std::cerr << "Abandoned reading response after the maximum "
                << again_limit << " attempts";
    syslog(LOG_NOTICE,
           "Abandoned reading response after the maximum %d attempts",
           again_limit);
  }
  if (close(fd_skt) < 0)
    throw std::runtime_error("Failure closing socket");
  parse_response(response);
  // if (body.size() > 0) {
  //   auto os = std::ofstream("tile.png", std::ios::binary);
  //   for (auto i = body.cbegin(); i != body.cend(); i++) {
  //     os << *i;
  //   }
  // } else {
  //   std::cerr << "The response has no body\n";
  // }
}

// void HttpClient::run_test()
// {
//   GetAddrInfo address_info(options.host, options.port);
//   for (const auto address : address_info.addresses) {
//     std::cout << address << '\n';
//   }
// }

std::string addrinfo_result_type::to_string() const
{
  return "ip_address: \"" + ip_address + "\", port: \"" + port +
    "\", protocol: " + std::to_string(protocol);
}

GetAddrInfo::GetAddrInfo(std::string host, std::string port) : addresses()
{
  this->host = host;
  this->port = port;
  struct addrinfo hint;
  int err;
  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;
  if ((err = getaddrinfo(host.c_str(), port.c_str(), &hint, &infop)) != 0) {
    std::ostringstream os;
    os << "Failure looking up host in DNS: " << gai_strerror(err);
    throw std::runtime_error(os.str());
  }
  // Use a copy of infop to iterate the results.  We need the original to free
  // the memory allocated by getaddrinfo.
  struct addrinfo* p = infop;
  for ( ; p != nullptr; p = p->ai_next) {
    struct sockaddr_in *sa = (struct sockaddr_in *)p->ai_addr;
    addrinfo_result_type r;
    r.ip_address = std::string(inet_ntoa(sa->sin_addr));
    r.port = std::to_string(ntohs(sa->sin_port));
    r.protocol = p->ai_protocol;
    addresses.push_back(r);
  }
}

GetAddrInfo::~GetAddrInfo()
{
  if (infop != nullptr)
    freeaddrinfo(infop);
}

int GetAddrInfo::connect()
{
  // Use a copy of infop to iterate the results.  We need the original to free
  // the memory allocated by getaddrinfo.
  struct addrinfo* rp;
  int sfd = -1;
  for (rp = infop ; rp != nullptr; rp = rp->ai_next) {
    // struct sockaddr_in *sa = (struct sockaddr_in *)rp->ai_addr;
    // printf("Trying  %s on port %d\n", inet_ntoa(sa->sin_addr), ntohs(sa->sin_port));
    sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
    if (sfd == -1)
      throw std::runtime_error("Failed to create socket");

    if (::connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break; // success

    close(sfd);
  }
  if (rp == nullptr) {
    std::ostringstream os;
    os << "Failed to connect to host: " << host << ", port " << port;
    throw ConnectionFailure(os.str());
  }
  return sfd;
}

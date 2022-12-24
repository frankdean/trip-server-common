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
#ifndef SOCKET_HPP
#define SOCKET_HPP

// #define DEBUG_SOCKET_CPP

#include "logger.hpp"
#include <exception>
#include <netinet/in.h>
#include <sstream>
#include <string>

namespace fdsd
{
namespace web
{

class HTTPServerRequest;

class SocketUtils {
protected:
  static utils::Logger logger;
  void set_flag(int fd, int flag) const;
  void clear_flag(int fd, int flag) const;
public:
  SocketUtils() {}
};

class Socket : public SocketUtils
{
private:
  int server_fd, new_socket;
  std::string m_listen_address;
  std::string m_port;
  struct sockaddr_in listen_socket_address;
public:
  Socket(std::string listen_address, std::string port);
  ~Socket();
  bool have_connection();
  int wait_connection(int timeout);
};

class SocketHandler : public SocketUtils
{
private:
  int m_fd;
  long maximum_request_size;
  bool m_eof;
  long total_read;
  bool headers_complete;
  bool read_complete;
  long content_length;
  long content_read_count;
#ifdef DEBUG_SOCKET_CPP
  long again_count;
#endif
  int line_count;
protected:
  void getline(std::string &s);
public:
  SocketHandler(int fd, long maximum_request_size)
    : m_fd(fd),
      maximum_request_size(maximum_request_size),
      m_eof(false),
      total_read(),
      headers_complete(false),
      read_complete(false),
      content_length(),
      content_read_count(),
#ifdef DEBUG_SOCKET
      again_count(),
#endif
      line_count() {}
  ~SocketHandler();
  bool is_more_data_to_read(int timeout) const;
  void close() const;
  void read(HTTPServerRequest &request);
  std::string read();
  void send(std::ostringstream& os) const;
  bool is_eof() const {
    return m_eof;
  }
};

} // namespace web
} // namespace fdsd

#endif // SOCKET_HPP

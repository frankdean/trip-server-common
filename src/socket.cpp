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
#include "http_request.hpp"
#include "socket.hpp"
#include "debug_utils.hpp"
#include "uri_utils.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

using namespace fdsd::utils;
using namespace fdsd::web;

#ifdef DEBUG_SOCKET_CPP
Logger SocketUtils::logger("socket", std::clog, fdsd::utils::Logger::debug);
#else
Logger SocketUtils::logger("socket", std::clog, fdsd::utils::Logger::info);
#endif

Socket::Socket(std::string listen_address, std::string port)
  :
  server_fd(-1),
  new_socket(-1),
  m_listen_address(listen_address),
  m_port(port)
{
    listen_socket_address.sin_family = AF_INET;
    listen_socket_address.sin_port = htons((uint16_t) stoi(port));
    if (listen_address == "0.0.0.0") {
      listen_socket_address.sin_addr.s_addr = INADDR_ANY;
    } else {
      listen_socket_address.sin_addr.s_addr = inet_addr(listen_address.c_str());
    }
    // listen_socket_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    // listen_scoket_address.sin_addr.s_addr = inet_addr("0.0.0.0");

    int addrlen = sizeof(listen_socket_address);

    // std::cout << "Initialising socket\n";
    // AF_INET = address family
    // PF_INET = protocol family

    // AF_INET & PF_INET are often defined to be the same value in socket.h
    // but it may be more correct to use PF_INET.

    // if ((server_fd = socket(PF_UNIX, SOCK_STREAM, 0)) == 0) {
    if ((server_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
      // cleanup_locale_memory_leaks();
      // exit(EXIT_FAILURE);
      throw std::runtime_error("Failed to initialise socket");
    }

    int opt = 1;
    if (setsockopt(server_fd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt))) {
      throw std::runtime_error("Failed to set listening socket SO_RESUSEADDR option");
    }
    opt = 1;
    if (setsockopt(server_fd,
                   SOL_SOCKET,
                   SO_KEEPALIVE,
                   &opt,
                   sizeof(opt))) {
      throw std::runtime_error("Failed to set listening socket SO_KEEPALIVE option");
    }
    // std::cout << "binding socket\n";
    if (bind(server_fd, (struct sockaddr*) &listen_socket_address, addrlen) < 0) {
      throw std::runtime_error("Failed to bind to socket");
    }
    // std::cout << "listening\n";
    if (listen(server_fd, SOMAXCONN) < 0) {
      throw std::runtime_error("Failed to listen to socket");
    }
    // set_flag(serverm_fd, O_NONBLOCK);
}

Socket::~Socket()
{
  if (server_fd >= 0) {
    try {
      ::close(server_fd);
    } catch (const std::exception& e) {
      // ignore
    }
  }
}

/**
 * Uses accept to check if there is a connection.  Returns without any timeout
 * or delay mechanism.
 * \return true if there is a connection on this socket, waiting to be read.
 */
bool Socket::have_connection()
{
  // std::cout << "accepting connection\n";
  int addrlen = sizeof(listen_socket_address);
  new_socket = ::accept(server_fd, (struct sockaddr*) &listen_socket_address,
                        (socklen_t*) &addrlen);
  return new_socket != -1;
}

/**
 * Uses poll to wait for a connection on the socket.
 * \param timeout timeout in milliseconds
 * \return the socket file descriptor if there is a connection on this socket,
 * waiting to be read, -1 if there is an error or a timeout occurred.
 */
int Socket::wait_connection(int timeout)
{
  struct pollfd poll_fd;
  poll_fd.fd = server_fd;
  poll_fd.events = POLLIN;
  poll_fd.revents = 0;
  struct pollfd fdinfo[1] = { { 0 } };
  fdinfo[0] = poll_fd;
  // logger << Logger::debug << "Polling... " << server_fd << Logger::endl;
  int nfds = poll(fdinfo,
                  1,
                  timeout);
  if (nfds >= 0) {
    // logger << Logger::debug
    //        << "There are " << nfds
    //        << " (read) ready file descriptors "
    //        << fdinfo[0].fd << Logger::endl;
    if (fdinfo[0].revents & POLLHUP) {
      logger << Logger::warn << "Warning socket disconnected" << Logger::endl;
    }
    if (fdinfo[0].revents & POLLERR) {
      logger << Logger::warn << "Warning socket polling error" << Logger::endl;
    } else if (fdinfo[0].revents & POLLNVAL) {
      // throw std::runtime_error("Invalid file descriptor");
      logger << Logger::warn << "Invalid file descriptor" << Logger::endl;
    } else if (fdinfo[0].revents & (POLLIN/* | POLLPRI*/)) {
      int addrlen = sizeof(listen_socket_address);
      new_socket = ::accept(server_fd,
                            (struct sockaddr*) &listen_socket_address,
                            (socklen_t*) &addrlen);
      // logger << Logger::debug << "There is data to read on socket "
      //        << new_socket << Logger::endl;
#ifdef ENABLE_KEEP_ALIVE
      int opt = 1;
      if (setsockopt(new_socket,
                     SOL_SOCKET,
                     SO_KEEPALIVE,
                     &opt,
                     sizeof(opt))) {
        // throw std::runtime_error("Failed to set new socket SO_KEEPALIVE option");
        logger << Logger::warn
               << "Failed to set new socket SO_KEEPALIVE option"
               << Logger::endl;
      }
#endif
      // logger << Logger::debug << "New socket: " << new_socket << Logger::endl;
      return new_socket;
    // } else {
    //   logger << Logger::debug
    //          << "Poll returned events: " << poll_fd.revents << Logger::endl;
    }
  } else {
    if (errno == EINTR) {
      // This happens when the server is stopped with Ctrl-C, so ignore in
      // that situation
      logger << Logger::debug;
    } else {
      logger << Logger::error;
    }
    logger
      << "Error whilst polling socket: "
      << ::strerror(errno)
      << Logger::endl;
  }
  // logger << Logger::debug << "Finished polling" << Logger::endl;
  return -1;
}

void SocketUtils::set_flag(int fd, int flag) const
{
  int flags;
  if ((flags = fcntl(fd, F_GETFL)) < 0) {
    throw std::runtime_error("Failure getting option flags");
  }
  if (fcntl(fd, F_SETFL, flags | flag) < 0) {
    throw std::runtime_error("Failure setting option flags");
  }
}

void SocketUtils::clear_flag(int fd, int flag) const
{
  int flags;
  if ((flags = fcntl(fd, F_GETFL)) < 0) {
    throw std::runtime_error("Failure getting option flags");
  }
  if (fcntl(fd, F_SETFL, flags & ~flag) < 0) {
    throw std::runtime_error("Failure clearing option flags");
  }
}

SocketHandler::~SocketHandler()
{
  try {
    close();
  } catch (const std::exception& e) {
    // ignore
  }
}

void SocketHandler::close() const
{
  // There are a lot of varying opinions on how to gracefully close a socket.
  // The Gnu documentation [1] suggests using SO_LINGER.

  // [1]: https://www.gnu.org/software/libc/manual/html_node/Closing-a-Socket.html

  // Half close the connection (the writing half)
  if (shutdown(m_fd, SHUT_WR) < 0) {
    logger << Logger::warn
           << std::this_thread::get_id()
           << " Error shutting down socket " << m_fd << " for write: ("
           << errno << ") " << ::strerror(errno) << Logger::endl;
  }

  // Set blocking mode so that perhaps close waits.
  clear_flag(m_fd, O_NONBLOCK);

  // Use SO_LINGER to ask close to wait until all the data has been
  // transmitted, subject to a timeout.
  struct linger linger;
  // Close blocks until data transmitted or timeout
  linger.l_onoff = 1;
  // Timeout period in seconds
  linger.l_linger = 120;
  if (setsockopt(m_fd,
                 SOL_SOCKET,
#ifdef __APPLE__
                 SO_LINGER_SEC,
#else
                 SO_LINGER,
#endif
                 (void *)&linger,
                 sizeof(linger)) != 0) {
#ifdef DEBUG_SOCKET_CPP
    logger << Logger::debug
           << std::this_thread::get_id()
           << " Failure setting SO_LINGER socket option: "
           << ::strerror(errno)
           << Logger::endl;
#endif // DEBUG_SOCKET_CPP
  }
  try {
    // std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    if (::close(m_fd) < 0)
      logger << Logger::debug
             << std::this_thread::get_id()
             << " Error closing socket: "
             << ::strerror(errno) << Logger::endl;

  } catch (const std::exception& e) {
    logger << Logger::debug
           << std::this_thread::get_id()
           << " Exception closing socket: "
           << e.what() << Logger::endl;
  }
}

bool SocketHandler::is_more_data_to_read(int timeout) const
{
  struct pollfd poll_fd;
  poll_fd.fd = m_fd;
  poll_fd.events = POLLIN;
  poll_fd.revents = 0;
  struct pollfd fdinfo[1] = { { 0 } };
  fdinfo[0] = poll_fd;
#ifdef DEBUG_SOCKET_CPP
  auto start = std::chrono::system_clock::now();
#endif // DEBUG_SOCKET_CPP
  int nfds = poll(fdinfo,
                  1,
                  timeout);
#ifdef DEBUG_SOCKET_CPP
  auto finish = std::chrono::system_clock::now();
  if (logger.is_level(Logger::debug)) {
    std::chrono::duration<double, std::milli> diff = finish - start;
    if (diff.count() > 200)
      logger << Logger::debug
             << "Timeout after " << diff.count() << "ms"
             << Logger::endl;
  }
#endif // DEBUG_SOCKET_CPP
  if (nfds >= 0) {
    // std::cout << "There are " << nfds
    //           << " (read) ready file descriptors "
    //           << fdinfo[0].fd << "\n";

#ifdef __linux__
    if (fdinfo[0].revents & POLLRDHUP) {
#ifdef DEBUG_SOCKET_CPP
      if (logger.is_level(Logger::debug))
        logger
          << Logger::debug
          << std::this_thread::get_id()
          << " Warning socket disconnected by client (checking if more to read)"
          << Logger::endl;
#endif // DEBUG_SOCKET_CPP
      return false;
    }
#endif
    if (fdinfo[0].revents & POLLHUP) {
#ifdef DEBUG_SOCKET_CPP
      if (logger.is_level(Logger::debug))
        logger << Logger::debug
               << std::this_thread::get_id()
               << " Peer closed its channel, while checking if more to read"
               << Logger::endl;
#endif // DEBUG_SOCKET_CPP
      return false;
    }
    if (fdinfo[0].revents & POLLERR) {
      logger
        << Logger::warn
        << std::this_thread::get_id()
        << " Warning socket polling error (checking if more to read)"
        << Logger::endl;
      return false;
    } else if (fdinfo[0].revents & POLLNVAL) {
      logger
        << Logger::warn
        << std::this_thread::get_id()
        << " Invalid file descriptor"
        << Logger::endl;
      return false;
    } else if (fdinfo[0].revents & (POLLIN/* | POLLPRI*/)) {
      // logger << Logger::debug << "There is more data to read!" << Logger::endl;
      return true;
    }
  } else {
    if (errno == EINTR) {
      // This happens when the server is stopped with Ctrl-C, so ignore in
      // that situation
      logger << Logger::debug;
    } else {
      logger << Logger::error;
    }
    logger
      << "Error whilst polling socket for more data: "
      << ::strerror(errno)
      << Logger::endl;
  }
  return false;
}

void SocketHandler::send(std::ostringstream& os) const
{
  // There isn't currently a way of modifying the stream as we read bits of it
  // so we need to make a copy.
  std::string message(os.str());
  int count = 0;
  // Sets the maximum number of blocks send, including any failed attempts
  int limit = 100;
  while (true && ++count < limit) {
    int length = message.length();
    // logger << Logger::debug
    //        << "Sending response message (" << m_fd << ") length: "
    //        << length
    //        << Logger::endl;
    int r;
    if ((r = ::send(m_fd, message.c_str(), length, 0)) == length) {
      // logger << Logger::debug << "Success (" << m_fd << ") length: " << length << Logger::endl;
      break;
    } else if (r < 0) {
      // logger << Logger::debug << "Response (" << m_fd << ") " << r
      //        << Logger::endl;
      switch (errno) {
        case EMSGSIZE:
          logger << Logger::warn << "Error sending response: EMSGSIZE"
                 << Logger::endl;
          break;
        case EAGAIN:
          logger << Logger::warn << "Error sending response: EAGAIN"
                 << Logger::endl;
          break;
        default:
          logger << Logger::warn << "Error sending response: ("
                 << errno << ") " << ::strerror(errno) << Logger::endl;
      }
      break;
    } else if (r != length) {
      // logger
      //   << Logger::debug
      //   << std::this_thread::get_id()
      //   << " socket (" << m_fd << ")"
      //   << " failed to send entire response.  Tried to send "
      //   << length << " bytes, but only "
      //   << r << " actually sent"
      //   << Logger::endl;
      struct pollfd poll_fd;
      poll_fd.fd = m_fd;
      poll_fd.events = POLLOUT;
      poll_fd.revents = 0;
      struct pollfd fdinfo[1] = { { 0 } };
      fdinfo[0] = poll_fd;
      int nfds = poll(fdinfo,
                      1,
                      10000);
      // logger << Logger::debug
      //        << std::this_thread::get_id()
      //        << " there are " << nfds
      //        << " (write) ready file descriptors ("
      //        << fdinfo[0].fd << ')' << Logger::endl;
      if (nfds >= 0) {
        if (fdinfo[0].revents & POLLHUP) {
          logger << Logger::warn
                 << std::this_thread::get_id()
                 << " Warning (write) socket disconnected"
                 << Logger::endl;
          break;
        }
        if (fdinfo[0].revents & POLLERR) {
          logger << Logger::warn
                 << std::this_thread::get_id()
                 << " Warning (write) socket polling error"
                 << Logger::endl;
          break;
        } else if (fdinfo[0].revents & POLLNVAL) {
          logger << Logger::warn
                 << std::this_thread::get_id()
                 << " Warning (write) socket invalid file descriptor"
                 << Logger::endl;
          break;
        } else if (fdinfo[0].revents & (POLLOUT)) {
          // logger << Logger::debug
          //        << std::this_thread::get_id()
          //        << " trying to send more... (" << count << ')' << Logger::endl;
          message.erase(0, r);
        }
      }
    }
  } // while
  // logger << Logger::debug
  //        << "Finished sending on thread "
  //        << std::this_thread::get_id()
  //        << Logger::endl;
}

void SocketHandler::getline(std::string &s)
{
  char c = 0;
  bool loop = true;
  while (c != '\n' && loop) {
    int valread = recv(m_fd, &c, 1, 0);
#ifdef DEBUG_SOCKET_CPP
    // logger << Logger::debug << "getline-read-result: " << valread;
    // if (c > ' ')
    //   logger << Logger::debug << " '" << c << "' ";
    // logger << Logger::debug
    //        << " value: " << (int) c << Logger::endl;
#endif
    if (headers_complete && valread > 0) {
      content_read_count += valread;
#ifdef DEBUG_SOCKET_CPP_TRACE
      logger << Logger::debug << "content_read_count: " << content_read_count
             << Logger::endl;
#endif
    }
    total_read++;
    if (total_read >= maximum_request_size)
      throw PayloadTooLarge();
    if (valread > 0) {
      if (c == '\r') {
        valread = recv(m_fd, &c, 1, MSG_PEEK);
        if (valread > 0 && c == '\n') {
          total_read++;
          line_count++;
          valread = recv(m_fd, &c, 1, 0);
          if (headers_complete)
            content_read_count += valread;
        }
      } else {
        s.push_back(c);
      }
    } else if (valread < 0) {
      switch (errno) {
        // case EWOULDBLOCK: <-- Same value as EAGAIN.  Both defined in include
        // files for portability
        case EAGAIN: {
#ifdef DEBUG_SOCKET_CPP_TRACE
          if (logger.is_level(Logger::debug))
            logger << Logger::debug
                   << std::this_thread::get_id()
                   << " (read) EAGAIN (" << again_count << ") on socket " << m_fd
                   << Logger::endl;
#endif
          loop = false;
          if (!headers_complete ||
              (content_length > 0 && content_read_count < content_length)) {
            const int timeout = headers_complete ? 10000 : 1000;
#ifdef DEBUG_SOCKET_CPP
          logger << Logger::debug
                 << "Checking (EAGAIN) for more data with timeout of "
                 << timeout << Logger::endl;
#endif
            read_complete = !is_more_data_to_read(timeout);
            if (!read_complete) {
              // set_flag(m_fd, O_NONBLOCK);
              loop = true;
#ifdef DEBUG_SOCKET_CPP_TRACE
              logger << Logger::debug << "There is more data" << Logger::endl;
              again_count++;
            } else {
              logger << Logger::debug << "There is no more data" << Logger::endl;
#endif
            }
#ifdef DEBUG_SOCKET_CPP_TRACE
          } else {
            logger << Logger::debug << "Not trying again" << Logger::endl;
#endif
          }
          break;
        }
        case EINTR: {
#ifdef DEBUG_SOCKET_CPP
          if (logger.is_level(Logger::debug))
            logger << Logger::debug
                   << std::this_thread::get_id()
                   << "(read) EINTR on socket " << m_fd << Logger::endl;
#endif // DEBUG_SOCKET_CPP
          // EINTR only occurs on non-blocking reads
          // loop again
          break;
        }
        case EBADF: {
          logger << Logger::warn << std::this_thread::get_id()
                 << " Bad file descriptor (EBADF)"
                 << " File descriptor: " << m_fd
                 << Logger::endl;
          loop = false;
          break;
        }
        case ETIMEDOUT: {
          logger << Logger::warn << std::this_thread::get_id()
                 << " Timeout (ETIMEDOUT)"
                 << " File descriptor: " << m_fd
                 << Logger::endl;
          loop = false;
          break;
        }
        default:
          throw std::runtime_error("Unexpected error reading socket (" +
                                   std::to_string(errno) + ")");
      }
    } else {
#ifdef DEBUG_SOCKET_CPP
      logger << Logger::debug << "EOF" << Logger::endl;
#endif
      loop = false;
    }
    if (!loop)
      read_complete = true;
#ifdef DEBUG_SOCKET_CPP
    // if (loop)
    //   logger << Logger::debug << "getline() looping" << Logger::endl;
    // else
    //   logger << Logger::debug << "getline() not looping" << Logger::endl;
#endif
  } // while
#ifdef DEBUG_SOCKET_CPP
  // logger << Logger::debug << "Exiting getline()" << Logger::endl;
#endif
}

void SocketHandler::read(HTTPServerRequest &request)
{
  clear_flag(m_fd, O_NONBLOCK);
  // set_flag(m_fd, O_NONBLOCK);
#ifdef DEBUG_SOCKET_CPP_TRACE
  logger << Logger::debug << "   --- New request ---   " << Logger::endl;
#endif
  std::string body;
  while (true) {
    std::string s;
    // std::cout << "getline()\n";
    getline(s);
#ifdef DEBUG_SOCKET_CPP_TRACE
    logger << Logger::debug << line_count << " >>>" << s << "<<< "
           << s.size() << " (" << content_read_count  << ")" << Logger::endl;
#endif
    if (!headers_complete && s.empty()) {
      headers_complete = true;
      // Using non-blocking reads once all the headers have been read
      set_flag(m_fd, O_NONBLOCK);
      content_length = request.get_content_length();
#ifdef DEBUG_SOCKET_CPP_TRACE
      logger << Logger::debug << "-- end of headers --- ContentLength: "
             << content_length << " (" << content_read_count << ")" << Logger::endl;
#endif
    } else if (!headers_complete) {
      std::string::size_type p;
      if (line_count == 1) {
        p = s.find(' ');
        if (p != std::string::npos) {
          auto search = request.request_methods.find(s.substr(0, p));
          if (search != request.request_methods.end()) {
            request.method = search->second;
          }
          // std::cout << "Method type is: \"" << request.method_to_str() << "\"\n";
          s.erase(0, p+1);
          // std::cout << "> After erase: " << line_count << " \"" << s << "\"\n";
          p = s.find(' ');
          if (p != std::string::npos) {
            request.uri = s.substr(0, p);
            s.erase(0, p+1);
          }
          // std::cout << "HTTP_REQUEST Path: \"" << request.uri << "\"\n";
          request.set_query_params(UriUtils::get_query_params(request.uri));
          // std::cout << "Query parameters:\n";
          // for (auto qp = request.query_params.begin(); qp != request.query_params.end(); ++qp) {
          //   std::cout << qp->first << '=' << qp->second << '\n';
          // }
          request.protocol = s;
          // std::cout << "Protocol: \"" << request.protocol << "\"\n";
        } else {
          std::cerr << "Warning: invalid request at line " << line_count << '\n';
        }
      } else {
        // TODO Is the space after colon in HTTP header required
        p = s.find(": ");
        if (p != std::string::npos) {
          const std::string key = s.substr(0, p);
          const std::string value = p < s.size() ? s.substr(p+2) : "";
          // std::cout << "Header: \"" << key << "\" -> \"" << value << "\"\n";
          request.headers[key] = value;
        } else {
          std::cerr << "Warning: invalid header at line " << line_count << '\n';
        }
      }
    } else {
#ifdef DEBUG_SOCKET_CPP
      // logger << Logger::debug << "body line" << Logger::endl;
#endif
      request.handle_content_line(s);
    }
    if (read_complete) {
#ifdef DEBUG_SOCKET_CPP_TRACE
      logger << Logger::debug << "-- read complete flag is set--" << Logger::endl;
#endif
      // Have we really finished reading all the data?
      if (!headers_complete ||
          (content_length > 0 && content_read_count < content_length)) {
        const int timeout = headers_complete ? 10000 : 1000;
#ifdef DEBUG_SOCKET_CPP
          logger << Logger::debug << "Checking for more data with timeout of "
                 << timeout << Logger::endl;
#endif
          read_complete = !is_more_data_to_read(timeout);
          if (read_complete) {
#ifdef DEBUG_SOCKET_CPP
            logger << Logger::debug << "No more data to read" << Logger::endl;
#endif
            break;
          } else {
#ifdef DEBUG_SOCKET_CPP
            logger << Logger::debug << "More data to read" << Logger::endl;
#endif
            // set_flag(m_fd, O_NONBLOCK);
            // clear_flag(m_fd, O_NONBLOCK);
            // std::cout << "More data\n";
          }
      } else {
#ifdef DEBUG_SOCKET_CPP_TRACE
        logger << Logger::debug << "Finished reading request" << Logger::endl;
#endif
        break;
      }
    } // read_complete
  } // while

#ifdef DEBUG_SOCKET_CPP
  // logger << Logger::debug << "Content-Length: " << content_length
  //        << " content read count: " << content_read_count << Logger::endl;
  // logger << Logger::debug << "EAGAIN count: " << again_count << Logger::endl;
#endif
  if (content_length >= 0 && content_length != content_read_count) {
    logger << Logger::warn << "Warning: Content-Length provided as "
           << content_length << " does not match actual length of "
           << content_read_count << Logger::endl;
  }
}

/// \deprecated use read(HTTPServerRequest&) instead
std::string SocketHandler::read()
{
  // if (logger.is_level(Logger::debug))
  //   logger << Logger::debug
  //          << std::this_thread::get_id()
  //          << " Reading from file descriptor: " << m_fd << Logger::endl;

  // set_flag(m_fd, O_NONBLOCK);
  std::string request_body;
  char buffer[1024] = { 0 };
  int again = 0;
  const int again_limit = 3;
  int short_read_count = 0;
  int previous_read = 0;
#ifdef DEBUG_SOCKET_CPP
  int empty_buffer_count = 0;
  int eagain_count = 0;
  int max_again_level = 0;
#endif // DEBUG_SOCKET_CPP
  while (again < again_limit) {
    // std::cout << "reading max " << sizeof(buffer) << " bytes from socket\n";
    // if (logger.is_level(Logger::debug))
    //   logger << Logger::debug
    //          << "Reading buffer from socket " << m_fd << Logger::endl;
    int valread = ::read(m_fd, buffer, sizeof(buffer));
    if (previous_read < 0 && valread > 0) {
#ifdef DEBUG_SOCKET_CPP
      if (logger.is_level(Logger::debug) && eagain_count > 0)
        logger << Logger::debug
               << "There was a successful read of " << valread
               << " bytes after EAGAIN (" << eagain_count << ")"
               << Logger::endl;
#endif // DEBUG_SOCKET_CPP
      again = 0;
    }
    previous_read = valread;
    // if (logger.is_level(Logger::debug))
    //   logger << Logger::debug << "Read " << valread << " bytes from socket "
    //          << m_fd << Logger::endl;
    if (valread > 0) {
      total_read += valread;
      if (logger.is_level(Logger::debug)) {
        // if (short_read_count > 0) {
        //   logger << Logger::debug
        //          << std::this_thread::get_id()
        //          << ' ' << short_read_count << " short block reads on socket "
        //          << m_fd
        //          << Logger::endl;
        // }
        // logger << Logger::debug << "<< buffer >>" << Logger::endl;
        // std::string sbuf(buffer, valread);
        // DebugUtils::hex_dump(sbuf, std::clog);
        // logger << Logger::debug << "<< end of buffer >>" << Logger::endl;
      }
      request_body.append(buffer, valread);
      if (valread < sizeof(buffer)) {
        short_read_count++;
        // We definitely see the number of bytes being read being less than the
        // buffer size, but in fact there is actually more data to be read, so
        // we must try again.
        set_flag(m_fd, O_NONBLOCK);
#ifdef DEBUG_SOCKET_CPP
        empty_buffer_count++;
#endif // DEBUG_SOCKET_CPP
        // if (logger.is_level(Logger::debug))
        //   logger << Logger::debug << "Buffer is empty on socket " << m_fd
        //          << Logger::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // if (is_more_data_to_read(2000)) {
        //   if (logger.is_level(Logger::debug))
        //     logger << Logger::debug
        //            << "Buffer less than full but more data to read on socket "
        //            << m_fd << Logger::endl;
        //   again = 0;
        // } else {
        //   logger << Logger::debug
        //          << "Buffer less than full and no more data to read on socket "
        //          << m_fd << Logger::endl;
        //   again = again_limit;
        // }
      } // else {
      // The socket read blocks (observed with Chrome) when the data exactly
      // matches the buffer size, so play safe and set non-blocking mode for all
      // subsequent reads.
      set_flag(m_fd, O_NONBLOCK);
      again = 0;
    } else if (valread == 0) {
#ifdef DEBUG_SOCKET_CPP
      if (logger.is_level(Logger::debug))
        logger << Logger::debug
               << std::this_thread::get_id()
               << " Break - EOF on socket " << m_fd << Logger::endl;
#endif // DEBUG_SOCKET_CPP
      m_eof = true;
      break;
    } else if (valread == -1) {
      // if (logger.is_level(Logger::debug))
      //   logger << Logger::debug
      //          << "errno: " << errno << " " << strerror(errno)
      //          << " on socket " << m_fd << Logger::endl;
      switch (errno) {
        // case EWOULDBLOCK: <-- Same value as EAGAIN.  Both defined in include
        // files for portability
        case EAGAIN: {
          // if (logger.is_level(Logger::debug))
          //   logger << Logger::debug
          //          << std::this_thread::get_id()
          //          << "(read) EAGAIN (" << again << ") on socket " << m_fd
          //          << Logger::endl;
          if (short_read_count == 0) {
            // logger << Logger::debug
            //        << std::this_thread::get_id()
            //        << " Sleeping a little x(" << again << ") on socket " << m_fd
            //        << Logger::endl;
            set_flag(m_fd, O_NONBLOCK);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            again++;
          } else {
            again = again_limit;
          }
#ifdef DEBUG_SOCKET_CPP
          eagain_count++;
#endif // DEBUG_SOCKET_CPP
          break;
        }
        case EINTR: {
#ifdef DEBUG_SOCKET_CPP
          if (logger.is_level(Logger::debug))
            logger << Logger::debug
                   << std::this_thread::get_id()
                   << "(read) EINTR on socket " << m_fd << Logger::endl;
#endif // DEBUG_SOCKET_CPP
          again = again_limit;
          break;
        }
        case EBADF: {
          logger << Logger::warn << std::this_thread::get_id()
                 << " Bad file descriptor (EBADF)"
                 << " File descriptor: " << m_fd
                 << Logger::endl;
          again = again_limit;
          break;
        }
        case ETIMEDOUT: {
          logger << Logger::warn << std::this_thread::get_id()
                 << " Timeout (ETIMEDOUT)"
                 << " File descriptor: " << m_fd
                 << Logger::endl;
          again = again_limit;
          break;
        }
        default:
          throw std::runtime_error("Unexpected error reading socket (" +
                                   std::to_string(errno) + ")");
      }
    } else {
      throw std::runtime_error("Unexpected response reading socket");
    }
  }
#ifdef DEBUG_SOCKET_CPP
  if (logger.is_level(Logger::debug)) {
    if (empty_buffer_count > 1)
      logger << Logger::debug << "The buffer was empty on "
             << empty_buffer_count << " occasions" << Logger::endl;
    // if (again <= again_limit)
    //   logger << Logger::debug << "Again count: " << again
    //          << Logger::endl;
    if (eagain_count > 0)
      logger << Logger::debug << "EAGAIN error count: " << eagain_count
             << " with again count reaching a maxium of " << max_again_level
             << Logger::endl;
    logger << Logger::debug
           << "Read total of " << total_read << " bytes from socket " << m_fd
           << Logger::endl;
  }
#endif // DEBUG_SOCKET_CPP
  // std::cout << "<< request body >>" << '\n';
  // DebugUtils::hex_dump(request_body, std::cout);
  // std::cout << "<< end of request body >>" << '\n';

  // if (logger.is_level(Logger::debug)) {
  //   logger << Logger::debug << "<< request body >>" << Logger::endl;
  //   DebugUtils::hex_dump(request_body, std::clog);
  //   logger << "<< end of request body >>" << Logger::endl;
  // }
  return request_body;
}

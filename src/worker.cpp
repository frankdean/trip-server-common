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
#include "worker.hpp"
#include "http_request.hpp"
#include "http_request_factory.hpp"
#include "http_request_handler.hpp"
#include "http_response.hpp"
#include "socket.hpp"
#include <chrono>
#include <thread>

using namespace fdsd::web;
using namespace fdsd::utils;

Logger Worker::logger("Worker", std::clog, fdsd::utils::Logger::info);

std::mutex fdsd::web::readyMutex;
std::condition_variable fdsd::web::readyCondVar;

Worker::Worker(std::queue<int>& q,
               std::shared_ptr<fdsd::web::HTTPRequestFactory> request_factory)
  : queue(q), stop_flag(false),
#ifdef ENABLE_KEEP_ALIVE
    keep_alive(false),
#endif
    request_factory(request_factory)
{
}

bool Worker::handle_socket_read(fdsd::web::SocketHandler& socket_handler)
{
  // logger << Logger::debug
  //         << std::this_thread::get_id()
  //         << " Reading request"
  //         << Logger::endl;
  std::string request_body = socket_handler.read();
  if (request_body.size() > 0) {
    // logger << Logger::debug << "Request size of " << request_body.size()
    //         << " bytes\n"
    //         << "<< body >>\n" << request_body << "\n<< end of body >>\n"
    //         << Logger::endl;
    HTTPServerRequest request(request_body);
    // logger << Logger::debug
    //         << "\n<< content >>\n"
    //         << request.content
    //         << "\n<< end of content >>\n"
    //         << Logger::endl;

    int content_length = -1;
    try {
      std::string s = request.get_header("Content-Length");
      if (!s.empty())
        content_length = std::stoi(s);
    } catch (const std::invalid_argument& e) {
      logger << Logger::debug << std::this_thread::get_id()
              << " Invalid content length header" << Logger::endl;
    } catch (const std::out_of_range& e) {
      // logger << Logger::debug
      //         << std::this_thread::get_id()
      //         << " Content length header out of range error" << Logger::endl;
    }
    // logger << Logger::debug
    //         << std::this_thread::get_id()
    //         << " Content-Length: " << content_length
    //         << Logger::endl;

    if (content_length > 0) {

      auto timeout = std::chrono::system_clock::now() +
        std::chrono::milliseconds(10000);
      while (request.content.size() < content_length &&
             std::chrono::system_clock::now() < timeout && !stop_flag) {
        // logger << Logger::debug
        //         << std::this_thread::get_id()
        //         << "Content-Length header: " << content_length
        //         << " does not match actual content length of: "
        //         << request.content.size()
        //         << "\nContent: \"" << request.content << "\""
        //         << Logger::endl;
        std::string more = socket_handler.read();
        if (more.size() > 0) {
          // logger << Logger::debug
          //         << std::this_thread::get_id()
          //         << " appending " << more.size() << " bytes"
          //         << Logger::endl;
          request.content.append(more);
        }
      }
    }

    // if (logger.is_level(Logger::debug) &&
    //     content_length >= 0 && request.content.size() != content_length) {
    //   logger << Logger::debug
    //           << std::this_thread::get_id()
    //           << "Content-Length header: " << content_length
    //           << " does not match actual content length of: "
    //           << request.content.size()
    //           << "\nContent: \"" << request.content << "\""
    //           << Logger::endl;
    // }

    std::string connection = request.get_header("Connection");
#ifdef ENABLE_KEEP_ALIVE
    keep_alive = istr_compare(connection, "keep-alive");
#endif

    auto response = request_factory->create_response_object();
    auto handler =
      request_factory->create_request_handler(request, *response);
#ifdef ENABLE_KEEP_ALIVE
    if (keep_alive) {
      response->set_header("Connection", "keep-alive");
    } else {
#else
      response->set_header("Connection", "close");
#endif
#ifdef ENABLE_KEEP_ALIVE
    }
#endif
    handler->handle_request(request, *response);
    std::ostringstream response_message;
    response->get_http_response_message(response_message);
#ifdef ENABLE_KEEP_ALIVE
    response->keep_alive = keep_alive;
#else
    response->keep_alive = false;
#endif
    // logger << Logger::debug
    //        << "Sending response"
    //        << Logger::endl
    //        << "\n---\n" << response_message.str() << "\n--"
    //        << Logger::endl;
    socket_handler.send(response_message);
    // logger << Logger::debug
    //         << "After sending response"
    //         << Logger::endl;
    return true;
  }
  // logger << Logger::debug
  //         << std::this_thread::get_id()
  //         << " Empty request" << Logger::endl;
  return false;
}

void Worker::run() {
  logger << Logger::debug << "Starting thread "
          << std::this_thread::get_id() << Logger::endl;
  while (!stop_flag) {
    try {
      int fd = -1;
#ifdef ENABLE_KEEP_ALIVE
      keep_alive = false;
#endif
      std::unique_lock<std::mutex> lock(readyMutex);
      readyCondVar.wait(lock, [&] () { return !queue.empty() || stop_flag; });
      if (!queue.empty()) {
        fd = queue.front();
        queue.pop();
        // logger << Logger::debug << std::this_thread::get_id()
        //         << " Read fd " << fd << Logger::endl;
      }
      lock.unlock();
      if (fd >= 0) {
        fdsd::web::SocketHandler handler(fd);
#ifdef ENABLE_KEEP_ALIVE
        if (handle_socket_read(handler) && keep_alive && !stop_flag) {
          // logger << Logger::debug << std::this_thread::get_id()
          //         << " Checking if more data ready (keep-alive) (" << fd << ")"
          //         << Logger::endl;
          while (!handler.is_eof() &&
                 handler.is_more_data_to_read(100) &&
                 !stop_flag) {
            // logger << Logger::debug
            //         << std::this_thread::get_id()
            //         << " Reading the extra data (keep-alive)" << Logger::endl;
            if (!handle_socket_read(handler))
              break;
          }
        }
#else
        handle_socket_read(handler);
#endif
      }
      // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      // logger << Logger::debug
      //         << std::this_thread::get_id()
      //         << " Looping..."
      //         << Logger::endl;
    } catch (const std::exception& e) {
      logger << Logger::alert << e.what() << Logger::endl;
    }
  } // while !stop_flag
  logger << Logger::debug
          << std::this_thread::get_id()
          << " Run loop finished for thread "
          << std::this_thread::get_id() << Logger::endl;
}

void Worker::start() {
  run();
}

void Worker::stop() {
  stop_flag = true;
}

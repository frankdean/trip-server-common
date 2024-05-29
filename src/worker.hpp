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
#ifndef WORKER_HPP
#define WORKER_HPP

#include "logger.hpp"
#include "db_error_handler.hpp"
#include <iostream>
#include <queue>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace fdsd
{

namespace web
{

class SocketHandler;
class HTTPRequestFactory;

extern std::mutex readyMutex;
extern std::condition_variable readyCondVar;

class Worker {
private:
  std::queue<int>& queue;
  bool stop_flag;
#ifdef ENABLE_KEEP_ALIVE
  bool keep_alive;
#endif
  static fdsd::utils::Logger logger;
  static int worker_count;
  int worker_id;
  std::shared_ptr<fdsd::utils::DbErrorHandler> db_error_handler;
  std::shared_ptr<fdsd::web::HTTPRequestFactory> request_factory;
  bool handle_socket_read(fdsd::web::SocketHandler& socket_handler);
  void run();
public:
  Worker(std::queue<int>& q,
         std::shared_ptr<fdsd::web::HTTPRequestFactory> request_factory
#ifdef HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H
         , std::shared_ptr<fdsd::utils::DbErrorHandler> db_error_handler
#endif
    );
  void start();
  void stop();
};

} // namespace web
} // namespace fdsd

#endif // WORKER_HPP

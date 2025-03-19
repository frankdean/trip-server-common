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
#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "logger.hpp"
#include "socket.hpp"
#include "db_error_handler.hpp"
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <thread>

namespace fdsd {
namespace utils { class Configuration; }
namespace web {

class HTTPRequestFactory;
class Worker;

class Application {
private:
  static fdsd::utils::Logger logger;
  std::queue<int> socket_queue;
  fdsd::web::Socket socket;
  std::vector<std::shared_ptr<fdsd::web::Worker>> workers;
  std::vector<std::thread*> worker_threads;
  /// Exit flag for run loop
  static volatile bool exit_now;
  std::unique_ptr<fdsd::utils::Configuration> config;
protected:
  virtual std::shared_ptr<HTTPRequestFactory> get_request_factory() const = 0;
public:
  Application(std::string listen_address,
              std::string port);
  virtual ~Application();
  void read_config_file(std::string config_filename);
  std::string get_config_value(std::string key, std::string default_value="");
  void initialize_locale() const;
  static void signalHandler(int signum);
  void run();
  void stop_workers() const;
  void initialize_workers(
      int count
#ifdef HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H
      , std::shared_ptr<fdsd::utils::DbErrorHandler> db_error_handler
#endif
    );
  void read_next_socket();
};

} // namespace web
} // namespace fdsd

#endif // APPLICATION_HPP

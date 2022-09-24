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
#include "application.hpp"
#include "configuration.hpp"
#include "get_options.hpp"
#include "http_request_factory.hpp"
#include "session.hpp"
#include "worker.hpp"
#include <csignal>
#ifdef HAVE_BOOST_LOCALE
#include <boost/locale.hpp>
#endif

using namespace fdsd::web;
using namespace fdsd::utils;
#ifdef HAVE_BOOST_LOCALE
using namespace boost::locale;
#endif

volatile bool Application::exit_now = false;

Logger Application::logger("application", std::clog, Logger::info);

Application::Application(std::string listen_address,
                         std::string port,
                         std::string locale_str)
  :
  socket_queue(),
  socket(listen_address, port),
  workers(),
  worker_threads(),
  document_root("./")
{
  initialize_locale(locale_str);
}

Application::~Application()
{
  stop_workers();
  // Avoid memory leaks caused by locale
  // https://stackoverflow.com/questions/34066598/memory-leak-when-setting-global-locale
  std::locale::global(std::locale("C"));
  std::cerr.imbue(std::locale("C"));
  std::cout.imbue(std::locale("C"));
  std::wcerr.imbue(std::locale("C"));
  std::wcout.imbue(std::locale("C"));
}

void Application::initialize_locale(std::string locale_str) const
{
  std::cerr.imbue(std::locale());
  std::cout.imbue(std::locale());
}

void Application::signalHandler(int signum)
{
  // std::cout << "Interrupt signal: " << signum << std::endl;
  try {
    SessionManager::get_session_manager()->persist_sessions();
  } catch (const std::exception& e) {
    std::cerr << "Exception whilst handling signal\n"
              << e.what() << Logger::endl;
  }
  exit_now = signum != 0;
  // std::cout << "Finished handling signal" << std::endl;
}

void Application::run()
{
  signal(SIGINT, Application::signalHandler);
  signal(SIGTERM, Application::signalHandler);
  while (!exit_now)
    read_next_socket();
}

void Application::read_config_file()
{
  std::string config_filename = get_config_filename();
  if (config_filename.empty()) {
    if (GetOptions::verbose_flag) {
#ifdef HAVE_BOOST_LOCALE
      // Error message shown when a configuration filename has not been specified
      std::cout << translate("Configuration filename not specified\n");
#else
      std::cout << "Configuration filename not specified\n";
#endif
    }
    config = std::unique_ptr<Configuration>(new Configuration);
  } else {
    if (GetOptions::verbose_flag) {
#ifdef HAVE_BOOST_LOCALE
      // Shows which configuration filename is being read
      std::cout << format(translate("Reading configuration from {1}")) % config_filename << '\n';
#else
      std::cout << "Reading configuration from " << config_filename << '\n';
#endif
    }
    auto start = std::chrono::system_clock::now();
    config = std::unique_ptr<Configuration>(new Configuration(config_filename));
    auto finish = std::chrono::system_clock::now();
    auto diff = finish - start;
    // if (GetOptions::verbose_flag) {
    //   std::cout << "Read configuration from " << config_filename << " in "
    //             << std::chrono::duration_cast<std::chrono::microseconds>(diff).count()
    //             << " us\n";
    // }
  }
}

std::string Application::get_config_value(
    std::string key,
    std::string default_value)
{
  if (config == nullptr)
    read_config_file();
  if (config == nullptr) {
    std::cerr << "Config is null\n";
  }
  return config->get(key, default_value);
}

void Application::stop_workers() const
{
  for (const auto worker : workers) {
    worker->stop();
  }
  readyCondVar.notify_all();
  // wait for all workers to stop
  for (auto t : worker_threads) {
    t->join();
    delete t;
  }
}

void Application::initialize_workers(int count)
{
  if (GetOptions::verbose_flag) {
#ifdef HAVE_BOOST_LOCALE
    // Shows how many worker processes are being created for the application
    std::cout << format(translate("Creating {1} worker",
                                  "Creating {1} workers",
                                  count)) % count
              << '\n';
#else
    std::cout << "Creating " << count << " worker(s)\n";
#endif
  }
  for (int i = 0; i < count; i++) {
    auto request_factory = get_request_factory();
    auto worker = std::make_shared<Worker>(
        Worker(socket_queue, request_factory));
    std::thread* t = new std::thread(&Worker::start, worker);
    worker_threads.push_back(t);
    workers.push_back(worker);
  }
}

void Application::read_next_socket()
{
  int fd;
  try {
    // logger << Logger::debug << "Wait..." << Logger::endl;
    if ((fd = socket.wait_connection(-1)) >= 0) {
      try {
        // logger << Logger::debug << "Push..." << Logger::endl;
        std::lock_guard<std::mutex> lock(readyMutex);
        socket_queue.push(fd);
        readyCondVar.notify_one();
      } catch (const std::exception& e) {
        logger << Logger::emergency
               << "Error pushing file descriptor on to queue\n"
               << e.what() << Logger::endl;
        throw e;
      }
    }
  } catch (const std::exception& e) {
    logger << Logger::emergency
           << "Error waiting for connection\n"
           << e.what() << Logger::endl;
    throw e;
  }
}

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
#include "../config.h"
#include "pg_pool.hpp"
#include "get_options.hpp"
#ifdef HAVE_BOOST_LOCALE
#include <boost/locale.hpp>
#endif
#include <syslog.h>

#ifdef HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H

using namespace fdsd::utils;
using namespace pqxx;
#ifdef HAVE_BOOST_LOCALE
using namespace boost::locale;
#endif

PgPoolManager::PgPoolManager(std::string connect_string, int pool_size)
  : DbErrorHandler(), connect_string(connect_string), mutex(), ready()
{
  if (GetOptions::verbose_flag) {
#ifdef HAVE_BOOST_LOCALE
    // Shows how many database connections are being created for the application
    std::cout << boost::locale::format(
        translate("Creating database pool with {1} connection",
                  "Creating database pool with {1} connections",
                  pool_size)) % pool_size
              << '\n';
#else
    std::cout << "Creating database pool with "
              << pool_size
              << " connection" << (pool_size != 1 ? "s" : "") << "\n";
#endif
  }

  for (int i = 0; i < pool_size; i++) {
#ifdef HAVE_LIBPQXX7
    queue.emplace(std::make_shared<connection>(connect_string));
#else
    queue.emplace(std::make_shared<lazyconnection>(connect_string));
#endif
  }
}

#ifdef HAVE_LIBPQXX7
void PgPoolManager::free_connection(std::shared_ptr<connection> connection)
#else
void PgPoolManager::free_connection(std::shared_ptr<lazyconnection> connection)
#endif
{
  std::unique_lock<std::mutex> lock(mutex);
  queue.push(connection);
  // Notify the next connection that is potentially waiting for a connection
  // to become free.
  lock.unlock();
  ready.notify_one();
}

#ifdef HAVE_LIBPQXX7
std::shared_ptr<connection> PgPoolManager::get_connection()
#else
std::shared_ptr<lazyconnection> PgPoolManager::get_connection()
#endif
{
  std::unique_lock<std::mutex> lock(mutex);
  while (queue.empty())
    ready.wait(lock);

  auto connection = queue.front();
  queue.pop();
  return connection;
}

void PgPoolManager::refresh_connections()
{
  syslog(LOG_INFO, "Refreshing database connection pool");

  std::unique_lock<std::mutex> lock(mutex);
  // The queue could be empty if all the connections are being held by workers.
  // Wait until at least one connection is available to refresh.
  while (queue.empty())
    ready.wait(lock);

  auto size = queue.size();
  for (std::queue<std::shared_ptr<pqxx::connection>>::size_type i = 0; i < size; i++) {
    queue.pop();
#ifdef HAVE_LIBPQXX7
    queue.emplace(std::make_shared<connection>(connect_string));
#else
    queue.emplace(std::make_shared<lazyconnection>(connect_string));
#endif
  }
  if (size == 1)
    syslog(LOG_INFO, "Refreshed %lu database connection", size);
  else
    syslog(LOG_INFO, "Refreshed %lu database connections", size);
}

void PgPoolManager::handle_broken_connection()
{
  refresh_connections();
}

#endif // HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H

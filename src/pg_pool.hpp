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
#ifndef POOL_HPP
#define POOL_HPP

#ifdef HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H

#include "db_error_handler.hpp"
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <queue>
#include <string>

namespace fdsd
{
namespace utils
{

class PgPoolManager : public fdsd::utils::DbErrorHandler {
public:
  PgPoolManager(std::string connect_string, int pool_size = 10);
  virtual ~PgPoolManager() {}
#ifdef HAVE_LIBPQXX7
  void free_connection(std::shared_ptr<pqxx::connection> connection);
  std::shared_ptr<pqxx::connection> get_connection();
#else
  void free_connection(std::shared_ptr<pqxx::lazyconnection> connection);
  std::shared_ptr<pqxx::lazyconnection> get_connection();
#endif
  void refresh_connections();
  virtual void handle_broken_connection() override;
private:
  std::string connect_string;
  std::mutex mutex;
  std::condition_variable ready;
#ifdef HAVE_LIBPQXX7
  std::queue<std::shared_ptr<pqxx::connection>> queue;
#else
  std::queue<std::shared_ptr<pqxx::lazyconnection>> queue;
#endif
};

} // namespace utils
} // namespace fdsd

#endif // HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H

#endif // POOL_HPP

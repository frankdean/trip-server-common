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
#include <chrono>
#include <iostream>
#include <thread>
#include "pg_pool.cpp"
#include "get_options.cpp"

#ifdef HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H

// Tests the pool manager will wait for a connection to be freed when the pool
// is empty.
bool test_exhausting_pool()
{
  PgPoolManager* m = new PgPoolManager("", 5);
  auto c1 = m->get_connection();
  auto c2 = m->get_connection();
  auto c3 = m->get_connection();
  auto c4 = m->get_connection();
  auto c5 = m->get_connection();
  std::thread t(
      [m, c1, c5] {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        m->free_connection(c5);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        m->free_connection(c1);
      }
    );
  const auto start = std::chrono::system_clock::now();
  const auto c6 = m->get_connection();
  const auto c7 = m->get_connection();
  const auto finish = std::chrono::system_clock::now();
  const auto diff = finish - start;
  const auto ms_delay = std::chrono::duration_cast<std::chrono::milliseconds>(diff)
    .count();
  // std::cout
  //   << "Connection retrieved in "
  //   << ms_delay
  //   << " ms\n";
  t.join();
  delete m;
  // Fetching the sixth connection should have been delayed by at least 10ms.
  // Conservatively, we'll just ensure it was at least close to that.
  return ms_delay > 7;
}
#endif // HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H

int main(void)
{
#ifdef HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H
  return !(
      test_exhausting_pool()
    );
#else
  return 0;
#endif // HAVE_PQXX_CONFIG_PUBLIC_COMPILER_H
}


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
#include "dao_helper.cpp"
#include <iostream>
#include <string>

using namespace fdsd::utils;

bool test_convert_tz_01()
{
  const std::string test_date = "2020-10-09 14:14:42.421+01";
  const long long expected = 1602249282421;
  const auto tp = dao_helper::convert_libpq_date_tz(test_date);
  const long long result =
    std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()).count();
  const long long diff = result - expected;
  const bool retval = diff == 0;
  if (!retval)
    std::cout << "test_convert_tz_01() failed, diff: " << diff << '\n';
  return retval;
}

bool test_convert_tz_02()
{
  const std::string test_date = "2020-06-21 14:14:42.421+00";
  const long long expected = 1592748882421;
  try {
    const auto tp = dao_helper::convert_libpq_date_tz(test_date);
    const long long result =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          tp.time_since_epoch()).count();
    const long long diff = result - expected;
    const bool retval = diff == 0;
    if (!retval)
      std::cout << "test_convert_tz_02() failed, diff: " << diff << '\n';
    return retval;
  } catch (const std::invalid_argument& e) {
    std::cout << "test_convert_tz_02() exception: " << e.what() << '\n';
    return false;
  }
}

bool test_trim()
{
  const std::string head = "  \t\t  ";
  const std::string tail = " \r\n\t   ";
  const std::string expected("1 2   3 4 5");
  const std::string test = head + expected + tail;
  std::string left_test = test;
  std::string right_test = test;
  std::string all_test = test;
  dao_helper::ltrim(left_test);
  dao_helper::rtrim(right_test);
  dao_helper::trim(all_test);
  const bool retval =
    left_test == expected + tail &&
    right_test == head + expected &&
    all_test == expected;
  return retval;
}

int main(void)
{ return !(
      test_convert_tz_01()
      && test_convert_tz_02()
      && test_trim()
    );
}

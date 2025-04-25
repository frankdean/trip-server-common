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
#include "uuid.cpp"

// #include <iostream>

bool test_length()
{
  const std::string uuid_str = UUID::generate_uuid();
  // std::cout << "UUID length is: " << uuid_str.length() << " characters\n";
  return uuid_str.length() == 36;
}

bool test_uniqueness()
{
  const std::string s1 = UUID::generate_uuid();
  const std::string s2 = UUID::generate_uuid();
  return s1 != s2;
}

bool test_invalid_uuid()
{
  return !UUID::is_valid("Some rubbish");
}

bool test_uuid_invalid_length()
{
  return !UUID::is_valid("49eb81a8-ed9c-11ec-9d3b-0800278dc04dXX");
}

bool test_valid_uuid()
{
  std::string s1 = UUID::generate_uuid();
  // std::cout << "Generated UUID to test for validity: " << s1 << '\n';
  return UUID::is_valid(s1);
}

int main(void)
{
  try {
    return !(
        test_length()
        && test_uniqueness()
        && test_valid_uuid()
        && test_invalid_uuid()
        && test_uuid_invalid_length()
      );
  } catch (const std::exception &e) {
    std::cerr << "Tests failed with: " << e.what() << '\n';
    return 1;
  }
}

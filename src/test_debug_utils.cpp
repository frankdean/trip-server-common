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
#include <iostream>
#include <sstream>
#include "debug_utils.cpp"

const std::string test_string = "Test string for formatted debug dump";

bool test_hex_dump_34()
{
  const std::string s = test_string.substr(0, 34);
  std::ostringstream os;
  DebugUtils::hex_dump(s, os);
  const std::string expected = "00000000: 5465 7374 2073 7472 696e 6720 666f 7220  Test string for \n00000010: 666f 726d 6174 7465 6420 6465 6275 6720  formatted debug \n00000020: 6475                                     du\n";
  const bool retval = expected == os.str();
  if (!retval) {
    std::cout << "Expected: \"" << expected << "\"\n";
    std::cout << "Actual:   \"" << os.str() << "\"\n";
  }
  return retval;
}

bool test_hex_dump_33()
{
  const std::string s = test_string.substr(0, 33);
  std::ostringstream os;
  DebugUtils::hex_dump(s, os);
  const std::string expected = "00000000: 5465 7374 2073 7472 696e 6720 666f 7220  Test string for \n00000010: 666f 726d 6174 7465 6420 6465 6275 6720  formatted debug \n00000020: 64                                       d\n";
  const bool retval = expected == os.str();
  if (!retval) {
    std::cout << "Expected: \"" << expected << "\"\n";
    std::cout << "Actual:   \"" << os.str() << "\"\n";
  }
  return retval;
}
bool test_hex_dump_32()
{
  const std::string s = test_string.substr(0, 32);
  std::ostringstream os;
  DebugUtils::hex_dump(s, os);
  const std::string expected = "00000000: 5465 7374 2073 7472 696e 6720 666f 7220  Test string for \n00000010: 666f 726d 6174 7465 6420 6465 6275 6720  formatted debug \n";
  const bool retval = expected == os.str();
  if (!retval) {
    std::cout << "Expected: \"" << expected << "\"\n";
    std::cout << "Actual:   \"" << os.str() << "\"\n";
  }
  return retval;
}

bool test_hex_dump_31()
{
  const std::string s = test_string.substr(0, 31);
  std::ostringstream os;
  DebugUtils::hex_dump(s, os);
  const std::string expected = "00000000: 5465 7374 2073 7472 696e 6720 666f 7220  Test string for \n00000010: 666f 726d 6174 7465 6420 6465 6275 67    formatted debug\n";
  bool retval = expected == os.str();
  if (!retval) {
    std::cout << "Expected: \"" << expected << "\"\n";
    std::cout << "Actual:   \"" << os.str() << "\"\n";
  }
  return retval;
}

bool test_hex_dump_30()
{
  const std::string s = test_string.substr(0, 30);
  std::ostringstream os;
  DebugUtils::hex_dump(s, os);
  const std::string expected = "00000000: 5465 7374 2073 7472 696e 6720 666f 7220  Test string for \n00000010: 666f 726d 6174 7465 6420 6465 6275       formatted debu\n";
  const bool retval = expected == os.str();
  if (!retval) {
    std::cout << "Expected: \"" << expected << "\"\n";
    std::cout << "Actual:   \"" << os.str() << "\"\n";
  }
  return retval;
}

bool test_hex_dump()
{
  const std::string s = test_string;
  std::ostringstream os;
  DebugUtils::hex_dump(s, os);
  const std::string expected = "00000000: 5465 7374 2073 7472 696e 6720 666f 7220  Test string for \n00000010: 666f 726d 6174 7465 6420 6465 6275 6720  formatted debug \n00000020: 6475 6d70                                dump\n";
  const bool retval = expected == os.str();
  if (!retval) {
    std::cout << "Expected: \"" << expected << "\"\n";
    std::cout << "Actual:   \"" << os.str() << "\"\n";
  }
  return retval;
}

int main(void)
{
  const int retval =
    !(
        test_hex_dump()
        && test_hex_dump_30()
        && test_hex_dump_31()
        && test_hex_dump_32()
        && test_hex_dump_33()
        && test_hex_dump_34()
      );
  return retval;
}

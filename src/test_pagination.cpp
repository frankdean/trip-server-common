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
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include "pagination.cpp"
#include "debug_utils.hpp"

bool test_page_count()
{
  Pagination p;
  const Pagination p0("", 0);
  p.set_items_per_page(20);
  const Pagination p1("", 1, 20);
  const Pagination p19("", 19, 20);
  const Pagination p20("", 20, 20);
  const Pagination p21("", 21, 20);
  const Pagination p39("", 39, 20);
  const Pagination p40("", 40, 20);
  const Pagination p41("", 41, 20);
  const Pagination p99("", 99, 20);
  const Pagination p100("", 100, 20);
  const Pagination p101("", 101, 20);
  // std::cout << p.get_page_count() << " pages of max "
  // << (std::numeric_limits<std::uint32_t>::max() / 20) << std::endl;
  const bool retval =
    p.get_page_count() == (std::numeric_limits<std::uint32_t>::max() - 1) / 20 + 1
    && p0.get_page_count() == 0
    && p1.get_page_count() == 1
    && p19.get_page_count() == 1
    && p20.get_page_count() == 1
    && p21.get_page_count() == 2
    && p39.get_page_count() == 2
    && p40.get_page_count() == 2
    && p41.get_page_count() == 3
    && p99.get_page_count() == 5
    && p100.get_page_count() == 5
    && p101.get_page_count() == 6
    ;
  if (!retval)
    std::cerr << "test_page_count() failed\n";
  return retval;
}

bool test_offset_00()
{
  const Pagination p(200);
  // std::cout << "Offset: " << p.get_offset() << std::endl;
  const bool retval =
    p.get_offset() == 0;
  if (!retval)
    std::cerr << "test_offset_00() failed\n";
  return retval;
}

bool test_offset_01()
{
  Pagination p(200);
  p.set_current_page(1);
  const bool retval = p.get_offset() == 0;
  if (!retval)
    std::cerr << "test_offset_01() failed\n";
  return retval;
}

bool test_offset_02()
{
  Pagination p("", 200, 20);
  p.set_current_page(2);
  const bool retval = p.get_offset() == 20;
  if (!retval)
    std::cerr << "test_offset_02() failed\n";
  return retval;
}

bool test_offset_03()
{
  Pagination p("", 200, 20);
  p.set_current_page(p.get_page_count());
  const bool retval =  p.get_offset() == 180;
  if (!retval)
    std::cerr << "test_offset_03() failed\n";
  return retval;
}

bool test_get_next_page_01()
{
  Pagination p;
  p.next();
  const bool retval =
    p.get_current_page() == 2;
  if (!retval)
    std::cerr << "test_get_next_page_01() failed\n";
  return retval;
}

bool test_get_next_page_02()
{
  Pagination p("", 21, 10);
  long p2 = p.next();
  long p3 = p.next();
  long p4 = p.next();
  const bool retval =
    p2 == 2
    && p3 == 3
    && p4 == 3;
  if (!retval)
    std::cerr << "test_get_next_page_01() failed\n";
  return retval;
}

// Tests attempting to get page zero, should get page one
bool test_get_previous_page_00()
{
  Pagination p;
  p.previous();
  p.previous();
  const std::uint32_t current_page = p.get_current_page();
  const bool current_page_ok = current_page == 1;
  if (!current_page_ok)
    std::cout << "test_get_previous_page_00() expected page number to be one, "
      "but was" << current_page << '\n';

  const std::uint32_t offset = p.get_offset();
  const bool offset_ok = offset == 0;
  if (!offset_ok)
    std::cout << "test_get_previous_page_00() expected offset to be zero, "
      "but was" << offset << '\n';
  const bool retval = current_page_ok && offset_ok;
  if (!retval)
    std::cerr << "test_get_previous_page_00() failed\n";
  return retval;
}

bool test_get_previous_page_01()
{
  Pagination p;
  p.previous();
  const bool retval =
    p.get_current_page() == 1;
  if (!retval)
    std::cerr << "test_get_previous_page_01() failed\n";
  return retval;
}

bool test_get_previous_page_02()
{
  Pagination p;
  p.set_current_page(99);
  p.previous();
  // std::cout << "Previous page is " << p.get_current_page() << '\n';
  const bool retval =
    p.get_current_page() == 98;
  if (!retval)
    std::cerr << "test_get_previous_page_02() failed\n";
  return retval;
}

// Potentially this test may fail on a platform that implements unsigned long with more than 32 bits
bool test_get_next_previous_page()
{
  std::uint32_t max_ulong = std::numeric_limits<std::uint32_t>::max();
  Pagination p("", max_ulong, 1);
  p.set_current_page(max_ulong);
  const long r1 = p.get_current_page();
  p.next();
  p.next();
  p.next();
  const long r2 = p.next();
  const long r3 = p.previous();
  const bool retval =
    r1 == max_ulong
    && r2 == max_ulong
    && r3 == max_ulong - 1;
  if (!retval)
    std::cerr << "test_get_next_previous_page() failed\n";
  return retval;
}

// Tests attempting to navigate beyond the last page
bool test_get_last_page_plus_one()
{
  bool retval = true;
  Pagination p("", 101, 20);
  const std::uint32_t expected_page_count = 6;
  const std::uint32_t page_count = p.get_page_count();
  const bool page_count_ok = page_count == expected_page_count;
  if (!page_count_ok) {
    retval = false;
    std::cout << "test_get_last_page_plus_one() expected "
              << expected_page_count << " but was " << page_count << " pages\n";
  }

  // Navigate to the last page
  p.set_current_page(expected_page_count);
  std::uint32_t current_page_number = p.get_current_page();
  if (current_page_number != expected_page_count) {
    retval =false;
    std::cout << "test_get_last_page_plus_one() expected last page to be "
              << expected_page_count << " but was " << current_page_number << '\n';
  }
  const uint32_t expected_last_page_offset = 100;
  uint32_t offset = p.get_offset();
  if (offset != expected_last_page_offset) {
    retval = false;
    std::cout << "test_get_last_page_plus_one() after setting last page, "
              << "expected offset to be "
              << expected_last_page_offset
              <<" but was" << offset << '\n';
  }

  // Trying to navigate past the last page
  p.next();
  current_page_number = p.get_current_page();
  if (current_page_number != expected_page_count) {
    retval =false;
    std::cout << "test_get_last_page_plus_one() expected after attempting to "
              << "navigate beyond last page to be "
              << expected_page_count << " but was "
              << current_page_number << '\n';
  }
  offset = p.get_offset();
  if (offset != expected_last_page_offset) {
    retval = false;
    std::cout << "test_get_last_page_plus_one() expected offset after "
              << "attempting to navigate beyond the last page, to be "
              << expected_last_page_offset
              <<" but was" << offset << '\n';
  }
  // Set the page number beyond the last page
  std::cout << "Setting page number to " << (expected_page_count + 1) << '\n';
  p.set_current_page(expected_page_count + 1);
  current_page_number = p.get_current_page();
  if (current_page_number != expected_page_count) {
    retval =false;
    std::cout << "test_get_last_page_plus_one() after attempting to "
              << "set last page beyond the maximum, expected "
              << expected_page_count << " but was "
              << current_page_number << '\n';
  }
  offset = p.get_offset();
  if (offset != expected_last_page_offset) {
    retval = false;
    std::cout << "test_get_last_page_plus_one() expected offset after "
              << "attempting to navigate beyond the last page, to be "
              << expected_last_page_offset
              <<" but was" << offset << '\n';
  }

  return retval;
}

// Setting the total should update the current page number and the offset.
bool test_set_total()
{
  bool retval = true;
  Pagination (p);
  p.set_items_per_page(20);
  const uint32_t expected_page_count = 6;
  const std::uint32_t unlimited_page_count =
    std::numeric_limits<std::uint32_t>::max();
  const std::uint32_t page_count = p.get_page_count();
  const bool page_count_ok = page_count > expected_page_count;
  if (!page_count_ok) {
    retval = false;
    std::cout << "test_set_total() expected unlimited page count to be "
              << unlimited_page_count << " but was " << page_count << " pages\n";
  }

  // Main test
  p.set_current_page(expected_page_count);
  p.set_total(101);
  std::uint32_t current_page_number = p.get_current_page();
  if (current_page_number != expected_page_count) {
    retval =false;
    std::cout << "test_set_total() expected last page to be "
              << expected_page_count << " but was " << current_page_number << '\n';
  }
  const std::uint32_t expected_offset = 100;
  const std::uint32_t offset = p.get_offset();
  if (expected_offset != offset) {
    std::cout << "test_set_total() expected offset to be "
              << expected_offset << " but was " << offset << '\n';
  }
  return retval;
}

bool test_get_html() {
  std::stringstream os;
  Pagination p("./test/?page=%u", 100, 10);
  p.set_current_page(6);
  p.get_html(os);
  int pages = 0;
  int previous = 0;
  int next = 0;
  char buf[255];
  const std::regex page_regex(".*page=\\d+.*page </span>\\d+.*");
  const std::regex previous_regex(".*previous page.*");
  const std::regex next_regex(".*next page.*");
  while (os.getline(&buf[0], sizeof(buf) -1)) {
    // std::cout << buf << '\n';
    if (std::regex_match(buf, page_regex))
      pages++;
    if (std::regex_match(buf, previous_regex))
      previous++;
    if (std::regex_match(buf, next_regex))
      next++;
  }
  // std::cout << pages << " pages\n";
  const int expected_page_count = 7;
  const bool retval = pages == expected_page_count &&
    next == 1 &&
    previous == 1;
  if (!retval) {
    std::cerr << "Failed test_get_html\n";
    if (pages != expected_page_count)
      std::cerr << "Expected " << expected_page_count
                << " pages, but there were "
                << pages << " pages\n";
    if (next != 1)
      std::cerr << "Next page button is missing\n";
    if (previous != 1)
      std::cerr << "Previous page button is missing\n";
  }
  return retval;
}

bool test_get_html_button_count_1_page()
{
  std::stringstream os;
  const Pagination p("./test/?page=%u", 20, 20);
  p.get_html(os);
  // std::cout << os.str() << '\n';
  int pages = 0;
  int previous = 0;
  int next = 0;
  char buf[255];
  const std::regex page_regex(".*page=\\d+.*page </span>\\d+.*");
  const std::regex previous_regex(".*disabled.*previous page.*");
  const std::regex next_regex(".*disabled.*next page.*");
  while (os.getline(&buf[0], sizeof(buf) -1)) {
    // std::cout << buf << '\n';
    if (std::regex_match(buf, page_regex))
      pages++;
    if (std::regex_match(buf, previous_regex))
      previous++;
    if (std::regex_match(buf, next_regex))
      next++;
  }
  // std::cout << pages << " pages\n";
  const bool retval = pages == 0 &&
    next == 0 &&
    previous == 0;
  if (!retval)
    std::cerr << "Failed test_get_html_button_count_1_page\n";
  return retval;
}

bool test_get_html_button_count_3_pages()
{
  std::stringstream os;
  const Pagination p("./test/?page=%u", 58, 20);
  p.get_html(os);
  // std::cout << os.str() << '\n';
  int pages = 0;
  int previous = 0;
  int next = 0;
  char buf[255];
  const std::regex page_regex(".*page=\\d+.*page </span>\\d+.*");
  const std::regex previous_regex(".*disabled.*previous page.*");
  const std::regex next_regex(".*next page.*");
  while (os.getline(&buf[0], sizeof(buf) -1)) {
    // std::cout << buf << '\n';
    if (std::regex_match(buf, page_regex))
      pages++;
    if (std::regex_match(buf, previous_regex))
      previous++;
    if (std::regex_match(buf, next_regex))
      next++;
  }
  // std::cout << pages << " pages\n";
  const int expected_page_count = 3;
  const bool retval = pages == expected_page_count &&
    next == 1 &&
    previous == 1;
  if (!retval) {
    std::cerr << "Failed test_get_html_button_count_3_pages \n";
    if (pages != expected_page_count)
      std::cerr << "Expected " << expected_page_count
                << " pages, but there were "
                << pages << " pages\n";
    if (next != 1)
      std::cerr << "Next page button is missing\n";
    if (previous != 1)
      std::cerr << "Previous page button is missing\n";
  }
  return retval;
}

bool test_get_html_02()
{
  const std::string page_url = "/foo/bar";
  std::stringstream os;
  std::map<std::string, std::string> query_params = {
    {"key1", "value1"},
    {"key2", "value2"},
    {"key3", "value3"},
  };
  Pagination p(page_url, query_params, 999, 10, 5, true, true, "pagina");
  p.set_current_page(6);
  p.get_html(os);
  // std::cout << "HTML:\n" << os.str() << '\n';;
  int pages = 0;
  int previous = 0;
  int next = 0;
  char buf[255];
  const std::regex page_regex(".*href=\"" + page_url + "\\?key1=value1&key2=value2&key3=value3&pagina=\\d+.*page </span>\\d+.*");
  const std::regex previous_regex(".*previous page.*");
  const std::regex next_regex(".*next page.*");
  while (os.getline(&buf[0], sizeof(buf) -1)) {
    // std::cout << buf << '\n';
    if (std::regex_match(buf, page_regex))
      pages++;
    if (std::regex_match(buf, previous_regex))
      previous++;
    if (std::regex_match(buf, next_regex))
      next++;
  }
  // std::cout << pages << " pages\n";
  const int expected_page_count = 7;
  const bool retval = pages == expected_page_count &&
    next == 1 &&
    previous == 1;
  if (!retval) {
    std::cerr << "Failed test_get_html_02\n";
    if (pages != expected_page_count)
      std::cerr << "Expected " << expected_page_count
                << " pages, but there were "
                << pages << " pages\n";
    if (next != 1)
      std::cerr << "Next page button is missing\n";
    if (previous != 1)
      std::cerr << "Previous page button is missing\n";
  }
  return retval;
}

bool test_get_html_03()
{
  const std::string page_url = "/foo/bar";
  std::stringstream os;
  std::map<std::string, std::string> query_params = {};
  Pagination p(page_url, query_params);
  p.set_current_page(6);
  p.get_html(os);
  // std::cout << "HTML:\n" << os.str() << '\n';;
  int pages = 0;
  int previous = 0;
  int next = 0;
  char buf[255];
  const std::regex page_regex(".*href=\"" + page_url + "\\?page=\\d+.*page </span>\\d+.*");
  const std::regex previous_regex(".*previous page.*");
  const std::regex next_regex(".*next page.*");
  while (os.getline(&buf[0], sizeof(buf) -1)) {
    // std::cout << buf << '\n';
    if (std::regex_match(buf, page_regex))
      pages++;
    if (std::regex_match(buf, previous_regex))
      previous++;
    if (std::regex_match(buf, next_regex))
      next++;
  }
  // std::cout << pages << " pages\n";
  const int expected_page_count = 7;
  const bool retval = pages == expected_page_count &&
    next == 1 &&
    previous == 1;
  if (!retval) {
    std::cerr << "Failed test_get_html_03\n";
    if (pages != expected_page_count)
      std::cerr << "Expected " << expected_page_count
                << " pages, but there were "
                << pages << " pages\n";
    if (next != 1)
      std::cerr << "Next page button is missing\n";
    if (previous != 1)
      std::cerr << "Previous page button is missing\n";
  }
  return retval;
}

bool test_get_html_large_page_numbers()
{
  const std::string page_url = "/foo/bar";
  std::stringstream os;
  std::map<std::string, std::string> query_params = {};
  Pagination p(page_url, query_params, 60000, 20);
  p.set_current_page(2123);
  p.get_html(os);
  std::cout << "HTML:\n" << os.str() << '\n';;
  int pages = 0;
  int previous = 0;
  int next = 0;
  int current_page = 0;
  char buf[255];
  const std::regex current_page_regex(".*href=\"" + page_url + "\\?page=2123.*aria-current=\"page\"><span.*>page </span>2123</a>.*");
  const std::regex page_regex(".*href=\"" + page_url + "\\?page=\\d+.*page </span>\\d+.*");
  const std::regex previous_regex(".*previous page.*");
  const std::regex next_regex(".*next page.*");
  while (os.getline(&buf[0], sizeof(buf) -1)) {
    // std::cout << buf << '\n';
    if (std::regex_match(buf, current_page_regex))
      current_page++;
    if (std::regex_match(buf, page_regex))
      pages++;
    if (std::regex_match(buf, previous_regex))
      previous++;
    if (std::regex_match(buf, next_regex))
      next++;
  }
  // std::cout << pages << " pages\n";
  const int expected_page_count = 7;
  const bool retval = pages == expected_page_count &&
    current_page == 1 &&
    next == 1 &&
    previous == 1;
  if (!retval) {
    std::cerr << "Failed test_get_html_large_page_numbers()\n";
    if (pages != expected_page_count)
      std::cerr << "Expected " << expected_page_count
                << " pages, but there were "
                << pages << " pages\n";
    if (next != 1)
      std::cerr << "Next page button is missing\n";
    if (previous != 1)
      std::cerr << "Previous page button is missing\n";
  }
  return retval;
}

int main(void)
{
  try {
    return !(
        test_get_html()
        && test_get_html_button_count_1_page()
        && test_get_html_button_count_3_pages()
        && test_page_count()
        && test_offset_00()
        && test_offset_01()
        && test_offset_02()
        && test_offset_03()
        && test_get_next_page_01()
        && test_get_next_page_02()
        && test_get_previous_page_00()
        && test_get_previous_page_01()
        && test_get_previous_page_02()
        && test_get_next_previous_page()
        && test_set_total()
        && test_get_last_page_plus_one()
        && test_get_html_02()
        && test_get_html_03()
        && test_get_html_large_page_numbers()
      );
  } catch (const std::exception &e) {
    std::cerr << "Tests failed with: " << e.what() << '\n';
    return 1;
  }
}

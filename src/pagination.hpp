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
#ifndef PAGINATION_HPP
#define PAGINATION_HPP

#include "logger.hpp"
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>

namespace fdsd
{
namespace web
{

class Pagination {
private:
  static fdsd::utils::Logger logger;
  /// True shows the first and last page numbers
  bool m_show_first_last;
  /// True shows the previous and next indicators
  bool m_show_prev_next;
  /// Total number of items.  A negative total indicates an unknown number of
  /// items.
  std::uint32_t m_total;
  /// The current page number
  std::uint32_t m_current_page;
  /// The number of pages
  std::uint32_t m_page_count;
  /// The number of items per page
  unsigned int m_items_per_page;
  /// The number of buttons to create
  unsigned int m_page_button_count;
  /// The first page in the current range
  std::int32_t m_begin_range;
  /// The last page in the current range
  std::int32_t m_end_range;
  /// The format string to use to create the href containing the page number.
  /// Deprecated, use m_page_number_query_param_key instead.
  std::string m_href_url;
  /// The url to the page up to but not including the query parameter
  /// separator `?`.
  std::string m_page_url;
  /// A map of query parameters as key-value pairs.
  std::map<std::string, std::string> m_query_params;
  /// The key of the query parameter to associate with each page number.
  std::string m_page_number_query_param_key;
  void update_page_ranges() ;
  bool show_first_page() const {
    return m_show_first_last && m_begin_range > 1;
  }
  bool show_last_page() const {
    return m_show_first_last && m_end_range < m_page_count;
  }
public:
  Pagination() : Pagination("") {}
  Pagination(std::uint32_t total) : Pagination("", total) {}
  // Deprecated - use the constructor below that takes the query params
  Pagination(std::string href_url_format_string,
             std::uint32_t total = std::numeric_limits<std::uint32_t>::max(),
             unsigned int items_per_page = 10,
             unsigned int buttons = 5,
             bool show_first_last = true,
             bool show_prev_next = true);
  Pagination(std::string page_url,
             std::map<std::string, std::string> query_params,
             std::uint32_t total = std::numeric_limits<std::uint32_t>::max(),
             unsigned int items_per_page = 10,
             unsigned int buttons = 5,
             bool show_first_last = true,
             bool show_prev_next = true,
             std::string page_number_query_param_key = "page");
  std::string get_html() const;
  virtual void get_html(std::ostream& os) const;
  void append_page_url(std::ostream& os, std::uint32_t page_number) const;
  void set_total(std::uint32_t total) {
    m_total = total;
    if (m_total == 0) {
      m_page_count = 0;
    } else {
      m_page_count = (m_total - 1) / m_items_per_page + 1;
      // std::cout << "Set page count to " << m_page_count
      //           << " based on items per page\n";
    }
    // std::cout << "Init\n";
    update_page_ranges();
  }
  std::uint32_t get_offset() const {
    return m_items_per_page * (m_current_page - 1);
  }
  unsigned int get_limit() const {
    return m_items_per_page;
  }
  std::uint32_t get_page_count() const {
    // std::cout << "get_page_count() " << m_page_count << '\n';
    return m_page_count;
  }
  std::uint32_t previous();
  std::uint32_t next();
  std::uint32_t get_current_page() const { return m_current_page; }
  void set_current_page(std::uint32_t current_page);
  void show_first_last(bool show) {
    m_show_first_last = show;
  }
  void show_prev_next(bool show) {
    m_show_prev_next = show;
  }
  void set_items_per_page(unsigned int count) {
    m_items_per_page = count;
    // Recalculate number of pages
    set_total(m_total);
  }
};

} // namespace web
} // namespace fdsd

#endif // PAGINATION_HPP

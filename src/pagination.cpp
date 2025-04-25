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
#include "pagination.hpp"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace fdsd::utils;
using namespace fdsd::web;

Logger Pagination::logger("pagination", std::clog, Logger::info);

Pagination::Pagination(
    std::string href_url_format_string,
    std::uint32_t total,
    std::uint32_t items_per_page,
    std::uint32_t buttons,
    bool show_first_last,
    bool show_prev_next) :
  m_show_first_last(show_first_last),
  m_show_prev_next(show_prev_next),
  m_current_page(1),
  m_page_count(0),
  m_items_per_page(items_per_page),
  m_page_button_count(buttons),
  m_begin_range(1),
  m_end_range(buttons),
  m_href_url(href_url_format_string),
  m_page_url(),
  m_query_params(),
  m_page_number_query_param_key()
{
  set_total(total);
}

/**
 * Constructor.  Primarily taking a map of query parameters.
 *
 * \param page_url the url to the page up to but not including the query
 * parameter separator `?`.
 * \param query_params a map of query parameters as key-value pairs.
 * \param page_number_query_param_key the key of the query parameter to
 * associate with each page number.
 * \param total the total number of items.  A negative total indicates an
 * unknown number of items.
 * \param items_per_page the number of items per page.
 * \param buttons the number of buttons to create.
 * \param show_first_last if true, shows buttons for both the first and last
 * pages.
 * \param show_prev_next if true, shows buttons for the previous and next
 * pages, relative to the current page.
 */
Pagination::Pagination(
    std::string page_url,
    std::map<std::string, std::string> query_params,
    std::uint32_t total,
    std::uint32_t items_per_page,
    std::uint32_t buttons,
    bool show_first_last,
    bool show_prev_next,
    std::string page_number_query_param_key) :
  m_show_first_last(show_first_last),
  m_show_prev_next(show_prev_next),
  m_current_page(1),
  m_page_count(0),
  m_items_per_page(items_per_page),
  m_page_button_count(buttons),
  m_begin_range(1),
  m_end_range(buttons),
  m_href_url(),
  m_page_url(page_url),
  m_query_params(query_params),
  m_page_number_query_param_key(page_number_query_param_key)
{
  set_total(total);
}

void Pagination::update_page_ranges()
{
  // std::cout << "\nupdate ranges for page " << m_current_page << '\n';
  // std::cout << "adjusted button count is " << m_page_button_count << '\n';
  std::int32_t begin_range = m_current_page - m_page_button_count / 2;
  m_end_range = m_current_page + m_page_button_count / 2;
  if (begin_range < 1) {
    m_begin_range = 1;
    m_end_range = m_page_button_count;
  } else {
    m_begin_range = begin_range;
  }
  if (m_end_range > m_page_count) {
    m_end_range = m_page_count;
    begin_range = m_end_range - m_page_button_count;
    if (begin_range < 1)
      m_begin_range = 1;
    else
      m_begin_range = begin_range;
  }
  // std::cout << "after update ranges current page is " << m_current_page << '\n'
  //           << "range is between " << m_begin_range << " and "
  //           << m_end_range << '\n';
}

std::uint32_t Pagination::previous()
{
  // std::cout << "previous() Current page: " << m_current_page << '\n';
  if (m_current_page > 1)
    m_current_page--;
  // std::cout << "previous() Current page after: " << m_current_page << '\n';
  update_page_ranges();
  // std::cout << "previous() Current page after update: " << m_current_page << '\n';
  return m_current_page;
}

std::uint32_t Pagination::next() {
  // std::cout << "next() 01" << m_current_page << '\n';
  if (m_current_page < m_page_count)
    m_current_page++;
  // std::cout << "next() 02" << m_current_page << '\n';
  update_page_ranges();
  return m_current_page;
}

void Pagination::set_current_page(std::uint32_t current_page)
{
  // std::cout << "Set current page to [01] " << current_page << " of "
  //           << m_page_count << " pages\n";
  if (current_page > 0 && current_page <= m_page_count) {
    m_current_page = current_page;
    // std::cout << "Set current page to [02] " << m_current_page << '\n';
  } else {
    m_current_page = current_page > m_page_count ? m_page_count : 1;
    // std::cout << "Set current page to [03] " << m_current_page << '\n';
  }
  update_page_ranges();
  // std::cout << "Set current page to [04] " << m_current_page << '\n';
}

/**
 * Appends the full URL to the passed stream, using one of two methods.  If
 * `m_href_url` is not empty, that is used as a format string to create the
 * query element of the URL.  Otherwise, a map of key-value query parameters
 * is used to construct the query element and the value of
 * `m_page_number_query_param_key` is used as the query parameter key of the
 * page number.
 */
void Pagination::append_page_url(std::ostream& os,
                                  std::uint32_t page_number) const
{
  if (!m_href_url.empty()) {
    int buf_size = std::snprintf(nullptr, 0, m_href_url.c_str(), page_number);
    if (buf_size > 0) {
      std::vector<char> buf(buf_size + 1);
      int r = std::snprintf(&buf[0], buf.size(), m_href_url.c_str(), page_number);
      if (r > 0) {
        std::string s(buf.begin(), buf.end() - 1);
        os << s;
      }
    }
  } else {
    os << m_page_url << '?';
    for (auto const& i : m_query_params) {
      os << i.first << '=' << i.second << '&';
    }
    os << m_page_number_query_param_key << '=' << page_number;
  }
}

std::string Pagination::get_html() const
{
  std::stringstream ss;
  get_html(ss);
  return ss.str();
}

void Pagination::get_html(std::ostream& os) const
{
  if (m_page_count < 2)
    return;
  os
    << "<nav aria-label=\"pagination\">\n"
    << "  <ul class=\"pagination justify-content-center\">\n";
  if (m_show_prev_next) {
    bool active = m_current_page > 1;
    os
      << "    <li";
    if (!active) {
      os << " class=\"page-item disabled\"";
    } else {
      os << " class=\"page-item\"";
    }
    os << ">";
    // std::cout << "Current page number: " << m_current_page << '\n';

    os
      << "<a class=\"page-link\" href=\"";
    // Fix situation where it may be possible to activate the disabled button
    std::uint32_t previous_page = m_current_page > 1 ? m_current_page -1 : m_current_page;
    append_page_url(os, previous_page);
    os
      <<"\">";
    os
      << "<span class=\"visually-hidden\" accesskey=\"p\">previous page</span>"
      << "<span aria-hidden=\"true\">&laquo;</span>";
    os << "</a>";
    os
      << "</li>\n";
  }
  if (show_first_page()) {
    os
      << "    <li class=\"page-item\"><a class=\"page-link\" href=\"";
    append_page_url(os, 1);
    os << "\"><span class=\"visually-hidden\">page </span>1</a></li>\n";
    // separator button
    if (m_begin_range >  2) {
      os
        << "    <li class=\"page-item disabled\"><a class=\"page-link\" href=\"";
      append_page_url(os, 1);
      os
        <<"\">";
      os
        << "<span class=\"visually-hidden\">separator</span>"
        << "<span aria-hidden=\"true\">&hellip;</span>";
      os << "</a>";
      os
        << "</li>\n";
    }
  }
  // std::cout << "Last page: " << m_page_count << '\n';
  for (std::uint32_t i = m_begin_range; i <= m_end_range; i++) {
    if (i == m_current_page) {
      os
        << "    <li class=\"page-item active\"><a class=\"page-link\" href=\"";
      append_page_url(os, i);
      os
        << "\" aria-current=\"page\">"
        "<span class=\"visually-hidden\">page </span>"
        << i
        << "</a></li>\n";
    } else {
      os
        << "    <li class=\"page-item\"><a class=\"page-link\" href=\"";
      append_page_url(os, i);
      os
        << "\">"
        "<span class=\"visually-hidden\">page </span>"
        << i
        << "</a></li>\n";
    }
  }
  if (show_last_page()) {
    if (m_end_range < m_page_count -1) {
      // separator button
      os
        << "    <li class=\"page-item disabled\"><a class=\"page-link\" href=\"";
      append_page_url(os, m_page_count);
      os
        <<"\">";
      os
        << "<span class=\"visually-hidden\">separator</span>"
        << "<span aria-hidden=\"true\">&hellip;</span>";
      os << "</a>";
      os
        << "</li>\n";
    }
    // last page button
    os
      << "    <li class=\"page-item\">";
    os << "<a class=\"page-link\" href=\"";
    append_page_url(os, m_page_count);
    os
      << "\"><span class=\"visually-hidden\">page </span>"
      << m_page_count
      << "</a></li>\n";
  }
  if (m_show_prev_next) {
    bool active = m_current_page < m_page_count;
    os
      << "    <li";
    if (!active) {
      os << " class=\"page-item disabled\"";
    } else {
      os << " class=\"page-item\"";
    }
    os << "><a class=\"page-link\" href=\"";
    // Fix situation where it may be possible to activate the disabled button
    std::uint32_t max_pages = get_page_count();
    std::uint32_t next_page = m_current_page < max_pages ? m_current_page +1 : m_current_page;
    append_page_url(os, next_page);
    os << "\">";
    os
      << "<span class=\"visually-hidden\" accesskey=\"n\">next page</span>"
      << "<span aria-hidden=\"true\">&raquo;</span>";
    os << "</a>";
    os << "</li>\n";
  }
  os
    << "  </ul>\n"
    << "</nav>\n";
}

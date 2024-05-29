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
#include "logger.hpp"

using namespace fdsd::utils;

const std::array<std::string, 8> Logger::levels = {
  "EMERGENCY", "ALERT", "CRITICAL", "ERROR", "WARN", "NOTICE", "INFO", "DEBUG"
};

std::ostringstream Logger::s_syslog;

void Logger::put_now() const {
  std::time_t t =
    std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  tm* nowTM = std::localtime(&t);

  const std::time_put<char>& tp =
    std::use_facet<std::time_put<char>>(std::locale());
  std::string format = "%F %T %z";
  tp.put(os, os, ' ', nowTM,
         format.c_str(), format.c_str() + format.size());
}

void Logger::log(std::string s, log_level log_level) {
  if (log_level <= level) {
    put_now();
    os << ' ' << label << " [" << levels[level] << "] " << s
       << '\n';
    if (log_level > debug)
      syslog(log_level, "[%s] %s", levels[level].c_str(), s.c_str());
  }
}

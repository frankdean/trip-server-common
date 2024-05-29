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
#ifndef FILE_UTILS_HPP
#define FILE_UTILS_HPP

#include "date_utils.hpp"
#include <exception>
#include <string>
#include <vector>

#ifdef HAVE_CXX17
#include <filesystem>
#else
#include <mutex>
#include <sys/stat.h>
#endif

namespace fdsd {
namespace utils {

struct file_details;
struct dir_entry;

class FileUtils {
private:
#ifndef HAVE_CXX17
  static std::mutex mutex;
#endif
public:

  class DirectoryAccessFailedException : public std::exception {
  private:
    std::string message;
  public:
    DirectoryAccessFailedException(std::string path) {
      message = "Error reading: " + path;
    }
    virtual const char* what() const throw() override {
      return message.c_str();
    }
  };

  enum file_type {unknown, socket, symbolic_link, regular_file, block_device, directory, character_device, fifo};
  static const std::string path_separator;
  static void strip_prefix(std::string prefix, std::string& path);
  static void strip_query_params(std::string& path);
  static std::string get_extension(std::string filename);
  static bool is_directory(std::string path);
  static bool is_file(std::string path);
  static file_details get_file_details(std::string path);
  static std::vector<dir_entry> get_directory(std::string path);
#ifdef HAVE_CXX17
  static FileUtils::file_type get_type(const std::filesystem::file_type& type);
#else
  static FileUtils::file_type get_type(const struct stat& s);
#endif
  static std::string get_type(const FileUtils::file_type& type);
};

struct file_details {
  long size = 0;
  DateTime datetime;
  FileUtils::file_type type = FileUtils::unknown;
};

struct dir_entry : file_details {
  dir_entry() {}
  dir_entry(file_details fd) : file_details(fd), name() {}
  std::string name;
  // FileUtils::file_type type = FileUtils::unknown;
};


} // namespace utils
} // namespace fdsd
#endif // FILE_UTILS_HPP

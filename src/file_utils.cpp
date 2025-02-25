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
#if ! BUILD_FOR_IOS
#include "../config.h"
#endif
#include "file_utils.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <syslog.h>

#ifdef HAVE_CXX17
#include <chrono>
#else
#include <dirent.h>
#endif

using namespace fdsd::utils;

const std::string FileUtils::path_separator = "/";

#ifndef HAVE_CXX17
std::mutex FileUtils::mutex;
#endif

/**
 * Strips the characters from the passed path if they match the passed prefix.
 * Also removes any leading path separator.
 */
void FileUtils::strip_prefix(std::string prefix, std::string& path)
{
  // remove leading slash
  // std::cout << "strip_prefix() - "
  //           << "Path before: \""
  //           << path << "\"\n";
  std::string::size_type x = path.find(prefix);

  // Path must start with the prefix
  if (x == std::string::npos || x != 0)
    return;

  // auto n = prefix.length();
  // std::cout << "Erasing first " << n << " characters\n";
  path.erase(0, prefix.length());
  if (!path.empty() && path.substr(0, 1) == path_separator)
    path.erase(0, 1);
  // std::cout << "strip_prefix() - "
  //           << "Path after: \""
  //           << path << "\"\n";
}

void FileUtils::strip_query_params(std::string& path)
{
  std::string::size_type x = path.find('?');
  if (x != std::string::npos)
    path.erase(x, path.length());
}

std::string FileUtils::get_extension(std::string filename)
{
  auto p = filename.find_last_of(".");
  if (p != std::string::npos && p + 1 < filename.length())
    return filename.substr(p + 1);
  return "";
}

bool FileUtils::is_directory(std::string path)
{
#ifdef HAVE_CXX17
  try {
    std::filesystem::path p(path);
    return std::filesystem::is_directory(path);
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << e.what() << '\n';
    syslog(LOG_WARNING, "File system error determining if \"%s\" is a directory: %s",
           path.c_str(),
           e.what());
  }
#else
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    if (s.st_mode & S_IFDIR)
      return true;
  }
#endif
  return false;
}

bool FileUtils::is_file(std::string path)
{
#ifdef HAVE_CXX17
  try {
    std::filesystem::path p(path);
    return std::filesystem::is_regular_file(path) ||
      std::filesystem::is_symlink(path);
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << e.what() << '\n';
    syslog(LOG_WARNING, "File system error determining if \"%s\" is a file: %s",
           path.c_str(),
           e.what());
  }
#else
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    if (S_ISREG(s.st_mode) || S_ISLNK(s.st_mode))
      return true;
  }
#endif
  return false;
}

std::string FileUtils::get_type(const FileUtils::file_type& type)
{
  switch(type) {
    case unknown:
      return "unknown";
    case socket:
      return "socket";
    case symbolic_link:
      return "symbolic link";
    case regular_file:
      return "regular file";
    case block_device:
      return "block device";
    case directory:
      return "directory";
    case character_device:
      return "character device";
    case fifo:
      return "FIFO";
  }
  return "unknown";
}

#ifdef HAVE_CXX17
FileUtils::file_type FileUtils::get_type(const std::filesystem::file_type& type)
{
  namespace fs = std::filesystem;

  switch (type) {
    case fs::file_type::regular:
      return FileUtils::regular_file;
    case fs::file_type::directory:
      return FileUtils::directory;
    case fs::file_type::symlink:
      return FileUtils::symbolic_link;
    case fs::file_type::block:
      return FileUtils::block_device;
    case fs::file_type::character:
      return FileUtils::character_device;
    case fs::file_type::fifo:
      return FileUtils::fifo;
    case fs::file_type::socket:
      return FileUtils::socket;
    default:
      return FileUtils::unknown;
  }
}
#else
FileUtils::file_type FileUtils::get_type(const struct stat& s)
{
  if (S_ISDIR(s.st_mode)) {
    return FileUtils::directory;
  } else if (S_ISREG(s.st_mode)) {
    return FileUtils::regular_file;
  } else if (S_ISLNK(s.st_mode)) {
    return FileUtils::symbolic_link;
  } else if (S_ISSOCK(s.st_mode)) {
    return FileUtils::socket;
  } else if (S_ISBLK(s.st_mode)) {
    return FileUtils::block_device;
  } else if (S_ISCHR(s.st_mode)) {
    return FileUtils::character_device;
  } else if (S_ISFIFO(s.st_mode)) {
    return FileUtils::fifo;
  }
  return FileUtils::unknown;
}
#endif

file_details FileUtils::get_file_details(std::string path)
{
  file_details retval;
#ifdef HAVE_CXX17
  std::filesystem::path p(path);
  try {
    if (!std::filesystem::is_directory(p)) {
      retval.size = std::filesystem::file_size(p);
    }

    // There is not a nice way to convert the filesytem clock to the system
    // clock in C++ 17.  This method subtracts the difference between the two,
    // base on the different represenations of the current time.  It isn't
    // perfectly accurate or even repeatable.  I.e. you might get a slightly
    // different time for the same file for different calls.  However, it
    // should be accurate enough for this usage.

    auto lwt = std::filesystem::last_write_time(p);
    auto ft_now = std::filesystem::file_time_type::clock::now();
    auto sc_now = std::chrono::system_clock::now();
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        lwt - ft_now + sc_now);

    retval.datetime = DateTime(sctp);

    retval.type = FileUtils::get_type(std::filesystem::status(p).type());
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Error getting details for \"" << path << "\": "
              << e.what() << '\n';
    syslog(LOG_WARNING,
           "Error getting details for \"%s\": %s",
           path.c_str(), e.what());
    retval.type = FileUtils::unknown;
  }
#else
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    retval.size = s.st_size;
    DateTime dt;
    // s.st_mtime last modified time as seconds since epoch
    dt.set_time_t(s.st_mtime);
    retval.datetime = dt;
    retval.type = get_type(s);
  } else {
    std::cerr << "Failed to get time of last modification for \""
              << path << "\"\n";
    syslog(LOG_WARNING, "Failed to get time of last modification for \"%s\"",
           path.c_str());
    DateTime file_datetime;
  }
#endif
  return retval;
}

/**
 * Gets a sorted list of file entries for the passed path.
 *
 * \param path the full pathname of the directory.
 *
 * \return a sorted list of file entries.
 */
std::vector<dir_entry> FileUtils::get_directory(std::string path)
{
  std::vector<dir_entry> retval;
#ifdef HAVE_CXX17
  // The dot-dot directory is skipped by the iterator
  file_details fd = FileUtils::get_file_details(path);
  dir_entry dot_dot(fd);
  dot_dot.name = "..";
  retval.push_back(dot_dot);
  try {
    for (auto const& de : std::filesystem::directory_iterator(path)) {
      file_details fd = get_file_details(de.path());
      dir_entry e(fd);
      e.name = de.path().filename();
      retval.push_back(e);
    }
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Exception reading directory: "
              << e.what() << '\n';
    syslog(LOG_WARNING,
           "Exception reading directory: \"%s\": %s", path.c_str(),
           e.what());
    throw DirectoryAccessFailedException(path);
  }
#else
  DIR *dir;
  struct dirent *entry;
  struct stat s_stat;
  std::unique_lock<std::mutex> lock(FileUtils::mutex);
  if ((dir = opendir(path.c_str())) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      dir_entry e;
      e.name = std::string(entry->d_name);

      std::string file_path = path + '/' + e.name;
      if (stat(file_path.c_str(), &s_stat) == 0) {
        DateTime dt;
        dt.set_time_t(s_stat.st_mtime);
        e.datetime = dt;
        e.size = (intmax_t) s_stat.st_size;
        e.type = get_type(s_stat);
      } else {
        std::cerr << "Unable to stat file: \""
                  << file_path
                  << "\"\n";
        syslog(LOG_NOTICE, "Unable to stat file: \"%s\"", file_path.c_str());
      }
      retval.push_back(e);
    }
    closedir(dir);
  } else {
    throw DirectoryAccessFailedException(path);
  }
#endif
  std::sort(retval.begin(),
            retval.end(),
            [] (const dir_entry& lhs,
                const dir_entry& rhs) -> bool {
              return lhs.name < rhs.name;
            });
  return retval;
}

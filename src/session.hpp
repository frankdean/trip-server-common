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
#ifndef SESSION_HPP
#define SESSION_HPP

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace fdsd
{
namespace web
{

class Session {
private:
  std::string user_id;
  std::time_t last_updated;
public:
  Session() {
    last_updated = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
  }
  Session(std::string new_user_id) : Session() {
    user_id = new_user_id;
  }
  std::string get_user_id() const {
    return user_id;
  }
  std::time_t get_last_updated_time_t() const {
    return last_updated;
  }
  void set_last_updated(std::time_t updated);
  void set_date(const std::string str_date);
};

typedef std::map<std::string, Session> session_map;

class SessionManager {
private:
  static const int max_session_minutes;
protected:
  /// Mutex used to lock the sessions map
  std::mutex session_mutex;
  /// Map with key of session_id and value of user_id
  fdsd::web::session_map sessions;
  static fdsd::web::SessionManager* session_manager;
  virtual void persist_invalidated_session(std::string session_id) {}
  virtual void session_updated(std::string session_id, const Session& session) const {}
public:
  SessionManager() : sessions(), session_mutex() {}
  static void set_session_manager(fdsd::web::SessionManager* session_manager);
  static fdsd::web::SessionManager* get_session_manager();
  std::pair<bool, std::string> get_user_id_for_session(const std::string& session_id);
  void save_session(const std::string& session_id, const std::string& user_id);
  void expire_sessions();
  void invalidate_session(const std::string& session_id);
  virtual void persist_sessions() {}
  virtual void load_sessions() {}
  void clear_sessions();
  std::string get_session_user_id(const std::string& session_id);
  int get_max_session_minutes() {
    return max_session_minutes;
  }
};

} // namespace web
} // namespace fdsd

#endif

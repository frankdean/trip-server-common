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
#include "session.hpp"
#include "date_utils.hpp"

using namespace fdsd::web;

fdsd::web::SessionManager* SessionManager::session_manager = nullptr;

const int SessionManager::default_max_session_minutes = 60;

void SessionManager::set_session_manager(
    fdsd::web::SessionManager* session_manager)
{
  SessionManager::session_manager = session_manager;
}

fdsd::web::SessionManager* SessionManager::get_session_manager()
{
  return SessionManager::session_manager;
}

void Session::set_last_updated(std::chrono::system_clock::time_point updated)
{
  last_updated = updated;
}
void Session::set_date(const std::string str_date)
{
  fdsd::utils::DateTime t(str_date);
  last_updated = t.time_tp();
}

/**
 * \return a std::pair the first element indicating whether the session exists
 * and the second element containing the user ID associated with the session.
 */
std::optional<std::string>
    SessionManager::get_user_id_for_session(const std::string& session_id)
{
  expire_sessions();
  std::lock_guard<std::mutex> lock(session_mutex);
  // std::cout << "There are " << sessions.size() << " sessions\n";
  // std::cout << "Searching for session_id: " << session_id << '\n';
  auto f = sessions.find(session_id);
  bool exists = (f != sessions.end());
  std::optional<std::string > user_id;
  if (exists) {
    // std::cout << "Found session for user: " << f->second.get_user_id() << '\n';
    user_id = f->second.get_user_id();
    f->second.set_last_updated(std::chrono::system_clock::now());
  // } else {
  //   std::cout << "Failed to find session_id \"" << session_id << "\"";
  }
  return user_id;
}

void SessionManager::save_session(const std::string& session_id,
                                  const std::string& user_id)
{
  std::lock_guard<std::mutex> lock(session_mutex);
  // std::cout << "Saving session_id \"" << session_id << "\" for " << user_id << '\n';
  Session s(user_id);
  sessions[session_id] = s;
  session_mutex.unlock();
  session_updated(session_id, s);
}

void SessionManager::clear_sessions()
{
  std::lock_guard<std::mutex> lock(session_mutex);
  sessions.clear();
}

void SessionManager::expire_sessions()
{
  std::lock_guard<std::mutex> lock(session_mutex);
  std::chrono::system_clock clock;
  auto now = std::chrono::system_clock::now();
  for (auto session = sessions.begin(); session != sessions.end(); ) {
    auto diff = now - session->second.get_last_updated_time_point();
    // std::cout << session->first << " duration "
    //           << std::chrono::duration_cast<std::chrono::seconds>(diff).count()
    //           << " seconds\n";

    if (diff > std::chrono::minutes(get_max_session_minutes())) {
      session = sessions.erase(session);
    } else {
      ++session;
    }
  }
}

void SessionManager::invalidate_session(const std::string& session_id)
{
  std::lock_guard<std::mutex> lock(session_mutex);
  auto f = sessions.find(session_id);
  if (f != sessions.end()) {
    sessions.erase(f);
    persist_invalidated_session(session_id);
  }
}

std::string SessionManager::get_session_user_id(const std::string& session_id)
{
  std::lock_guard<std::mutex> lock(session_mutex);
  auto f = sessions.find(session_id);
  if (f != sessions.end())
    return f->second.get_user_id();
  return "";
}

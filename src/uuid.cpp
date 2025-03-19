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
#include "../config.h"
#include <iostream>
#ifdef HAVE_OSSP_UUID_H
// Not using the C++ wrapper as it does not have a pkg-config file in the Debian distro
// #include <ossp/uuid++.hh>
#include <ossp/uuid.h>
#else
#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif
#endif

#include <string>
#include "uuid.hpp"

using namespace fdsd::utils;

#ifdef HAVE_OSSP_UUID_H
/// Wrapper for creating an OSSP UUID context so that the context can be
/// destroyed, including when an exception is thrown.
class UuidContext {
private:
  uuid_t* _ctx;
public:
  UuidContext();
  ~UuidContext();
  uuid_t* get_context() {return _ctx;};
};

UuidContext::UuidContext()
{
  uuid_rc_t rc;
  if ((rc = uuid_create(&_ctx)) != UUID_RC_OK) {
    std::string err_str(uuid_error(rc));
    std::string error =  "Error creating uuid object to generate UUID: "
      + err_str;
    throw std::runtime_error(error);
  }
}

UuidContext::~UuidContext()
{
  uuid_rc_t rc;
  if ((rc = uuid_destroy(_ctx)) != UUID_RC_OK) {
    std::string err_str(uuid_error(rc));
    std::string error = "Error destroying uuid object: "
      + err_str;
  }
}
#endif // HAVE_OSSP_UUID_H


std::string UUID::generate_uuid()
{
  // Note comments in PostgreSQL documents stating OSSP is not well maintained.
  // https://www.postgresql.org/docs/current/uuid-ossp.html
#ifdef HAVE_OSSP_UUID_H
  UuidContext uuid_context;
  uuid_rc_t rc;
  if ((rc = uuid_make(uuid_context.get_context(),
                      UUID_MAKE_V1)) != UUID_RC_OK) {
    std::string err_str(uuid_error(rc));
    std::string error =  "Error generating UUID: "
      + err_str;
    throw std::runtime_error(error);
  }
  char *str = NULL;
  if ((rc = uuid_export(uuid_context.get_context(),
                        UUID_FMT_STR, &str, NULL)) != UUID_RC_OK) {
    std::string err_str(uuid_error(rc));
    std::string error =  "Error exporting UUID: "
      + err_str;
    throw std::runtime_error(error);
  }
  std::string retval(str);
  free(str);
  return retval;
#else
#ifdef HAVE_UUID_UUID_H
  uuid_t uuid;
#ifdef HAVE_SAFE_UUID
  int safe = uuid_generate_time_safe(uuid);
  if (safe == -1) {
    std::cerr << "WARNING: UUID was NOT generated in a safe manner\n"
      "This could allow duplicate UUIDs to be created when\n"
      "multiple processes generate UUIDs.\n\n"
      "Run the UUID daemon (from the uuid-runtime package on Debian)\n"
      " to support safe generation.\n";
  } else {
    std::cerr << "INFO: UUID was generated in a safe manner\n";
  }
#else
  // This should use /dev/random, if available, otherwise time and mac based
  uuid_generate(uuid);
#endif
  char uuid_str[37];
  uuid_unparse_lower(uuid, uuid_str); // 1b4e28ba-2fa1-11d2-883f-0016d3cca427
  std::string retval(uuid_str);
  // std::cerr << "Created UUID\n";
  return retval;
#else
  throw std::runtime_error("UUID library not available");
#endif
#endif
}

bool UUID::is_valid(const std::string s)
{
#ifdef HAVE_OSSP_UUID_H
  if (s.length() != UUID_LEN_STR)
    return false;
  UuidContext uuid_context;
  uuid_rc_t rc;
  rc = uuid_import(uuid_context.get_context(),
                   UUID_FMT_STR,
                   s.c_str(),
                   s.length());
  // if (rc != UUID_RC_OK) {
  //   std::cerr << "Error converting \"" << s << "\": "
  //             << uuid_error(rc) << '\n';
  // }
  return rc == UUID_RC_OK;
#else
#ifdef HAVE_UUID_UUID_H
  uuid_t ctx;
  int r = uuid_parse(s.c_str(), ctx);
  // char uuid_str[37];
  // uuid_unparse_lower(uu, uuid_str);
  // std::string s(uuid_str);
  // std::cout << "UUID: \"" << uuid << "\" is " << (r == 0 ? "valid" : "invalid") << '\n';
  // if (r == 0)
  //   std::cout << "Parsed as " << s << '\n';
  return r == 0;
#else
  throw std::runtime_error("UUID library not available");
#endif
#endif
}

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
#include "http_request.cpp"
#include <iostream>
#include <regex>

const std::string test_post_header =
  "POST /trip2/app/sharing HTTP/1.1\r\n"
  "Host: localhost:8080\r\n"
  "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
  "Accept-Encoding: gzip, deflate\r\n"
  "Accept-Language: en-GB,en;q=0.9\r\n"
  "Content-Type: application/x-www-form-urlencoded\r\n"
  "Origin: http://localhost:8088\r\n"
  "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.1 Safari/605.1.15\r\n"
  "Connection: keep-alive\r\n"
  "Upgrade-Insecure-Requests: 1\r\n"
  "Referer: http://localhost:8088/trip2/app/sharing\r\n"
  "Content-Length: 59\r\n"
  "Cookie: TRIP_SESSION_ID=e61198f8-b7c5-42df-a34f-5dd3affe79aa\r\n"
  "\r\n"
;

const std::string test_post_body =
  "nickname%5B0%5D=Fred&nickname%5B24%5D=admin&action=activate";

const std::string test_bad_post_header =
  "POST /trip2/app/sharing HTTP/1.1\r\n"
  "Host: localhost:8080\r\n"
  "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
  "Accept-Encoding: gzip, deflate\r\n"
  "Accept-Language: en-GB,en;q=0.9\r\n"
  "Content-Type: application/x-www-form-urlencoded\r\n"
  "Origin: http://localhost:8088\r\n"
  "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.1 Safari/605.1.15\r\n"
  "Connection: keep-alive\r\n"
  "Upgrade-Insecure-Requests: 1\r\n"
  "Referer: http://localhost:8088/trip2/app/sharing\r\n"
  "Content-Length: 99\r\n"
  "Cookie: TRIP_SESSION_ID=e61198f8-b7c5-42df-a34f-5dd3affe79aa\r\n"
  "\r\n"
;

const std::string test_bad_post_body =
  "nickname0%5D=Fred&nickname%5B24=admin&foo=&nickname%5B%5D=bar"
  "&action=activate&nickname%5B99%5D=test";

const std::string test_post_file =
  "POST /trip2/app/itinerary/upload HTTP/1.1\r\n"
  "Host: localhost:8080\r\n"
  "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
  "Accept-Encoding: gzip, deflate\r\n"
  "Accept-Language: en-GB,en;q=0.9\r\n"
  "Content-Type: multipart/form-data; boundary=----TestBoundaryString\r\n"
  "Origin: http://localhost:8080\r\n"
  "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.1 Safari/605.1.15\r\n"
  "Connection: keep-alive\r\n"
  "Upgrade-Insecure-Requests: 1\r\n"
  "Referer: http://localhost:8080/trip2/app/itinerary/upload\r\n"
  "Content-Length: 405\r\n"
  "Cookie: TRIP_SESSION_ID=e61198f8-b7c5-42df-a34f-5dd3affe79aa\r\n"
  "\r\n"
  "------TestBoundaryString\r\n"
  "Content-Disposition: form-data; name=\"id\"\r\n"
  "\r\n"
  "7\r\n"
  "------TestBoundaryString\r\n"
  "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
  "Content-Type: text/plain\r\n"
  "\r\n"
  "first line\r\n"
  "second line\r\n"
  "third line\r\n"
  "\r\n"
  "------TestBoundaryString\r\n"
  "Content-Disposition: form-data; name=\"action\"\r\n"
  "\r\n"
  "upload\r\n"
  "------TestBoundaryString--\r\n";

bool extract_array_from_post_params_test1()
{
  bool retval = true;
  const std::string test_request = test_post_header + test_post_body;
  try {
    HTTPServerRequest request(test_request);
    const std::map<long, std::string> nickname_map =
      request.extract_array_param_map("nickname");
    const int size = nickname_map.size();
    if (size != 2) {
      retval = false;
      std::cerr << "extract_array_from_post_params_test1() failed\n"
                << "expected 2 result, but got " << size << '\n';
      for (const auto &m : nickname_map) {
        std::cerr << "Entry: " << m.first << " -> \"" << m.second << "\"\n";
      }
    }
    const std::string first = nickname_map.at(0);
    if (first != "Fred") {
      retval = false;
      std::cerr << "Expected \"Fred\" but got \"" << first << "\"\n";
    }
    const std::string second = nickname_map.at(24);
    if (second != "admin") {
      retval = false;
      std::cerr << "Expected \"admin\" but got \"" << first << "\"\n";
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    retval = false;
  }
  return retval;
}

bool extract_array_from_post_params_test2()
{
  bool retval = true;
  int invalid_value = 0;
  const std::string test_request = test_bad_post_header + test_bad_post_body;
  try {
    HTTPServerRequest request(test_request);
    const std::map<long, std::string> nickname_map =
      request.extract_array_param_map("nickname");
    const int size = nickname_map.size();
    if (size != 1) {
      retval = false;
      std::cerr << "extract_array_from_post_params_test2() failed\n"
                << "expected one result, but got " << size << '\n';
      for (const auto &m : nickname_map) {
        std::cerr << "Entry: " << m.first << " -> \"" << m.second << "\"\n";
      }
    }
    const std::string first = nickname_map.at(99);
    if (first != "test") {
      retval = false;
      std::cerr << "Expected \"test\" but got \"" << first << "\"\n";
    }
  } catch (const std::invalid_argument &e) {
    invalid_value++;
  } catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << '\n';
    retval = false;
  }
  if (invalid_value > 1) {
    retval = false;
    std::cerr << "Exepected only 1 invalid numeric value, but got "
              << invalid_value << '\n';
  }
  if (!retval)
    std::cerr << "extract_array_from_post_params_test4() failed\n";
  return retval;
}

bool test_get_header()
{
  bool retval = true;
  const std::string test_request = test_post_header + test_post_body;
  try {
    HTTPServerRequest request(test_request);
    const auto result = request.get_header("Connection");
    retval = (result == "keep-alive");
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    retval = false;
  }
  return retval;
}

bool test_get_header_case_insignificant()
{
  bool retval = true;
  const std::string test_request = test_post_header + test_post_body;
  try {
    HTTPServerRequest request(test_request);
    const auto result = request.get_header("connection");
    retval = (result == "keep-alive");
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    retval = false;
  }
  return retval;
}

bool test_form_upload()
{
  bool retval = true;

  try {
    HTTPServerRequest request(test_post_file);
    retval = request.headers.size() == 12;
    if (retval)
      retval = request.get_post_params().size() == 2;
    const auto mps = request.multiparts;
    if (retval)
      retval = mps.size() == 1;
    try {
      const auto mp = mps.at("file");
      if (retval)
        retval = mp.headers.size() == 2;
      if (retval)
        retval = !mp.body.empty();
    } catch (const std::out_of_range &e) {
      std::cerr << "Exception in test_form_upload: " << e.what() << '\n';
      retval = false;
    }
    if (retval)
      retval = request.get_post_params()["action"] == "upload";
    if (retval)
      retval = request.get_post_params()["id"] == "7";
    if (!retval) {
      std::cerr << "Failed test in test_form_upload()\n";
      std::cerr << request.headers.size() << " headers:\n";
      for (const auto &h : request.headers) {
        std::cerr << "header: \"" << h.first << "\" -> \"" << h.second << "\"\n";
      }
      for (const auto &h : request.get_post_params()) {
        std::cerr << "param: \"" << h.first << "\" -> \"" << h.second << "\"\n";
      }
      for (const auto &m : request.multiparts) {
        std::cerr << "Multipart: \"" << m.first << "\"\n";
        for (const auto &h : m.second.headers) {
          std::cerr << "local header: \"" << h.first << "\" -> \"" << h.second << "\"\n";
        }
        std::cerr << "Multipart body: \"" << m.second.body << "\"\n";
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Exception during test_form_upload(): "
              << e.what() << '\n';
  }
  return retval;
}

int main(void)
{
  try {
    return !(
        extract_array_from_post_params_test1() &&
        extract_array_from_post_params_test2() &&
        test_get_header() &&
        test_get_header_case_insignificant() &&
        test_form_upload()
      );
  } catch (const std::exception &e) {
    std::cerr << "Tests failed with: " << e.what() << '\n';
    return 1;
  }
}

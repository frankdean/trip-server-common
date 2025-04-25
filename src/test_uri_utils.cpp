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
#include "uri_utils.cpp"
#include <iomanip>
#include <iostream>
#include <string>

bool test_simple_uri()
{
  const UriUtils u;
  return "This is a/test" ==
    u.uri_decode("This+is%20a/test") &&
    "%" == u.uri_decode("%25") &&
    "/first" == u.uri_decode("%2Ffirst") &&
    "last/" == u.uri_decode("last%2F") &&
    "A" == u.uri_decode("A") &&
    "" == u.uri_decode("");
}

bool test_not_url_form_encoded()
{
  const UriUtils u;
  return "This+is a/test" ==
    u.uri_decode("This+is%20a/test", false);
}

bool test_reserved_characters()
{
  const UriUtils u;
  std::string expected = " !#$%&'()*+,/:;=?@[]";
  return expected ==
    //            SP !  #  $  %  &  '  (  )  *  +  ,  /  :  ;  =  ?  @  [  ]
    u.uri_decode("%20%21%23%24%25%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D") &&
    expected ==
    u.uri_decode("%20%21%23%24%25%26%27%28%29%2a%2b%2c%2f%3a%3b%3d%3f%40%5b%5d");
}

bool test_utf8_characters()
{
  const UriUtils u;
  std::string expected = "\U0001F62Ctest\U0001F600";
  // std::cout << "Expected value: " << expected << std::endl;
  return expected ==
    u.uri_decode("%F0%9F%98%ACtest%F0%9F%98%80");
}

bool test_invalid_uri()
{
  const UriUtils u;
  return
    "I hope we don't access characters beyond the end of the string.%" ==
    u.uri_decode("I hope we don't access characters beyond the end of the string.%")
    && "%" == u.uri_decode("%")
    && "%1" == u.uri_decode("%1")
    && "%1Gabc%1g:" == u.uri_decode("%1Gabc%1g%3a")
    && "%1Gabc%1:" == u.uri_decode("%1Gabc%1%3a")
    && "%QV" == u.uri_decode("%QV")
    && "%GG%% %%% A %B %%C" == u.uri_decode("%GG%% %%% %41 %%42 %%%43")
    ;
}

bool test_form_decoded_plus_sign_01()
{
  const std::string result = UriUtils::uri_decode("+");
  // std::cout << "Result 01: \"" << result << "\"\n";
  return result == " ";
}

bool test_form_decoded_plus_sign_02()
{
  const std::string result = UriUtils::uri_decode("%2B");
  // std::cout << "Result 02: \"" << result << "\"\n";
  return result == "+";
}

bool test_get_account_query_params()
{
  const std::string account_key = "account_id";
  const std::string account_id = "54c7a56d-1029-42ae-87da-2384362c42d4";
  const std::string test_uri = "http://localhost:8080/fdsd/test?" +
    account_key + '=' + account_id;
  auto params = UriUtils::get_query_params(test_uri);
  // if (params.size() != 1) {
  //   std::cout << "Got " << params.size() << " parameters\n";
  // }
  return params[account_key] == account_id;
}

bool test_get_two_query_params()
{
  const std::string account_key = "account_id";
  const std::string account_id = "54c7a56d-1029-42ae-87da-2384362c42d4";
  const std::string test_key = "test";
  const std::string test_value = "my_test";
  const std::string test_uri = "http://localhost:8080/fdsd/test?" +
    account_key + '=' + account_id + '&' +
    test_key + '=' + test_value;
  auto params = UriUtils::get_query_params(test_uri);
  // if (params.size() != 1) {
  //   std::cout << "Got " << params.size() << " parameters";
  // }
  return params[account_key] == account_id && params[test_key] == test_value;
}

bool test_get_three_query_params()
{
  const std::string account_key = "account_id";
  const std::string account_id = "54c7a56d-1029-42ae-87da-2384362c42d4";
  const std::string test_key = "test";
  const std::string test_value = "my_test";
  const std::string test_key2 = "test";
  const std::string test_value2 = "my_test";
  const std::string test_uri = "http://localhost:8080/fdsd/test?" +
    account_key + '=' + account_id + '&' +
    test_key + '=' + test_value + '&' +
    test_key2 + '=' + test_value2;
  auto params = UriUtils::get_query_params(test_uri);
  // if (params.size() != 1) {
  //   std::cout << "Got " << params.size() << " parameters";
  // }
  return params[account_key] == account_id &&
    params[test_key] == test_value &&
    params[test_key2] == test_value2;
}

bool test_get_empty_query_params()
{
  const std::string test_uri = "http://localhost:8080/fdsd/test?";
  auto params = UriUtils::get_query_params(test_uri);
  // std::cout << "Got " << params.size() << " parameters\n";
  return params.size() == 0;
}

bool test_get_null_query_params()
{
  const std::string test_uri = "http://localhost:8080/fdsd/test";
  auto params = UriUtils::get_query_params(test_uri);
  // std::cout << "Got " << params.size() << " parameters\n";
  return params.size() == 0;
}

bool test_get_invalid_query_params_01()
{
  auto params = UriUtils::get_query_params("blah?rubbish");
  return params.size() == 1 && params["rubbish"] == "";
}

bool test_get_invalid_query_params_02()
{
  auto params = UriUtils::get_query_params("blah?rubbish&more");
  return params.size() == 2 && params["rubbish"] == "" && params["more"] == "";
}

bool test_get_invalid_query_params_03()
{
  auto params = UriUtils::get_query_params("blah?rubbish=&more");
  return params.size() == 2 && params["rubbish"] == "" && params["more"] == "";
}

bool test_get_invalid_query_params_04() {
  auto params = UriUtils::get_query_params("rubbish=&more");
  return params.size() == 0;
}

bool test_get_query_params_multiple_query_separators()
{
  auto params = UriUtils::get_query_params("?key1=value1&key2=value2?key3=value3&key4=value4");
  // for (auto p = params.begin(); p != params.end(); ++p) {
  //   std::cout << p->first << '=' << p->second << '\n';
  // }
  return params.size() == 2 && params["key3"] == "value3" && params["key4"] == "value4";
}

bool test_encode_single_reserved_character()
{
  const std::string result = UriUtils::uri_encode("#");
  return result == "%23";
}

bool test_encode_unreserved_character()
{
  const std::string result = UriUtils::uri_encode("f");
  return result == "f";
}

bool test_encode_all_unreserved_and_reserved_characters()
{
  const std::string test_string = UriUtils::unreserved_characters +
    UriUtils::reserved_characters + ' ';
  const std::string encoded = UriUtils::uri_encode(test_string, false);
  // std::cout << "Encoded: \"" << encoded << "\"\n";
  const std::string decoded = UriUtils::uri_decode(test_string, false);
  // std::cout << "Decoded: \"" << decoded << "\"\n";
  return decoded == test_string;
}

bool test_form_encoded_space()
{
  const std::string result = UriUtils::uri_encode(" ");
  // std::cout << "Result: \"" << result << "\"\n";
  return result == "+";
}

bool test_encode_all_unreserved_and_reserved_characters_form_encoded()
{
  const std::string test_string = UriUtils::unreserved_characters +
    UriUtils::reserved_characters + ' ';
  const std::string encoded = UriUtils::uri_encode(test_string, true);
  // std::cout << "Encoded: \"" << encoded << "\"\n";
  const std::string decoded = UriUtils::uri_decode(encoded, true);
  // std::cout << "Test value: \"" << test_string << "\"\n";
  // std::cout << "Decoded   : \"" << decoded << "\"\n";
  return decoded == test_string;
}

bool test_encode_rfc_1738()
{
  const std::string test_string = "http://some.foo.bar/place?entry=first line(%1)\nsecond line (#2)\x7F";
  const std::string encoded = UriUtils::uri_encode_rfc_1738(test_string);
  const std::string expected =
    "http://some.foo.bar/place?entry=first%20line(%251)%0Asecond%20line%20(%232)%7F";
  const bool retval = encoded == expected;
  if (!retval)
    std::cout << "test_encode_rfc_1738 failed\n"
              << "expected: \"" << expected
              << "\"\nbut was   \""
              << encoded << "\"\n";
  return retval;
}

int main(void)
{
  try {
    const int retval =
      !(
          test_encode_rfc_1738()
          && test_invalid_uri()
          && test_not_url_form_encoded()
          && test_simple_uri()
          && test_reserved_characters()
          && test_utf8_characters()
          && test_form_decoded_plus_sign_01()
          && test_form_decoded_plus_sign_02()
          && test_get_account_query_params()
          && test_get_empty_query_params()
          && test_get_null_query_params()
          && test_get_two_query_params()
          && test_get_three_query_params()
          && test_get_invalid_query_params_01()
          && test_get_invalid_query_params_02()
          && test_get_invalid_query_params_03()
          && test_get_invalid_query_params_04()
          && test_get_query_params_multiple_query_separators()
          && test_encode_single_reserved_character()
          && test_encode_unreserved_character()
          && test_encode_all_unreserved_and_reserved_characters()
          && test_form_encoded_space()
          && test_encode_all_unreserved_and_reserved_characters_form_encoded()
        );
    return retval;
  } catch (const std::exception &e) {
    std::cerr << "Tests failed with: " << e.what() << '\n';
    return 1;
  }
}

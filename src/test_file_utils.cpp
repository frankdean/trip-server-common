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
#include "file_utils.cpp"
#include <iostream>
#include <string>

using namespace fdsd::utils;

const std::string simple_prefix = FileUtils::path_separator + "foo";

const std::string test_prefix_tolerable = simple_prefix +
  FileUtils::path_separator + "bar";

const std::string test_prefix_good = test_prefix_tolerable +
  FileUtils::path_separator;

const std::string test_prefix_bad = "foo-bar";

// Should successfully strip the prefix from the passed path.

bool test_strip_prefix_01()
{
  std::cout << "test_strip_prefix_01() simple prefix: \""
            << simple_prefix << "\"\n";
  const std::string expected("test" + FileUtils::path_separator);
  std::string test_path = simple_prefix + FileUtils::path_separator + expected;
  std::cout << "test_strip_prefix_01 test path before: \""
            << test_path << "\"\n";
  FileUtils::strip_prefix(simple_prefix, test_path);
  std::cout << "test_strip_prefix_01 result: \""
            << test_path << "\"\n";
  return test_path == expected;
}

// Should successfully strip a prefix containing more than one path separator
// from the passed path.
bool test_strip_prefix_02()
{
  std::cout << "test_strip_prefix_02() prefix: \""
            << test_prefix_good << "\"\n";
  const std::string expected("test" + FileUtils::path_separator);
  std::string test_path = test_prefix_good + FileUtils::path_separator +
    expected;
  std::cout << "test_strip_prefix_02 test path before: \""
            << test_path << "\"\n";
  FileUtils::strip_prefix(test_prefix_good, test_path);
  std::cout << "test_strip_prefix_02 result: \""
            << test_path << "\"\n";
  return test_path == expected;
}

// Should successfully strip a prefix containing a leading dot(period) ".".
bool test_strip_prefix_03()
{
  const std::string dot_prefix = "." + FileUtils::path_separator;
  std::cout << "test_strip_prefix_03() prefix: \""
            << dot_prefix << "\"\n";
  const std::string expected("test" + FileUtils::path_separator);
  std::string test_path = dot_prefix + expected;
  std::cout << "test_strip_prefix_03 test path before: \""
            << test_path << "\"\n";
  FileUtils::strip_prefix(dot_prefix, test_path);
  std::cout << "test_strip_prefix_03 result: \""
            << test_path << "\"\n";
  return test_path == expected;
}

// Should successfully strip a prefix containing only a leading path separator.
bool test_strip_prefix_04()
{
  const std::string test_prefix = FileUtils::path_separator;
  std::cout << "test_strip_prefix_04() prefix: \""
            << test_prefix << "\"\n";
  const std::string expected("test" + FileUtils::path_separator);
  std::string test_path = test_prefix + expected;
  std::cout << "test_strip_prefix_04 test path before: \""
            << test_path << "\"\n";
  FileUtils::strip_prefix(test_prefix, test_path);
  std::cout << "test_strip_prefix_04 result: \""
            << test_path << "\"\n";
  return test_path == expected;
}

// Should cope with an empty path.
bool test_strip_prefix_05()
{
  std::cout << "test_strip_prefix_05() prefix: \""
            << test_prefix_good << "\"\n";
  const std::string expected;
  std::string test_path = test_prefix_good + FileUtils::path_separator +
    expected;
  std::cout << "test_strip_prefix_05 test path before: \""
            << test_path << "\"\n";
  FileUtils::strip_prefix(test_prefix_good, test_path);
  std::cout << "test_strip_prefix_05 result: \""
            << test_path << "\"\n";
  return test_path == expected;
}

// Should cope with a path containing just a path separator.
bool test_strip_prefix_06()
{
  std::cout << "test_strip_prefix_06() prefix: \""
            << test_prefix_good << "\"\n";
  const std::string expected = FileUtils::path_separator;
  std::string test_path = test_prefix_good + FileUtils::path_separator +
    expected;
  std::cout << "test_strip_prefix_06 test path before: \""
            << test_path << "\"\n";
  FileUtils::strip_prefix(test_prefix_good, test_path);
  std::cout << "test_strip_prefix_06 result: \""
            << test_path << "\"\n";
  return test_path == expected;
}

// Should fail to strip a prefix which does not contain the path.
bool test_strip_prefix_07()
{
  std::cout << "test_strip_prefix_07() prefix: \""
            << test_prefix_good << "\"\n";
  const std::string expected{test_prefix_bad + FileUtils::path_separator +
    "test" + FileUtils::path_separator};
  std::string test_path = expected;
  std::cout << "test_strip_prefix_07 test path before: \""
            << test_path << "\"\n";
  FileUtils::strip_prefix(test_prefix_good, test_path);
  std::cout << "test_strip_prefix_07 result: \""
            << test_path << "\"\n";
  return test_path == expected;
}

// Should fail to strip a prefix which does not contain the path in the first
// position.
bool test_strip_prefix_08()
{
  std::cout << "test_strip_prefix_08() prefix: \""
            << test_prefix_good << "\"\n";
  const std::string expected{test_prefix_bad + FileUtils::path_separator +
    test_prefix_good + FileUtils::path_separator +
    "test" + FileUtils::path_separator};
  std::string test_path = expected;
  std::cout << "test_strip_prefix_08 test path before: \""
            << test_path << "\"\n";
  FileUtils::strip_prefix(test_prefix_good, test_path);
  std::cout << "test_strip_prefix_08 result: \""
            << test_path << "\"\n";
  return test_path == expected;
}

// Should cope with an empty prefix.
bool test_strip_prefix_09()
{
  const std::string expected = "my/test/path";
  std::string test_path = expected;
  FileUtils::strip_prefix("", test_path);
  return test_path == expected;
}

// Should cope with an empty prefix and an empty path.
bool test_strip_prefix_10()
{
  const std::string expected;
  std::string test_path = expected;
  FileUtils::strip_prefix("", test_path);
  return test_path == expected;
}

bool test_strip_query_params_none()
{
  const std::string expected = "/foo/bar";
  std::string path(expected);
  FileUtils::strip_query_params(path);
  return path == expected;
}

bool test_strip_query_params_only_query_param()
{
  const std::string expected = "";
  std::string path{'?'};
  FileUtils::strip_query_params(path);
  return path == expected;
}

bool test_strip_query_params_empty_query_params()
{
  const std::string expected = "/foo/bar";
  std::string path(expected + '?');
  FileUtils::strip_query_params(path);
  return path == expected;
}

bool test_strip_query_params_with_query_params()
{
  const std::string expected = "/foo/bar";
  std::string path(expected + "?foo=bar&so-on?invalid=bad");
  FileUtils::strip_query_params(path);
  return path == expected;
}

bool test_get_extension_none()
{
  return FileUtils::get_extension("foo-bar").empty();
}

bool test_get_extension_null()
{
  return FileUtils::get_extension("foo-bar.").empty();
}

bool test_get_extension_null_string()
{
  return FileUtils::get_extension("").empty();
}

bool test_get_extension()
{
  return "xyz" == FileUtils::get_extension("foo-bar.xyz");
}

bool test_get_extension_html()
{
  return "html" == FileUtils::get_extension("foo-bar.html");
}

int main(void)
{
  try {
    return !(
        test_strip_prefix_01()
        && test_strip_prefix_02()
        && test_strip_prefix_03()
        && test_strip_prefix_04()
        && test_strip_prefix_05()
        && test_strip_prefix_06()
        && test_strip_prefix_07()
        && test_strip_prefix_08()
        && test_strip_prefix_09()
        && test_strip_prefix_10()
        && test_strip_query_params_none()
        && test_strip_query_params_only_query_param()
        && test_strip_query_params_empty_query_params()
        && test_strip_query_params_with_query_params()
        && test_get_extension_none()
        && test_get_extension_null()
        && test_get_extension_null_string()
        && test_get_extension()
        && test_get_extension_html()
      );
  } catch (const std::exception &e) {
    std::cerr << "Tests failed with: " << e.what() << '\n';
    return 1;
  }
}

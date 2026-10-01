/*
    InputLeap -- mouse and keyboard sharing utility
    Copyright (C) InputLeap contributors

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "winapi.h"
#include <string>
#include <vector>

std::string win_wchar_to_utf8(const WCHAR* utfStr);
std::vector<WCHAR> utf8_to_win_char(const std::string& str);

// Windows' narrow argv uses the system code page. Parse the original UTF-16
// command line instead, preserving UTF-8 throughout the application.
std::vector<std::string> win_command_line_to_utf8(const WCHAR* command_line);
int win_utf8_main(int (*main_function)(int, char**));

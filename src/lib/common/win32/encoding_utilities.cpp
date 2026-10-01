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

#include "encoding_utilities.h"
#include <stringapiset.h>
#include <limits>
#include <shellapi.h>
#include <memory>
#include <system_error>

std::string win_wchar_to_utf8(const WCHAR* utfStr)
{
    int utfLength = lstrlenW(utfStr);
    int mbLength = WideCharToMultiByte(CP_UTF8, 0, utfStr, utfLength, nullptr, 0, nullptr, nullptr);
    std::string mbStr(mbLength, 0);
    WideCharToMultiByte(CP_UTF8, 0, utfStr, utfLength, &mbStr[0], mbLength, nullptr, nullptr);
    return mbStr;
}

std::vector<WCHAR> utf8_to_win_char(const std::string& str)
{
    if (str.size() > std::numeric_limits<int>::max())
        return {};
    int input_len = static_cast<int>(str.size());
    int result_len = MultiByteToWideChar(CP_UTF8, 0, str.data(), input_len, nullptr, 0);
    std::vector<WCHAR> result;
    result.resize(result_len + 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), input_len, result.data(), result_len);
    return result;
}

std::vector<std::string> win_command_line_to_utf8(const WCHAR* command_line)
{
    int argc = 0;
    auto argv = CommandLineToArgvW(command_line, &argc);
    if (!argv) {
        throw std::system_error(GetLastError(), std::system_category(),
                                "Could not read Windows command line");
    }
    auto release = [](WCHAR** value) { LocalFree(value); };
    std::unique_ptr<WCHAR*, decltype(release)> owner(argv, release);
    std::vector<std::string> arguments;
    arguments.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        arguments.push_back(win_wchar_to_utf8(argv[i]));
    }
    return arguments;
}

int win_utf8_main(int (*main_function)(int, char**))
{
    auto arguments = win_command_line_to_utf8(GetCommandLineW());
    std::vector<char*> argv;
    argv.reserve(arguments.size() + 1);
    for (auto& argument : arguments) {
        argv.push_back(argument.data());
    }
    argv.push_back(nullptr);
    return main_function(static_cast<int>(arguments.size()), argv.data());
}

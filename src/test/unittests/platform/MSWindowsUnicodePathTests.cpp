/* InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) InputLeap contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "base/EventQueue.h"
#include "arch/win32/ArchMiscWindows.h"
#include "base/finally.h"
#include "base/log_outputters.h"
#include "common/win32/encoding_utilities.h"
#include "io/filesystem.h"
#include "net/SecureSocket.h"
#include "net/SecureUtils.h"
#include "net/SocketMultiplexer.h"
#include <gtest/gtest.h>
#include <fstream>

namespace inputleap {

TEST(MSWindowsCommandLineTests, PreservesUnicodeQuotesEmptyArgumentsAndTrailingSlash)
{
    const auto arguments = win_command_line_to_utf8(
        L"\"C:\\Program Files\\\uc5b4\ub2c8\uc2a4\ud2b8\ube44\uc804\\input-leapc.exe\" "
        L"--profile-dir \"C:\\Users\\\uc5b4\ub2c8\uc2a4\ud2b8\ube44\uc804 test \U0001f431\\\\\" "
        L"\"\" \"quoted\\\"argument\"");
    const std::vector<std::string> expected = {
        u8"C:\\Program Files\\\uc5b4\ub2c8\uc2a4\ud2b8\ube44\uc804\\input-leapc.exe",
        "--profile-dir",
        u8"C:\\Users\\\uc5b4\ub2c8\uc2a4\ud2b8\ube44\uc804 test \U0001f431\\",
        "", "quoted\"argument"
    };
    EXPECT_EQ(arguments, expected);
}

TEST(MSWindowsCommandLineTests, Utf8ServiceCommandRoundTrip)
{
    const std::string command = u8"input-leapc.exe --profile-dir \"C:\\Users\\"
        u8"\uc5b4\ub2c8\uc2a4\ud2b8\ube44\uc804 test \U0001f431\" --name laptop";
    auto wide = utf8_to_win_char(command);
    EXPECT_EQ(win_wchar_to_utf8(wide.data()), command);
    auto arguments = win_command_line_to_utf8(wide.data());
    ASSERT_EQ(arguments.size(), 5u);
    EXPECT_EQ(arguments[2], u8"C:\\Users\\\uc5b4\ub2c8\uc2a4\ud2b8\ube44\uc804 test \U0001f431");
}

TEST(MSWindowsCommandLineTests, PersistedServiceCommandRoundTrip)
{
    const auto key_name = L"Software\\InputLeapUnicodeTest-" +
        std::to_wstring(GetCurrentProcessId());
    HKEY key = nullptr;
    ASSERT_EQ(RegCreateKeyExW(HKEY_CURRENT_USER, key_name.c_str(), 0, nullptr,
                             REG_OPTION_VOLATILE, KEY_ALL_ACCESS, nullptr, &key, nullptr),
              ERROR_SUCCESS);
    auto cleanup = finally([&]() {
        RegCloseKey(key);
        RegDeleteKeyW(HKEY_CURRENT_USER, key_name.c_str());
    });
    const std::string command = u8"input-leapc.exe --profile-dir \"C:\\Users\\"
        u8"\uc5b4\ub2c8\uc2a4\ud2b8\ube44\uc804 test \U0001f431\"";
    ArchMiscWindows::setValue(key, "Command", command);
    EXPECT_EQ(ArchMiscWindows::readValueString(key, "Command"), command);
    EXPECT_TRUE(ArchMiscWindows::readValueString(key, "Missing").empty());
    const char legacy[] = "input-leapc.exe --name laptop";
    ASSERT_EQ(RegSetValueExA(key, "Legacy", 0, REG_SZ,
                            reinterpret_cast<const BYTE*>(legacy), sizeof(legacy)), ERROR_SUCCESS);
    EXPECT_EQ(ArchMiscWindows::readValueString(key, "Legacy"), legacy);
    ArchMiscWindows::setValue(key, "Empty", std::string());
    EXPECT_TRUE(ArchMiscWindows::readValueString(key, "Empty").empty());
}

class MSWindowsUnicodePathTests : public testing::Test {
protected:
    void SetUp() override
    {
        root_ = fs::temp_directory_path() /
            ("input-leap-unicode-" + std::to_string(GetCurrentProcessId()) + "-" +
             std::to_string(GetTickCount64()));
        ASSERT_TRUE(fs::create_directory(root_));
        directory_ = root_ / fs::u8path(u8"\uc5b4\ub2c8\uc2a4\ud2b8\ube44\uc804 test \U0001f431");
        fs::create_directory(directory_);
    }

    void TearDown() override
    {
        std::error_code error;
        fs::remove_all(root_, error);
    }

    fs::path root_;
    fs::path directory_;
};

TEST_F(MSWindowsUnicodePathTests, GenerateAndReadCertificate)
{
    auto path = directory_ / "InputLeap.pem";
    generate_pem_self_signed_cert(path.u8string());
    ASSERT_TRUE(fs::is_regular_file(path));
    auto fingerprint = get_pem_file_cert_fingerprint(path.u8string(), FingerprintType::SHA256);
    EXPECT_EQ(fingerprint.data.size(), 32u);
}

TEST_F(MSWindowsUnicodePathTests, LoadCertificateAndPrivateKey)
{
    // Generate at an ASCII filename first to isolate certificate loading from generation.
    auto original = root_ / "InputLeap.pem";
    generate_pem_self_signed_cert(original.u8string());
    auto path = directory_ / "InputLeap.pem";
    fs::rename(original, path);
    EventQueue events;
    SocketMultiplexer multiplexer;
    SecureSocket socket(&events, &multiplexer, IArchNetwork::kINET,
                        ConnectionSecurityLevel::ENCRYPTED);
    socket.initSsl(true);
    EXPECT_TRUE(socket.load_certificates(path));
    EXPECT_FALSE(socket.load_certificates(directory_ / "missing.pem"));
    auto invalid = directory_ / "invalid.pem";
    std::ofstream stream;
    open_utf8_path(stream, invalid);
    stream << "invalid certificate";
    stream.close();
    EXPECT_FALSE(socket.load_certificates(invalid));
}

TEST_F(MSWindowsUnicodePathTests, WriteLog)
{
    auto path = directory_ / "input-leap.log";
    FileLogOutputter log(path.u8string().c_str());
    EXPECT_TRUE(log.write(kINFO, "Unicode path log"));
    std::ifstream stream;
    open_utf8_path(stream, path);
    std::string line;
    std::getline(stream, line);
    EXPECT_EQ(line, "Unicode path log");
}

TEST_F(MSWindowsUnicodePathTests, RotateLog)
{
    auto path = directory_ / "input-leap.log";
    FileLogOutputter log(path.u8string().c_str());
    std::string large_message(1024 * 1024 + 1, 'x');
    EXPECT_TRUE(log.write(kINFO, large_message.c_str()));
    auto rotated = directory_ / "input-leap.log.1";
    ASSERT_TRUE(fs::is_regular_file(rotated));
    EXPECT_FALSE(fs::exists(path));
    // A second rotation must replace the previous backup at the same path.
    large_message[0] = 'y';
    EXPECT_TRUE(log.write(kINFO, large_message.c_str()));
    std::ifstream stream;
    open_utf8_path(stream, rotated);
    EXPECT_EQ(stream.get(), 'y');
}

} // namespace inputleap

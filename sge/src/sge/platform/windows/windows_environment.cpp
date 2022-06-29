/*
   Copyright 2022 Nora Beda and SGE contributors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "sgepch.h"
#include "sge/platform/windows/windows_environment.h"

#define NOMINMAX
#include <Windows.h>
#include <ShlObj.h>

namespace sge {
    int32_t windows_run_command(const process_info& info) {
        size_t buffer_size = (info.cmdline.length() + 1) * sizeof(char);
        char* buffer = (char*)malloc(buffer_size);
        memcpy(buffer, info.cmdline.c_str(), buffer_size);

        STARTUPINFOA startup_info;
        memset(&startup_info, 0, sizeof(STARTUPINFOA));
        startup_info.cb = sizeof(STARTUPINFOA);
        startup_info.dwFlags |= STARTF_USESTDHANDLES;

        HANDLE output_file = nullptr;
        std::unique_ptr<SECURITY_ATTRIBUTES> security_attributes;
        if (!info.output_file.empty()) {
            security_attributes = std::make_unique<SECURITY_ATTRIBUTES>();
            security_attributes->nLength = sizeof(SECURITY_ATTRIBUTES);
            security_attributes->lpSecurityDescriptor = nullptr;
            security_attributes->bInheritHandle = true;

            output_file = ::CreateFileW(info.output_file.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE,
                                        security_attributes.get(), CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL, nullptr);

            startup_info.hStdOutput = output_file;
            startup_info.hStdError = output_file;
        }

        if (info.detach) {
            startup_info.hStdInput = INVALID_HANDLE_VALUE;

            if (startup_info.hStdOutput == nullptr) {
                startup_info.hStdOutput = INVALID_HANDLE_VALUE;
            }

            if (startup_info.hStdError == nullptr) {
                startup_info.hStdError = INVALID_HANDLE_VALUE;
            }
        }

        if (startup_info.hStdInput == nullptr) {
            startup_info.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
        }

        if (startup_info.hStdOutput == nullptr) {
            startup_info.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
        }

        if (startup_info.hStdError == nullptr) {
            startup_info.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
        }

        fs::path workdir = info.workdir.empty() ? fs::current_path() : info.workdir;
        std::string workdir_string = workdir.string();

        int32_t exit_code = -1;
        std::string exe_string = info.executable.string();

        PROCESS_INFORMATION win32_process_info;
        if (::CreateProcessA(exe_string.c_str(), buffer, security_attributes.get(), nullptr, true,
                             0, nullptr, workdir_string.c_str(), &startup_info,
                             &win32_process_info)) {
            if (!info.detach) {
                ::WaitForSingleObject(win32_process_info.hProcess,
                                      std::numeric_limits<DWORD>::max());

                DWORD win32_exit_code = 0;
                if (::GetExitCodeProcess(win32_process_info.hProcess, &win32_exit_code)) {
                    exit_code = (int32_t)win32_exit_code;
                }
            } else {
                exit_code = 0;
            }

            ::CloseHandle(win32_process_info.hProcess);
            ::CloseHandle(win32_process_info.hThread);
        }

        if (exit_code < 0) {
            exit_code = (int32_t)::GetLastError();
        }

        free(buffer);
        if (!info.detach && output_file != nullptr) {
            ::CloseHandle(output_file);
        }

        return exit_code;
    }

    struct hkey_data {
        HKEY key = (HKEY)INVALID_HANDLE_VALUE;
        std::string path;
    };

    static std::optional<hkey_data> create_environment_hkey() {
        std::optional<hkey_data> result;

        HKEY key;
        DWORD created_new_key;

        static const char* const key_path = "Environment";
        if (::RegCreateKeyExA(HKEY_CURRENT_USER, key_path, 0, nullptr, REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS, nullptr, &key, &created_new_key) == ERROR_SUCCESS) {
            hkey_data data;

            data.key = key;
            data.path = key_path;

            result = data;
        }

        return result;
    }

    bool windows_setenv(const std::string& key, const std::string& value) {
        auto hkey = create_environment_hkey();
        if (hkey.has_value()) {
            LSTATUS status = ::RegSetValueExA(hkey->key, key.c_str(), 0, REG_SZ,
                                              (LPCBYTE)value.c_str(), value.length() + 1);

            ::RegCloseKey(hkey->key);
            if (status == ERROR_SUCCESS) {
                ::SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                                      (LPARAM)hkey->path.c_str(), SMTO_BLOCK, 100, nullptr);

                return true;
            }
        }

        return false;
    }

    std::string windows_getenv(const std::string& key) {
        std::string result;

        auto hkey = create_environment_hkey();
        if (hkey.has_value()) {
            static DWORD data_size = 512;
            char* data = new char[data_size];

            DWORD value_type;
            LSTATUS status = ::RegGetValueA(hkey->key, nullptr, key.c_str(), RRF_RT_ANY,
                                            &value_type, (PVOID)data, &data_size);

            ::RegCloseKey(hkey->key);
            if (status == ERROR_SUCCESS) {
                result = data;
            }

            delete[] data;
        }

        return result;
    }

    bool windows_hasenv(const std::string& key) {
        bool has_variable = false;

        auto hkey = create_environment_hkey();
        if (hkey.has_value()) {
            LSTATUS status =
                ::RegQueryValueExA(hkey->key, key.c_str(), 0, nullptr, nullptr, nullptr);
            has_variable = status == ERROR_SUCCESS;

            ::RegCloseKey(hkey->key);
        }

        return has_variable;
    }

    fs::path windows_get_home_directory() {
        fs::path result;

        wchar_t path[MAX_PATH];
        if (SUCCEEDED(::SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, path))) {
            result = path;
        }

        return result;
    }

    uint64_t windows_get_process_id() { return (uint64_t)::GetCurrentProcessId(); }
} // namespace sge

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

#include <Windows.h>
#include <ShlObj.h>

namespace sge {
    struct hkey_data {
        HKEY key;
        std::string path;
    };

    static std::optional<hkey_data> create_environment_hkey() {
        std::optional<hkey_data> result;

        HKEY key;
        DWORD created_new_key;

        static const char* const key_path = "Environment";
        if (RegCreateKeyExA(HKEY_CURRENT_USER, key_path, 0, nullptr, REG_OPTION_NON_VOLATILE,
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
            LSTATUS status = RegSetValueExA(hkey->key, key.c_str(), 0, REG_SZ,
                                            (LPCBYTE)value.c_str(), value.length() + 1);

            RegCloseKey(hkey->key);
            if (status == ERROR_SUCCESS) {
                SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)hkey->path.c_str(),
                                    SMTO_BLOCK, 100, nullptr);

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
            LSTATUS status = RegGetValueA(hkey->key, nullptr, key.c_str(), RRF_RT_ANY, &value_type,
                                          (PVOID)data, &data_size);

            RegCloseKey(hkey->key);
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
            LSTATUS status = RegQueryValueExA(hkey->key, key.c_str(), 0, nullptr, nullptr, nullptr);
            has_variable = status == ERROR_SUCCESS;

            RegCloseKey(hkey->key);
        }

        return has_variable;
    }

    fs::path windows_get_home_directory() {
        fs::path result;

        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, path))) {
            result = path;
        }

        return result;
    }
} // namespace sge

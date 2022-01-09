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
#include "sge/platform/desktop/desktop_window.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <commdlg.h>
namespace sge {
    static std::optional<fs::path> open_dialog(HWND owner, const char* filter) {
        char file[MAX_PATH];
        memset(file, 0, MAX_PATH * sizeof(char));

        char current_directory[MAX_PATH];
        memset(file, 0, MAX_PATH * sizeof(char));

        OPENFILENAMEA ofn;
        memset(&ofn, 0, sizeof(OPENFILENAMEA));
        ofn.lStructSize = sizeof(OPENFILENAMEA);

        ofn.hwndOwner = owner;
        ofn.lpstrFile = file;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetCurrentDirectoryA(MAX_PATH, current_directory)) {
            ofn.lpstrInitialDir = current_directory;
        }

        if (GetOpenFileNameA(&ofn) != TRUE) {
            return std::optional<fs::path>();
        }

        return fs::path(file);
    }

    static std::optional<fs::path> save_dialog(HWND owner, const char* filter) {
        char file[MAX_PATH];
        memset(file, 0, MAX_PATH * sizeof(char));

        char current_directory[MAX_PATH];
        memset(file, 0, MAX_PATH * sizeof(char));

        OPENFILENAMEA ofn;
        memset(&ofn, 0, sizeof(OPENFILENAMEA));
        ofn.lStructSize = sizeof(OPENFILENAMEA);

        ofn.hwndOwner = owner;
        ofn.lpstrFile = file;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = strchr(filter, '\0') + 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if (GetCurrentDirectoryA(MAX_PATH, current_directory)) {
            ofn.lpstrInitialDir = current_directory;
        }

        if (GetSaveFileNameA(&ofn) != TRUE) {
            return std::optional<fs::path>();
        }

        return fs::path(file);
    }

    std::optional<fs::path> desktop_window::native_file_dialog(
        dialog_mode mode, const std::vector<dialog_file_filter>& filters) {
        HWND owner = glfwGetWin32Window(m_window);

        std::vector<char> filter;
        for (const auto& filter_data : filters) {
            filter.insert(filter.end(), filter_data.name.begin(), filter_data.name.end());
            filter.push_back('\0');

            filter.insert(filter.end(), filter_data.filter.begin(), filter_data.filter.end());
            filter.push_back('\0');
        }
        filter.push_back('\0');

        std::optional<fs::path> (*dialog_func)(HWND owner, const char* filter);
        switch (mode) {
        case dialog_mode::open:
            dialog_func = open_dialog;
            break;
        case dialog_mode::save:
            dialog_func = save_dialog;
            break;
        default:
            throw std::runtime_error("invalid dialog mode!");
        }

        return dialog_func(owner, filter.data());
    }
} // namespace sge

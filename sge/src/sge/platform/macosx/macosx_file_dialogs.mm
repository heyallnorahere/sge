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
namespace sge {
    std::optional<fs::path> desktop_window::native_file_dialog(
        dialog_mode mode, const std::vector<dialog_file_filter>& filters) {
        spdlog::warn("file dialogs are not yet supported on MacOS X");
        return std::optional<fs::path>();
    }
}
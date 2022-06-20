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
#include "sge/imgui/imgui_extensions.h"
#include "sge/asset/project.h"

using namespace sge;
namespace ImGui {
    bool InputPath(const char* label, fs::path* path, ImGuiInputTextFlags flags,
                   ImGuiInputTextCallback callback, void* user_data) {
        std::string string_path = path->string();

        bool value_changed = InputText(label, &string_path, flags, callback, user_data);
        if (value_changed) {
            *path = string_path;
        }

        return value_changed;
    }

    const std::string READ_ONLY_ASSET = "__READONLY__";
    bool InputAsset(const char* label, ref<asset>* current_value, const std::string& asset_type,
                    const std::string& drag_drop_id) {
        ref<asset> _asset = *current_value;
        ImGui::PushID(ImGui::GetID(label));

        std::string control_value;
        if (_asset) {
            fs::path asset_path = _asset->get_path();
            if (asset_path.empty()) {
                control_value = "<no path>";
            } else {
                control_value = asset_path.filename().string();
            }
        } else {
            control_value = "No " + asset_type + " set";
        }

        ImGui::PushID("filename-display");
        ImGui::InputText(label, &control_value, ImGuiInputTextFlags_ReadOnly);
        ImGui::PopID();

        bool changed = false;
        if (drag_drop_id != READ_ONLY_ASSET) {
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload =
                        ImGui::AcceptDragDropPayload(drag_drop_id.c_str())) {
                    fs::path path = std::string((const char*)payload->Data,
                                                payload->DataSize / sizeof(char) - 1);

                    auto& manager = project::get().get_asset_manager();
                    auto value = manager.get_asset(path);

                    if (!value) {
                        spdlog::error("failed to retrieve asset: {0}", path.string());
                    } else {
                        *current_value = value;
                        changed = true;
                    }
                }

                ImGui::EndDragDropTarget();
            }

            if (_asset) {
                ImGui::SameLine();

                if (ImGui::Button("x")) {
                    *current_value = nullptr;
                    changed = true;
                }
            }
        }

        ImGui::PopID();
        return changed;
    }
} // namespace ImGui
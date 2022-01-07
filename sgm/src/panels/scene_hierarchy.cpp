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

#include "sgmpch.h"
#include "panels/panels.h"
#include "editor_scene.h"
namespace sgm {
    void scene_hierarchy_panel::render() {
        entity& selection = editor_scene::get_selection();
        auto _scene = editor_scene::get_scene();

        size_t index = 0;
        _scene->for_each([&](entity current) {
            if (!current.has_all<tag_component>()) {
                auto& tag = current.add_component<tag_component>();
                tag.tag = "Entity";
            }

            std::string node_id = "entity-" + std::to_string(index);
            ImGuiID id = ImGui::GetID(node_id.c_str());
            ImGui::PushID(id);

            auto& tag = current.get_component<tag_component>();
            bool selected = current == selection;
            if (ImGui::Selectable(tag.tag.c_str(), &selected)) {
                selection = current;
            }

            bool delete_entity = false;
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete Entity")) {
                    delete_entity = true;
                }

                ImGui::EndPopup();
            }

            ImGui::PopID();

            if (delete_entity) {
                if (current == selection) {
                    editor_scene::reset_selection();
                }

                _scene->destroy_entity(current);
            }

            index++;
        });

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
            editor_scene::reset_selection();
        }

        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiMouseButton_Right, false)) {
            if (ImGui::MenuItem("Create New Entity")) {
                _scene->create_entity("Entity");
            }

            ImGui::EndPopup();
        }
    }
} // namespace sgm
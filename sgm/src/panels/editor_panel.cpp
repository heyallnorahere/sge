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
#include <sge/renderer/renderer.h>
#include <imgui_internal.h>
namespace sgm {
    template <typename T>
    static void draw_component(const std::string& name, entity e,
                               std::function<void(T&)> callback) {
        if (e.has_all<T>()) {
            static constexpr ImGuiTreeNodeFlags tree_flags =
                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
                ImGuiTreeNodeFlags_FramePadding;

            T& component = e.get_component<T>();
            ImVec2 region_available = ImGui::GetContentRegionAvail();
            ImGuiContext* imgui_context = ImGui::GetCurrentContext();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 4.f));
            float line_height =
                imgui_context->Font->FontSize + imgui_context->Style.FramePadding.y * 2.f;

            ImGui::Separator();
            bool open = ImGui::TreeNodeEx(name.c_str(), tree_flags);
            ImGui::PopStyleVar();

            ImGui::SameLine(region_available.x - line_height / 2.f);
            if (ImGui::Button("+", ImVec2(line_height, line_height))) {
                ImGui::OpenPopup("component-settings");
            }

            bool remove = false;
            if (ImGui::BeginPopup("component-settings")) {
                if (ImGui::MenuItem("Remove")) {
                    remove = true;
                }

                ImGui::EndPopup();
            }

            if (open) {
                callback(component);
                ImGui::TreePop();
            }

            if (remove) {
                e.remove_component<T>();
            }
        }
    }

    using component_add_callback = std::function<void(entity)>;

    template <typename T>
    static std::pair<std::string, component_add_callback> component_pair(const std::string& text) {
        component_add_callback callback = [](entity e) {
            if (!e.has_all<T>()) {
                e.add_component<T>();
            } else {
                spdlog::warn("attempted to add a component of type {0} to an entity that already "
                             "has the requested component!",
                             typeid(T).name());
            }
        };
        return std::make_pair(text, callback);
    }

    void editor_panel::render() {
        swapchain& swap_chain = application::get().get_swapchain();
        if (m_displayed_textures.empty()) {
            size_t image_count = swap_chain.get_image_count();
            m_displayed_textures.resize(image_count);
        } else {
            size_t current_image = swap_chain.get_current_image_index();
            m_displayed_textures[current_image].clear();
        }

        entity& selection = editor_scene::get_selection();
        if (!selection) {
            ImGui::Text("No entity is selected.");
            return;
        }

        auto& tag = selection.get_component<tag_component>();
        ImGui::InputText("##entity-tag", &tag.tag);

        ImGui::SameLine();
        ImGui::PushItemWidth(-1.f);

        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("add-component");
        }

        if (ImGui::BeginPopup("add-component")) {
            // lets go unnecessary automation
            static std::unordered_map<std::string, component_add_callback> options = {
                component_pair<transform_component>("Transform"),
                component_pair<camera_component>("Camera"),
                component_pair<sprite_renderer_component>("Sprite renderer")
            };

            for (const auto& [text, callback] : options) {
                if (ImGui::MenuItem(text.c_str())) {
                    callback(selection);

                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }

        ImGui::PopItemWidth();

        draw_component<transform_component>(
            "Transform", selection, [](transform_component& transform) {
                ImGui::DragFloat2("Translation", &transform.translation.x);
                ImGui::DragFloat("Rotation", &transform.rotation);
                ImGui::DragFloat2("Scale", &transform.scale.x);
            });

        draw_component<camera_component>("Camera", selection, [](camera_component& camera) {
            ImGui::Checkbox("Primary", &camera.primary);

            std::vector<const char*> type_names = { "Orthographic", "Perspective" };
            int32_t camera_type = (int32_t)camera.camera.get_projection_type();
            if (ImGui::Combo("Camera type", &camera_type, type_names.data(), type_names.size())) {
                camera.camera.set_projection_type((projection_type)camera_type);
            }

            if (camera.camera.get_projection_type() == projection_type::orthographic) {
                float view_size = camera.camera.get_orthographic_size();
                if (ImGui::DragFloat("View size", &view_size)) {
                    camera.camera.set_orthographic_size(view_size);
                }

                float near_clip = camera.camera.get_orthographic_near_plane();
                if (ImGui::DragFloat("Near clip", &near_clip)) {
                    camera.camera.set_orthographic_near_plane(near_clip);
                }

                float far_clip = camera.camera.get_orthographic_far_plane();
                if (ImGui::DragFloat("Far clip", &far_clip)) {
                    camera.camera.set_orthographic_far_plane(far_clip);
                }
            } else {
                float fov = camera.camera.get_vertical_fov();
                if (ImGui::DragFloat("Vertical field of view", &fov)) {
                    camera.camera.set_vertical_fov(fov);
                }

                float near_clip = camera.camera.get_perspective_near_plane();
                if (ImGui::DragFloat("Near clip", &near_clip)) {
                    camera.camera.set_perspective_near_plane(near_clip);
                }

                float far_clip = camera.camera.get_perspective_far_plane();
                if (ImGui::DragFloat("Far clip", &far_clip)) {
                    camera.camera.set_perspective_far_plane(far_clip);
                }
            }
        });

        draw_component<sprite_renderer_component>("Sprite renderer", selection, [this](sprite_renderer_component& component) {
            ImGui::ColorEdit4("Color", &component.color.x);

            auto texture = component.texture;
            bool can_reset = true;

            if (!texture) {
                texture = renderer::get_white_texture();
                can_reset = false;
            }

            // todo: texture loading

            add_texture(texture);
            ImGui::Image(texture->get_imgui_id(), ImVec2(100.f, 100.f));

            if (can_reset) {
                ImGui::SameLine();

                if (ImGui::Button("X")) {
                    component.texture.reset();
                }
            }
        });
    }

    void editor_panel::add_texture(ref<texture_2d> texture) {
        swapchain& swap_chain = application::get().get_swapchain();
        size_t current_image = swap_chain.get_current_image_index();
        m_displayed_textures[current_image].push_back(texture);
    }
} // namespace sgm
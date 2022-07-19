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
#include "sge/scene/scene_serializer.h"
#include "sge/scene/components.h"
#include "sge/script/script_engine.h"
#include "sge/script/script_helpers.h"
#include "sge/script/garbage_collector.h"
#include "sge/asset/json.h"
#include "sge/asset/project.h"
namespace sge {
    static std::unique_ptr<serialization_data> current_serialization;
    serialization_data* serialization_data::current() { return current_serialization.get(); }

    void to_json(json& data, const id_component& comp) { data = comp.id; }
    void from_json(const json& data, id_component& comp) { comp.id = data.get<guid>(); }

    void to_json(json& data, const tag_component& comp) { data = comp.tag; }
    void from_json(const json& data, tag_component& comp) { comp.tag = data.get<std::string>(); }

    void to_json(json& data, const transform_component& comp) {
        data["translation"] = comp.translation;
        data["z_layer"] = comp.z_layer;
        data["rotation"] = comp.rotation;
        data["scale"] = comp.scale;
    }

    void from_json(const json& data, transform_component& comp) {
        comp.translation = data["translation"].get<glm::vec2>();
        comp.z_layer = data["z_layer"].get<int32_t>();
        comp.rotation = data["rotation"].get<float>();
        comp.scale = data["scale"].get<glm::vec2>();
    }

    void to_json(json& data, const camera_component& camera) {
        data["primary"] = camera.primary;

        switch (camera.camera.get_projection_type()) {
        case projection_type::orthographic:
            data["view_size"] = camera.camera.get_orthographic_size();
            data["vertical_fov"] = nullptr;
            data["near_clip"] = camera.camera.get_orthographic_near_plane();
            data["far_clip"] = camera.camera.get_orthographic_far_plane();
            break;
        case projection_type::perspective:
            data["view_size"] = nullptr;
            data["vertical_fov"] = camera.camera.get_vertical_fov();
            data["near_clip"] = camera.camera.get_perspective_near_plane();
            data["far_clip"] = camera.camera.get_perspective_far_plane();
            break;
        default:
            throw std::runtime_error("invalid projection type!");
        }
    }

    void from_json(const json& data, camera_component& camera) {
        camera.primary = data["primary"].get<bool>();

        float near_clip = data["near_clip"].get<float>();
        float far_clip = data["far_clip"].get<float>();

        if (data["vertical_fov"].is_null() && !data["view_size"].is_null()) {
            float view_size = data["view_size"].get<float>();

            camera.camera.set_orthographic(view_size, near_clip, far_clip);
        } else if (data["view_size"].is_null() && !data["vertical_fov"].is_null()) {
            float fov = data["vertical_fov"].get<float>();

            camera.camera.set_perspective(fov, near_clip, far_clip);
        } else {
            throw std::runtime_error("invalid projection type!");
        }
    }

    static json serialize_asset_path(ref<asset> _asset) {
        if (!_asset) {
            return nullptr;
        }

        if (!project::loaded()) {
            throw std::runtime_error("cannot serialize assets without a project loaded!");
        }

        fs::path path = _asset->get_path();
        if (!path.empty()) {
            if (path.is_absolute()) {
                fs::path asset_dir = project::get().get_asset_dir();
                path = path.lexically_relative(asset_dir);
            }

            return path;
        }

        return nullptr;
    }

    template <typename T>
    static ref<T> deserialize_asset_path(const json& data) {
        static_assert(std::is_base_of_v<asset, T>, "given type is not an asset");

        if (data.is_null()) {
            return nullptr;
        }

        if (!project::loaded()) {
            throw std::runtime_error("cannot deserialize assets without a project loaded!");
        }

        fs::path path = data.get<fs::path>();
        ref<asset> _asset = project::get().get_asset_manager().get_asset(path);

        ref<T> result;
        if (_asset) {
            result = _asset.as<T>();
        }

        return result;
    }

    void to_json(json& data, const sprite_renderer_component& comp) {
        data["color"] = comp.color;
        data["texture"] = serialize_asset_path(comp.texture);
        data["shader"] = serialize_asset_path(comp._shader);
    }

    void from_json(const json& data, sprite_renderer_component& comp) {
        comp.color = data["color"].get<glm::vec4>();
        comp.texture = deserialize_asset_path<texture_2d>(data["texture"]);
        comp._shader = deserialize_asset_path<shader>(data["shader"]);
    }

    void to_json(json& data, const rigid_body_component& rb) {
        data["fixed_rotation"] = rb.fixed_rotation;

        using body_type = rigid_body_component::body_type;
        switch (rb.type) {
        case body_type::static_:
            data["type"] = "static";
            break;
        case body_type::kinematic:
            data["type"] = "kinematic";
            break;
        case body_type::dynamic:
            data["type"] = "dynamic";
            break;
        default:
            throw std::runtime_error("invalid body type!");
        }

        data["filter_category"] = rb.filter_category;
        data["filter_mask"] = rb.filter_mask;
    }

    static const std::unordered_map<std::string, rigid_body_component::body_type> body_type_map = {
        { "static", rigid_body_component::body_type::static_ },
        { "kinematic", rigid_body_component::body_type::kinematic },
        { "dynamic", rigid_body_component::body_type::dynamic },
    };

    void from_json(const json& data, rigid_body_component& rb) {
        rb.fixed_rotation = data["fixed_rotation"].get<bool>();

        std::string body_type = data["type"].get<std::string>();
        if (body_type_map.find(body_type) == body_type_map.end()) {
            throw std::runtime_error("invalid body type!");
        }
        rb.type = body_type_map.at(body_type);

        static const std::string category_node_name = "filter_category";
        if (data.find(category_node_name) != data.end()) {
            rb.filter_category = data[category_node_name].get<uint16_t>();
        }

        static const std::string mask_node_name = "filter_mask";
        if (data.find(mask_node_name) != data.end()) {
            rb.filter_mask = data[mask_node_name].get<uint16_t>();
        }
    }

    void to_json(json& data, const box_collider_component& bc) {
        data["density"] = bc.density;
        data["friction"] = bc.friction;
        data["restitution"] = bc.restitution;
        data["restitution_threashold"] = bc.restitution_threashold;
        data["size"] = bc.size;
    }

    void from_json(const json& data, box_collider_component& bc) {
        bc.density = data["density"].get<float>();
        bc.friction = data["friction"].get<float>();
        bc.restitution = data["restitution"].get<float>();
        bc.restitution_threashold = data["restitution_threashold"].get<float>();
        bc.size = data["size"].get<glm::vec2>();
    }

    void to_json(json& data, const script_component& component) {
        if (component._class == nullptr) {
            data = nullptr;
            return;
        }

        data["script_name"] = component.class_name;
        data["enabled"] = component.enabled;
        data["properties"] = nullptr;

        if (component.instance && component.instance->get() != nullptr) {
            void* instance = component.instance->get();

            std::vector<void*> properties;
            script_engine::iterate_properties(component._class, properties);

            if (!properties.empty()) {
                json property_data;
                for (void* property : properties) {
                    if (!script_helpers::is_property_serializable(property)) {
                        continue;
                    }

                    std::string property_name = script_engine::get_property_name(property);
                    json& result = property_data[property_name];

                    script_helpers::serialize_property(instance, property, result);
                }

                data["properties"] = property_data;
            }
        }
    }

    struct script_deserializer {
        void operator()(json property_data, script_component& component) {
            entity current_entity = current_serialization->current_entity;

            component.verify_script(current_entity);
            void* instance = component.instance->get();

            std::vector<void*> properties;
            script_engine::iterate_properties(component._class, properties);

            for (void* property : properties) {
                if (!script_helpers::is_property_serializable(property)) {
                    continue;
                }

                std::string property_name = script_engine::get_property_name(property);
                if (property_data.find(property_name) == property_data.end()) {
                    continue;
                }

                const json& data = property_data[property_name];
                script_helpers::deserialize_property(instance, property, data);
            }
        }
    };

    void from_json(const json& data, script_component& component) {
        component.class_name = data["script_name"].get<std::string>();
        component.enabled = data["enabled"].get<bool>();

        std::optional<size_t> assembly_index = project::get().get_assembly_index();
        if (!assembly_index.has_value()) {
            spdlog::warn("there is no app assembly loaded!");
            return;
        }

        void* app_assembly = script_engine::get_assembly(assembly_index.value());
        component._class = script_engine::get_class(app_assembly, component.class_name);

        json property_data = data["properties"];
        if (!property_data.is_null() && component._class != nullptr) {
            script_deserializer deserializer;
            deserializer(property_data, component);
        }
    }

    template <typename T>
    static void serialize_component(entity e, const std::string& key, json& data) {
        if (e.has_all<T>()) {
            data[key] = e.get_component<T>();
        } else {
            data[key] = nullptr;
        }
    }

    template <typename T>
    static void deserialize_component(entity e, const std::string& key, const json& data) {
        if (data.find(key) == data.end()) {
            return;
        }

        if (data[key].is_null()) {
            return;
        }

        data[key].get_to(e.ensure_component<T>());
    }

    static void new_serialization(ref<scene> _scene) {
        if (current_serialization) {
            throw std::runtime_error("please do not use two serializers at the same time!");
        }

        current_serialization = std::make_unique<serialization_data>();
        current_serialization->_scene = _scene;
    }

    static void serialize_entity(json& data, entity current, bool id = true) {
        current_serialization->current_entity = current;

        if (id) {
            serialize_component<id_component>(current, "guid", data);
        }

        serialize_component<tag_component>(current, "tag", data);
        serialize_component<transform_component>(current, "transform", data);
        serialize_component<camera_component>(current, "camera", data);
        serialize_component<sprite_renderer_component>(current, "sprite", data);
        serialize_component<rigid_body_component>(current, "rigid_body", data);
        serialize_component<box_collider_component>(current, "box_collider", data);
        serialize_component<script_component>(current, "script", data);
    }

    static entity deserialize_entity(const json& data, bool id = true) {
        entity e = current_serialization->_scene->create_entity();
        current_serialization->current_entity = e;

        if (id) {
            deserialize_component<id_component>(e, "guid", data);
        }

        deserialize_component<tag_component>(e, "tag", data);
        deserialize_component<transform_component>(e, "transform", data);
        deserialize_component<camera_component>(e, "camera", data);
        deserialize_component<sprite_renderer_component>(e, "sprite", data);
        deserialize_component<rigid_body_component>(e, "rigid_body", data);
        deserialize_component<box_collider_component>(e, "box_collider", data);
        deserialize_component<script_component>(e, "script", data);

        return e;
    }

    static void run_post_deserialize_tasks() {
        while (!current_serialization->post_deserialize.empty()) {
            auto task = current_serialization->post_deserialize.front();
            current_serialization->post_deserialize.pop();

            task();
        }
    }

    void entity_serializer::serialize(json& data, entity e) {
        new_serialization(e.get_scene());
        serialize_entity(data, e, m_serialize_guid);
        current_serialization.reset();
    }

    entity entity_serializer::deserialize(const json& data, ref<scene> _scene) {
        new_serialization(_scene);
        entity e = deserialize_entity(data, m_serialize_guid);

        run_post_deserialize_tasks();
        current_serialization.reset();

        return e;
    }

    scene_serializer::scene_serializer(ref<scene> _scene) { m_scene = _scene; }

    void scene_serializer::serialize(const fs::path& path) {
        new_serialization(m_scene);
        json data;

        // todo: global scene data

        std::list<json> entities;
        m_scene->for_each([&](entity current) {
            json entity_data;
            serialize_entity(entity_data, current);

            entities.push_front(entity_data);
        });

        std::array<json, scene::collision_category_count> collision_category_names;
        bool insert_category_name_array = false;

        for (size_t i = 0; i < scene::collision_category_count; i++) {
            const std::string& name = m_scene->m_collision_category_names[i];
            if (name.length() > 0) {
                insert_category_name_array |= true;
                collision_category_names[i] = name;
            } else {
                collision_category_names[i] = nullptr;
            }
        }

        json category_name_array;
        if (insert_category_name_array) {
            category_name_array = collision_category_names;
        } else {
            category_name_array = nullptr;
        }

        data["entities"] = entities;
        data["collision_categories"] = category_name_array;
        current_serialization.reset();

        std::ofstream stream(path);
        stream << data.dump(4) << std::flush;
        stream.close();
    }

    void scene_serializer::deserialize(const fs::path& path) {
        if (!fs::exists(path)) {
            spdlog::warn("attempted to deserialize nonexistent scene: {0}", path.string());
            return;
        }

        new_serialization(m_scene);
        json data;
        try {
            std::ifstream stream(path);
            stream >> data;
            stream.close();
        } catch (const std::exception& exc) {
            spdlog::warn("error while reading scene {0}: {1}", path.string(), exc.what());

            current_serialization.reset();
            return;
        }

        m_scene->clear();
        const auto& category_node = data["collision_categories"];
        if (!category_node.is_null()) {
            for (size_t i = 0; i < scene::collision_category_count; i++) {
                const auto& subnode = category_node[i];
                if (!subnode.is_null()) {
                    m_scene->m_collision_category_names[i] = subnode.get<std::string>();
                }
            }
        }

        for (const auto& entity_data : data["entities"]) {
            deserialize_entity(entity_data);
        }

        run_post_deserialize_tasks();
        current_serialization.reset();
    }
} // namespace sge

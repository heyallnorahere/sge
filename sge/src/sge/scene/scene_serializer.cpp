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
#include "sge/scene/entity.h"
#include "sge/scene/components.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace glm {
    void to_json(json& data, const vec2& vec) {
        for (length_t i = 0; i < 2; i++) {
            data.push_back(vec[i]);
        }
    }

    void from_json(const json& data, vec2& vec) {
        if (data.size() != 2) {
            throw std::runtime_error("malformed json");
        }

        for (length_t i = 0; i < 2; i++) {
            vec[i] = data[i].get<float>();
        }
    }

    void to_json(json& data, const vec4& vec) {
        for (length_t i = 0; i < 4; i++) {
            data.push_back(vec[i]);
        }
    }

    void from_json(const json& data, vec4& vec) {
        if (data.size() != 4) {
            throw std::runtime_error("malformed json");
        }

        for (length_t i = 0; i < 4; i++) {
            vec[i] = data[i].get<float>();
        }
    }
} // namespace glm

namespace sge {
    static std::string serialize_path(const fs::path& path) {
        std::string result = path.string();

        size_t pos;
        while ((pos = result.find('\\')) != std::string::npos) {
            result.replace(pos, 1, "/");
        }

        return result;
    }

    void to_json(json& data, const guid& id) { data = (uint64_t)id; }
    void from_json(const json& data, guid& id) { id = data.get<uint64_t>(); }

    void to_json(json& data, const id_component& comp) { data = comp.id; }
    void from_json(const json& data, id_component& comp) { comp.id = data.get<guid>(); }

    void to_json(json& data, const tag_component& comp) { data = comp.tag; }
    void from_json(const json& data, tag_component& comp) { comp.tag = data.get<std::string>(); }

    void to_json(json& data, const transform_component& comp) {
        data["translation"] = comp.translation;
        data["rotation"] = comp.rotation;
        data["scale"] = comp.scale;
    }

    void from_json(const json& data, transform_component& comp) {
        comp.translation = data["translation"].get<glm::vec2>();
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

        if (!data["view_size"].is_null()) {
            float view_size = data["view_size"].get<float>();

            camera.camera.set_orthographic(view_size, near_clip, far_clip);
        } else if (!data["vertical_fov"].is_null()) {
            float fov = data["vertical_fov"].get<float>();

            camera.camera.set_perspective(fov, near_clip, far_clip);
        } else {
            throw std::runtime_error("invalid projection type!");
        }
    }

    void to_json(json& data, const sprite_renderer_component& comp) {
        data["color"] = comp.color;

        if (!comp.texture_path.empty()) {
            data["texture"] = serialize_path(comp.texture_path);
        } else {
            data["texture"] = nullptr;
        }
    }

    void from_json(const json& data, sprite_renderer_component& comp) {
        comp.color = data["color"].get<glm::vec4>();

        if (!data["texture"].is_null()) {
            fs::path path = data["texture"].get<std::string>();

            auto img_data = image_data::load(path);
            if (!img_data) {
                throw std::runtime_error("could not load sprite texture: " + path.string());
            }

            // again, replace once asset system
            texture_spec spec;
            spec.filter = texture_filter::linear;
            spec.wrap = texture_wrap::repeat;
            spec.image = image_2d::create(img_data, image_usage_none);
            comp.texture = texture_2d::create(spec);
            comp.texture_path = path;
        }
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

        T* component;
        if (e.has_all<T>()) {
            component = &e.get_component<T>();
        } else {
            component = &e.add_component<T>();
        }

        *component = data[key].get<T>();
    }

    scene_serializer::scene_serializer(ref<scene> _scene) { m_scene = _scene; }

    void scene_serializer::serialize(const fs::path& path) {
        json data;

        // todo: global scene data

        json entities;
        m_scene->for_each([&](entity current) {
            json entity_data;

            serialize_component<id_component>(current, "guid", entity_data);
            serialize_component<tag_component>(current, "tag", entity_data);
            serialize_component<transform_component>(current, "transform", entity_data);
            serialize_component<camera_component>(current, "camera", entity_data);
            serialize_component<sprite_renderer_component>(current, "sprite", entity_data);
            serialize_component<rigid_body_component>(current, "rigid_body", entity_data);
            serialize_component<box_collider_component>(current, "box_collider", entity_data);

            entities.push_back(entity_data);
        });
        data["entities"] = entities;

        std::ofstream stream(path);
        stream << data.dump(4) << std::flush;
        stream.close();
    }

    void scene_serializer::deserialize(const fs::path& path) {
        if (!fs::exists(path)) {
            spdlog::warn("attempted to deserialize nonexistent scene: {0}", path.string());
            return;
        }

        json data;
        std::ifstream stream(path);
        stream >> data;
        stream.close();

        m_scene->clear();

        for (const auto& entity_data : data["entities"]) {
            entity e = m_scene->create_entity();

            deserialize_component<id_component>(e, "guid", entity_data);
            deserialize_component<tag_component>(e, "tag", entity_data);
            deserialize_component<transform_component>(e, "transform", entity_data);
            deserialize_component<camera_component>(e, "camera", entity_data);
            deserialize_component<sprite_renderer_component>(e, "sprite", entity_data);
            deserialize_component<rigid_body_component>(e, "rigid_body", entity_data);
            deserialize_component<box_collider_component>(e, "box_collider", entity_data);
        }
    }
} // namespace sge

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

#pragma once
#include "sge/scene/scene.h"
#include "sge/asset/json.h"
namespace sge {
    class entity_serializer {
    public:
        entity_serializer(bool serialize_guid = true) : m_serialize_guid(serialize_guid) {}
        ~entity_serializer() = default;

        void serialize(json& data, entity e);
        entity deserialize(const json& data, ref<scene> _scene);

    private:
        bool m_serialize_guid;
    };

    class scene_serializer {
    public:
        scene_serializer(ref<scene> _scene);
        ~scene_serializer() = default;

        void serialize(const fs::path& path);
        void deserialize(const fs::path& path);

    private:
        ref<scene> m_scene;
    };
} // namespace sge

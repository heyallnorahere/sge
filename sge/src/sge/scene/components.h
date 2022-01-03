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

namespace sge {
    struct tag_component {
        std::string tag;

        tag_component() = default;
        tag_component(const tag_component&) = default;
        tag_component(const std::string& t) : tag(t) {}
    };

    struct transform_component {
        glm::vec2 translation = glm::vec2(0.f, 0.f);
        float rotation = 0.f; // In degrees
        glm::vec2 scale = glm::vec2(1.f, 1.f);

        transform_component() = default;
        transform_component(const transform_component&) = default;
        transform_component(const glm::vec3& t) : translation(t) {}

        glm::mat4 get_transform() const {
            return glm::translate(glm::mat4(1.0f), glm::vec3(translation, 0.f)) *
                   glm::rotate(glm::mat4(1.f), glm::radians(rotation), glm::vec3(0.f, 0.f, 1.f)) *
                   glm::scale(glm::mat4(1.0f), glm::vec3(scale, 1.0f));
        }
    };

    struct quad_component {
        glm::vec2 position;
        glm::vec2 size;
        glm::vec4 color;
    };
} // namespace sge
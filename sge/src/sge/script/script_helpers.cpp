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
#include "sge/script/script_helpers.h"
#include "sge/script/script_engine.h"
namespace sge {
    static void* managed_helpers_class = nullptr;
    void script_helpers::init(void* helpers_class) { managed_helpers_class = helpers_class; }

    bool script_helpers::property_has_attribute(void* property, void* attribute_type) {
        void* reflection_property = script_engine::to_reflection_property(property);
        void* reflection_type = script_engine::to_reflection_type(attribute_type);

        void* method = script_engine::get_method(managed_helpers_class, "PropertyHasAttribute");
        void* returned =
            script_engine::call_method(nullptr, method, reflection_type, reflection_type);
        return script_engine::unbox_object<bool>(returned);
    }

    bool script_helpers::is_property_serializable(void* property) {
        bool serializable = true;

        void* unserialized_attribute = script_engine::get_class(0, "SGE.UnserializedAttribute");
        serializable &= ~property_has_attribute(property, unserialized_attribute);

        uint32_t accessors = script_engine::get_property_accessors(property);
        serializable &= ((accessors & (property_accessor_get | property_accessor_set)) !=
                         property_accessor_none);

        uint32_t visibility = script_engine::get_property_visibility(property);
        serializable &=
            ((visibility & member_visibility_flags_public) != member_visibility_flags_none);

        return serializable;
    }
} // namespace sge
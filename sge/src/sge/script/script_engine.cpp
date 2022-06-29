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
#include "sge/asset/project.h"
#include "sge/script/script_engine.h"
#include "sge/script/mono_include.h"
#include "sge/script/garbage_collector.h"
#include "sge/script/script_helpers.h"
#include "sge/script/value_wrapper.h"
#include "sge/scene/components.h"
#include "sge/core/environment.h"
#include "sge/core/application.h"

namespace sge {
    struct assembly_t {
        MonoAssembly* assembly = nullptr;
        MonoImage* image = nullptr;
        fs::path path;
    };

    struct script_engine_data_t {
        MonoDomain* root_domain = nullptr;
        MonoDomain* script_domain = nullptr;
        std::vector<assembly_t> assemblies;
        bool debug_enabled;

        bool reload_callbacks_locked = false;
        std::vector<std::optional<std::function<void()>>> reload_callbacks;
    };

    static std::unique_ptr<script_engine_data_t> script_engine_data;
    static void script_engine_init_internal() {
        char domain_name[16];
        strcpy(domain_name, "SGE-Runtime");

        script_engine_data->script_domain = mono_domain_create_appdomain(domain_name, nullptr);
        mono_domain_set(script_engine_data->script_domain, false);
        garbage_collector::init();

        if (script_engine_data->assemblies.empty()) {
            script_engine::load_assembly(fs::current_path() / "assets" / "assemblies" /
                                         "SGE.Scriptcore.dll");

            script_engine::register_internal_script_calls();
            script_helpers::init();
        }
    }

    class debugger_agent {
    public:
        void set(const std::string& key, const std::string& value) { m_data[key] = value; }
        void set(const std::string& key, const char* value) { m_data[key] = value; }
        void set(const std::string& key, bool value) { m_data[key] = value ? "y" : "n"; }

        void set(const std::string& key, const std::string& address, const std::string& port) {
            m_data[key] = address + ":" + port;
        }

        operator std::string() const {
            std::string data;
            for (const auto& [key, value] : m_data) {
                if (!data.empty()) {
                    data += ",";
                }

                data += key + "=" + value;
            }

            return "--debugger-agent=" + data;
        }

    private:
        std::unordered_map<std::string, std::string> m_data;
    };

    void script_engine::init() {
        if (script_engine_data) {
            throw std::runtime_error("the script engine has already been initialized!");
        }

        script_engine_data = std::make_unique<script_engine_data_t>();
        script_engine_data->debug_enabled = application::get().is_editor();

        std::string assembly_path = (fs::current_path() / "assets").string();
        mono_set_assemblies_path(assembly_path.c_str());
        mono_config_parse(nullptr);

        std::vector<std::string> args;
        if (script_engine_data->debug_enabled) {
            debugger_agent agent;

            agent.set("transport", "dt_socket");
            agent.set("server", true);
            agent.set("address", SGE_DEBUGGER_AGENT_ADDRESS, SGE_DEBUGGER_AGENT_PORT);
            agent.set("logfile", "assets/logs/mono-debugger.log");
            agent.set("loglevel", "10");

            args.insert(args.end(), { "--breakonex", "--soft-breakpoints", agent });
        }

        std::vector<char*> jit_args;
        for (const auto& str : args) {
            char* buffer = (char*)malloc((str.length() + 1) * sizeof(char));
            strcpy(buffer, str.c_str());

            jit_args.push_back(buffer);
        }

        mono_jit_parse_options((int)jit_args.size(), jit_args.data());
        script_engine_data->root_domain = mono_jit_init("SGE");

        if (script_engine_data->debug_enabled) {
            mono_debug_init(MONO_DEBUG_FORMAT_MONO);
            mono_debug_domain_create(script_engine_data->root_domain);
        }

        for (auto buffer : jit_args) {
            free(buffer);
        }

        script_engine_init_internal();
    }

    static void script_engine_shutdown_internal() {
        garbage_collector::shutdown();
        mono_domain_set(script_engine_data->root_domain, false);
        mono_domain_unload(script_engine_data->script_domain);
    }

    void script_engine::shutdown() {
        if (!script_engine_data) {
            throw std::runtime_error("the script engine is not initialized!");
        }

        script_engine_shutdown_internal();
        mono_jit_cleanup(script_engine_data->root_domain);
        script_engine_data.reset();
    }

    void script_engine::register_internal_call(const std::string& name, const void* callback) {
        mono_add_internal_call(name.c_str(), callback);
    }

    bool script_engine::compile_app_assembly() {
#ifdef SGE_BUILD_SCRIPTCORE
        auto& _project = project::get();

        fs::path project_path = _project.get_script_project_path();
        if (!fs::exists(project_path)) {
            return false;
        }

        static const std::vector<std::string> msbuild_args = SGE_MSBUILD_ARGS;
        std::stringstream args;
        for (const auto& argument : msbuild_args) {
            args << std::quoted(argument) << " ";
        }

        args << "-r -nologo -p:Configuration=" << project::get_config() << " "
             << std::quoted(project_path.string());

        process_info p_info;
        p_info.executable = SGE_MSBUILD_EXE;
        p_info.cmdline = args.str();

        environment::run_command(p_info);
#endif

        return true;
    }

    static std::optional<size_t> find_viable_assembly_index() {
        std::optional<size_t> index;

        for (size_t i = 0; i < script_engine_data->assemblies.size(); i++) {
            const auto& assembly = script_engine_data->assemblies[i];

            if (assembly.assembly == nullptr) {
                index = i;
                break;
            }
        }

        return index;
    }

    static bool load_assembly_raw(const fs::path& path, assembly_t& assembly) {
        std::string string_path = path.string();
        if (!fs::exists(path)) {
            spdlog::warn("assembly does not exist: {0}", string_path);
            return false;
        } else {
            MonoImageOpenStatus status;

            assembly.assembly = mono_assembly_open_full(string_path.c_str(), &status, false);
            assembly.image = mono_assembly_get_image(assembly.assembly);
            assembly.path = path;

            if (status != MONO_IMAGE_OK) {
                std::string error_message = mono_image_strerror(status);
                spdlog::warn("could not open {0}: ", string_path, error_message);
                return false;
            } else {
                return true;
            }
        }
    }

    std::optional<size_t> script_engine::load_assembly(const fs::path& path) {
        std::string string_path = path.string();
        for (size_t i = 0; i < script_engine_data->assemblies.size(); i++) {
            if (script_engine_data->assemblies[i].path == path &&
                script_engine_data->assemblies[i].assembly != nullptr) {
                spdlog::warn("attempted to load {0} more than once", string_path);
                return i;
            }
        }

        std::optional<size_t> assembly_index;
        assembly_t data;

        if (load_assembly_raw(path, data)) {
            auto found_index = find_viable_assembly_index();
            if (found_index.has_value()) {
                size_t index = found_index.value();
                script_engine_data->assemblies[index] = data;
                return index;
            } else {
                size_t index = script_engine_data->assemblies.size();
                script_engine_data->assemblies.push_back(data);
                return index;
            }
        }

        return assembly_index;
    }

    static bool unload_assembly_raw(size_t index) {
        if (index >= script_engine_data->assemblies.size()) {
            return false;
        }

        auto& data = script_engine_data->assemblies[index];
        if (data.assembly == nullptr) {
            return false;
        }

        data.assembly = nullptr;
        data.image = nullptr;

        return true;
    }

    bool script_engine::unload_assembly(size_t index) {
        if (index == 0) {
            throw std::runtime_error("index 0 is reserved!");
        }

        bool succeeded = unload_assembly_raw(index);
        if (succeeded) {
            script_engine_data->assemblies[index].path.clear();
        }

        return succeeded;
    }

    static void reinitialize_script_engine(const std::optional<std::function<bool()>>& pre_reload) {
        for (size_t i = 0; i < script_engine_data->assemblies.size(); i++) {
            if (!unload_assembly_raw(i)) {
                throw std::runtime_error("Failed to unload assembly " + std::to_string(i) + "!");
            }
        }

        script_engine_shutdown_internal();
        if (pre_reload.has_value() && !pre_reload.value()()) {
            throw std::runtime_error("Pre-reload callback failed!");
        }

        script_engine_init_internal();
        for (auto& assembly : script_engine_data->assemblies) {
            const auto& path = assembly.path;

            if (!path.empty()) {
                assembly_t data;

                if (load_assembly_raw(path, data)) {
                    assembly = data;
                } else {
                    throw std::runtime_error("Failed to reload assembly!");
                }
            }
        }

        // what with reloading assemblies
        script_engine::register_component_types();
        script_helpers::init();
    }

    enum class script_property_type { value, _entity, array, list, _asset };
    struct script_property_data_t {
        value_wrapper data;
        std::vector<value_wrapper> array;
        void* array_element_type;

        std::string type_name;
        script_property_type property_type;
    };

    // based on Hazel::ScriptEngine::ReloadAssembly, Hazel-dev branch dotnet6
    void script_engine::reload_assemblies(const std::vector<ref<scene>>& current_scenes,
                                          const std::optional<std::function<bool()>>& pre_reload) {
        std::vector<
            std::unordered_map<guid, std::unordered_map<std::string, script_property_data_t>>>
            old_property_values;

        void* asset_class = script_helpers::get_core_type("SGE.Asset", true);
        for (ref<scene> _scene : current_scenes) {
            std::unordered_map<guid, std::unordered_map<std::string, script_property_data_t>>
                entity_data;

            _scene->for_each<script_component>([&](entity e) {
                auto& sc = e.get_component<script_component>();
                if (sc.gc_handle == 0) {
                    return; // if the script isn't already initialized, theres nothing to save
                }

                void* _class = sc._class;
                void* instance = garbage_collector::get_ref_data(sc.gc_handle);

                std::vector<void*> properties;
                iterate_properties(_class, properties);

                std::unordered_map<std::string, script_property_data_t> property_values;
                for (void* property : properties) {
                    if (!script_helpers::is_property_serializable(property)) {
                        continue;
                    }

                    script_property_data_t result;
                    void* type = get_property_type(property);
                    result.type_name = script_helpers::get_type_name_safe(type);

                    void* property_data = get_property_value(instance, property);
                    if (is_value_type(type)) {
                        size_t size = script_helpers::get_type_size(type);

                        const void* unboxed = unbox_object(property_data);
                        result.data = value_wrapper(unboxed, size);

                        result.property_type = script_property_type::value;
                    } else {
                        static const std::vector<std::string> list_type_names = {
                            "System.Collections.Generic.IList",
                            "System.Collections.Generic.IReadOnlyList",
                            "System.Collections.Generic.List"
                        };

                        bool list = false;
                        for (const auto& list_name : list_type_names) {
                            if (result.type_name.substr(0, list_name.length()) == list_name) {
                                list = true;
                                break;
                            }
                        }

                        if (list) {
                            void* object_type = get_class_from_object(property_data);
                            void* count_property = get_property(object_type, "Count");

                            void* returned = get_property_value(property_data, count_property);
                            int32_t count = unbox_object<int32_t>(returned);

                            void* item_property = get_property(object_type, "Item");
                            result.array_element_type = get_property_type(item_property);
                            if (!is_value_type(result.array_element_type)) { // for now
                                continue;
                            }

                            size_t item_size =
                                script_helpers::get_type_size(result.array_element_type);
                            for (int32_t i = 0; i < count; i++) {
                                void* item = get_property_value(property_data, item_property, &i);
                                const void* unboxed_data = unbox_object(item);

                                result.array.emplace_back(unboxed_data, item_size);
                            }

                            result.property_type = script_property_type::list;
                        } else if (result.type_name == "SGE.Entity") {
                            if (property_data != nullptr) {
                                entity e = script_helpers::get_entity_from_object(property_data);
                                guid id = e.get_guid();

                                result.data = value_wrapper(&id, sizeof(guid));
                            }

                            result.property_type = script_property_type::_entity;
                        } else if (script_helpers::type_is_array(type)) {
                            result.array_element_type = get_array_element_type(property_data);
                            if (!is_value_type(result.array_element_type)) {
                                continue; // for now
                            }

                            size_t size = get_array_length(property_data);
                            size_t element_size =
                                script_helpers::get_type_size(result.array_element_type);

                            for (size_t i = 0; i < size; i++) {
                                void* boxed = get_array_element(property_data, i);
                                const void* unboxed = unbox_object(boxed);

                                result.array.emplace_back(unboxed, element_size);
                            }

                            result.property_type = script_property_type::array;
                        } else if (script_helpers::type_extends(type, asset_class)) {
                            auto _asset = script_helpers::get_asset_from_object(property_data);
                            if (_asset) {
                                result.data = value_wrapper(&_asset->id, sizeof(guid));
                            }

                            result.property_type = script_property_type::_asset;
                        }
                    }

                    std::string name = get_property_name(property);
                    property_values.insert(std::make_pair(name, result));
                }

                guid id = e.get_guid();
                entity_data.insert(std::make_pair(id, property_values));

                garbage_collector::destroy_ref(sc.gc_handle);
                sc.gc_handle = 0;
                sc._class = nullptr;
            });

            old_property_values.push_back(entity_data);
        }

        reinitialize_script_engine(pre_reload);
        for (size_t scene_index = 0; scene_index < old_property_values.size(); scene_index++) {
            auto _scene = current_scenes[scene_index];
            const auto& entity_map = old_property_values[scene_index];

            for (const auto& [entity_id, property_values] : entity_map) {
                entity current_entity = _scene->find_guid(entity_id);
                if (!current_entity) {
                    continue;
                }

                auto& sc = current_entity.get_component<script_component>();
                for (const auto& assembly : script_engine_data->assemblies) {
                    if (assembly.image == nullptr) {
                        continue;
                    }

                    void* _class = get_class(assembly.image, sc.class_name);
                    if (_class != nullptr) {
                        sc._class = _class;
                        break;
                    }
                }

                if (sc._class == nullptr) {
                    spdlog::warn(
                        "between reloads, script class {0} was deleted - deleting script data",
                        sc.class_name);

                    std::vector<void*> classes;
                    iterate_classes(script_engine_data->assemblies[1].image, classes);

                    spdlog::info("classes in the user script assembly:");
                    for (void* _class : classes) {
                        class_name_t name;
                        get_class_name(_class, name);
                        spdlog::info("\t{0}", get_string(name));
                    }

                    sc.class_name.clear();
                    continue;
                }

                sc.verify_script(current_entity);
                void* script_object = garbage_collector::get_ref_data(sc.gc_handle);

                for (const auto& [property_name, data] : property_values) {
                    void* property = get_property(sc._class, property_name);
                    if (property == nullptr) {
                        spdlog::warn("between reloads, script property {0}.{1} was deleted - "
                                     "deleting its data",
                                     sc.class_name, property_name);

                        continue;
                    }

                    switch (data.property_type) {
                    case script_property_type::array: {
                        auto mono_element_type = (MonoClass*)data.array_element_type;
                        auto mono_array =
                            mono_array_new(script_engine_data->script_domain, mono_element_type,
                                           (uintptr_t)data.array.size());

                        for (size_t i = 0; i < data.array.size(); i++) {
                            const auto& value = data.array[i];
                            void* addr = mono_array_addr_with_size(mono_array, (int)value.size(),
                                                                   (uintptr_t)i);

                            memcpy(addr, value.ptr(), value.size());
                        }

                        set_property_value(script_object, property, mono_array);
                    } break;
                    case script_property_type::_entity: {
                        void* entity_object = nullptr;
                        if (data.data) {
                            guid id = data.data.get<guid>();
                            entity value = _scene->find_guid(id);

                            entity_object = script_helpers::create_entity_object(value);
                        }

                        set_property_value(script_object, property, entity_object);
                    } break;
                    case script_property_type::list: {
                        void* list = script_helpers::create_list_object(data.array_element_type);
                        void* list_type = get_class_from_object(list);

                        void* add_method = get_method(list_type, "Add");
                        for (const auto& element : data.array) {
                            call_method(list, add_method, (void*)element.ptr());
                        }

                        set_property_value(script_object, property, list);
                    } break;
                    case script_property_type::value:
                        set_property_value(script_object, property, (void*)data.data.ptr());
                        break;
                    case script_property_type::_asset: {
                        void* value = nullptr;
                        if (data.data) {
                            auto& manager = project::get().get_asset_manager();

                            guid id = data.data.get<guid>();
                            auto _asset = manager.get_asset(id);

                            value = script_helpers::create_asset_object(_asset);
                        }

                        set_property_value(script_object, property, value);
                    } break;
                    }
                }
            }
        }

        script_engine_data->reload_callbacks_locked = true;
        for (const auto& callback : script_engine_data->reload_callbacks) {
            if (callback.has_value()) {
                callback.value()();
            }
        }

        script_engine_data->reload_callbacks_locked = false;
    }

    size_t script_engine::add_on_reload_callback(const std::function<void()>& callback) {
        if (script_engine_data->reload_callbacks_locked) {
            throw std::runtime_error("reload callbacks have been locked!");
        }

        for (size_t i = 0; i < script_engine_data->reload_callbacks.size(); i++) {
            auto& element = script_engine_data->reload_callbacks[i];

            if (!element.has_value()) {
                element = callback;
                return i;
            }
        }

        size_t index = script_engine_data->reload_callbacks.size();
        script_engine_data->reload_callbacks.emplace_back(callback);
        return index;
    }

    bool script_engine::remove_on_reload_callback(size_t index) {
        if (script_engine_data->reload_callbacks_locked) {
            throw std::runtime_error("reload callbacks have been locked!");
        }

        if (index >= script_engine_data->reload_callbacks.size()) {
            return false;
        }

        auto& callback = script_engine_data->reload_callbacks[index];
        if (!callback.has_value()) {
            return false;
        }

        callback.reset();
        return true;
    }

    size_t script_engine::get_assembly_count() { return script_engine_data->assemblies.size(); }

    fs::path script_engine::get_assembly_path(size_t index) {
        fs::path assembly_path;

        if (index < script_engine_data->assemblies.size()) {
            assembly_path = script_engine_data->assemblies[index].path;
        }

        return assembly_path;
    }

    std::string script_engine::get_assembly_name(void* assembly) {
        auto mono_image = (MonoImage*)assembly;
        auto mono_assembly = mono_image_get_assembly(mono_image);

        auto assembly_name = mono_assembly_get_name(mono_assembly);
        std::string name = mono_assembly_name_get_name(assembly_name);
        mono_assembly_name_free(assembly_name);

        return name;
    }

    void* script_engine::get_assembly(size_t index) {
        if (index >= script_engine_data->assemblies.size()) {
            return nullptr;
        }

        return script_engine_data->assemblies[index].image;
    }

    void* script_engine::get_assembly_from_class(void* _class) {
        if (_class == nullptr) {
            return nullptr;
        }

        auto mono_class = (MonoClass*)_class;
        return mono_class_get_image(mono_class);
    }

    void* script_engine::get_mscorlib() { return mono_get_corlib(); }

    std::string script_engine::get_string(const class_name_t& class_name) {
        if (!class_name.namespace_name.empty()) {
            return class_name.namespace_name + "." + class_name.class_name;
        } else {
            return class_name.class_name;
        }
    }

    void script_engine::get_class_name(void* _class, class_name_t& name) {
        auto mono_class = (MonoClass*)_class;

        name.namespace_name = mono_class_get_namespace(mono_class);
        name.class_name = mono_class_get_name(mono_class);
    }

    void script_engine::iterate_classes(void* assembly, std::vector<void*>& classes) {
        auto image = (MonoImage*)assembly;
        auto table = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
        int32_t row_count = mono_table_info_get_rows(table);

        classes.clear();
        for (int32_t i = 1; i < row_count; i++) {
            auto _class = mono_class_get(image, (i + 1) | MONO_TOKEN_TYPE_DEF);
            classes.push_back(_class);
        }
    }

    void* script_engine::get_class(void* assembly, const std::string& name) {
        class_name_t class_name;
        class_name.class_name = name;

        size_t separator_pos = name.find_last_of('.');
        if (separator_pos != std::string::npos) {
            class_name.class_name = name.substr(separator_pos + 1);
            class_name.namespace_name = name.substr(0, separator_pos);
        }

        return get_class(assembly, class_name);
    }

    void* script_engine::get_class(void* assembly, const class_name_t& name) {
        auto image = (MonoImage*)assembly;
        return mono_class_from_name(image, name.namespace_name.c_str(), name.class_name.c_str());
    }

    void* script_engine::get_class_from_object(void* object) {
        auto mono_object = (MonoObject*)object;
        return mono_object_get_class(mono_object);
    }

    void* script_engine::get_base_class(void* derived) {
        auto mono_class = (MonoClass*)derived;
        return mono_class_get_parent(mono_class);
    }

    bool script_engine::is_value_type(void* _class) {
        auto mono_class = (MonoClass*)_class;
        return mono_class_is_valuetype(mono_class);
    }

    void* script_engine::alloc_object(void* _class) {
        auto mono_class = (MonoClass*)_class;

        void* object = mono_object_new(script_engine_data->script_domain, mono_class);
        if (object == nullptr) {
            throw std::runtime_error("could not create object!");
        }

        return object;
    }

    void* script_engine::clone_object(void* original) {
        if (original == nullptr) {
            return nullptr;
        }

        return mono_object_clone((MonoObject*)original);
    }

    void script_engine::init_object(void* object) {
        auto mono_object = (MonoObject*)object;
        mono_runtime_object_init(mono_object);
    }

    const void* script_engine::unbox_object(void* object) {
        auto mono_object = (MonoObject*)object;
        return mono_object_unbox(mono_object);
    }

    size_t script_engine::get_array_length(void* array) {
        auto mono_array = (MonoArray*)array;
        return (size_t)mono_array_length(mono_array);
    }

    void* script_engine::get_array_element_type(void* array) {
        void* array_type = get_class_from_object(array);
        auto mono_class = (MonoClass*)array_type;
        return mono_class_get_element_class(mono_class);
    }

    void* script_engine::get_array_element(void* array, size_t index) {
        void* array_type = get_class_from_object(array);
        auto mono_class = (MonoClass*)array_type;

        MonoClass* element_type = mono_class_get_element_class(mono_class);
        bool value_type = mono_class_is_valuetype(element_type);

        auto mono_array = (MonoArray*)array;
        int32_t element_size = mono_array_element_size(mono_class);
        void* ptr = mono_array_addr_with_size(mono_array, element_size, (uintptr_t)index);

        if (value_type) {
            return mono_value_box(script_engine_data->script_domain, element_type, ptr);
        } else {
            return *(void**)ptr;
        }
    }

    void* script_engine::get_method(void* _class, const std::string& name) {
        std::string method_desc = "*:" + name;
        auto mono_desc = mono_method_desc_new(method_desc.c_str(), false);

        auto mono_class = (MonoClass*)_class;
        return mono_method_desc_search_in_class(mono_desc, mono_class);
    }

    std::string script_engine::get_method_name(void* method) {
        auto mono_method = (MonoMethod*)method;
        return mono_method_get_name(mono_method);
    }

    void* script_engine::get_method_return_type(void* method) {
        auto mono_method = (MonoMethod*)method;
        auto signature = mono_method_signature(mono_method);

        auto return_type = mono_signature_get_return_type(signature);
        return mono_class_from_mono_type(return_type);
    }

    static uint32_t get_visibility_from_flags(uint32_t flags) {
        uint32_t visibility = member_visibility_flags_none;

        switch (flags & MONO_METHOD_ATTR_ACCESS_MASK) {
        case MONO_METHOD_ATTR_PUBLIC:
            visibility |= member_visibility_flags_public;
            break;
        case MONO_METHOD_ATTR_FAMILY:
            visibility |= member_visibility_flags_protected;
            break;
        case MONO_METHOD_ATTR_PRIVATE:
            visibility |= member_visibility_flags_private;
            break;
        case MONO_METHOD_ATTR_ASSEM:
            visibility |= member_visibility_flags_internal;
            break;
        default:
            throw std::runtime_error("invalid access attribute!");
        }

        if ((flags & MONO_METHOD_ATTR_STATIC) != 0) {
            visibility |= member_visibility_flags_static;
        }

        return visibility;
    }

    uint32_t script_engine::get_method_visibility(void* method) {
        uint32_t visibility = member_visibility_flags_none;

        auto mono_method = (MonoMethod*)method;
        uint32_t flags = mono_method_get_flags(mono_method, nullptr);

        return get_visibility_from_flags(flags);
    }

    void script_engine::get_method_parameters(void* method,
                                              std::vector<method_parameter_t>& parameters) {
        auto mono_method = (MonoMethod*)method;
        auto signature = mono_method_signature(mono_method);

        uint32_t param_count = mono_signature_get_param_count(signature);
        auto names = (const char**)malloc(sizeof(const char*) * param_count);
        mono_method_get_param_names(mono_method, names);

        MonoType* type = nullptr;
        void* iterator = nullptr;

        parameters.clear();
        size_t parameter_index = 0;
        while ((type = mono_signature_get_params(signature, &iterator)) != nullptr) {
            method_parameter_t parameter;
            parameter.name = names[parameter_index];
            parameter.type = mono_class_from_mono_type(type);
            parameters.push_back(parameter);

            parameter_index++;
        }

        free(names);
    }

    void script_engine::iterate_methods(void* _class, std::vector<void*>& methods) {
        auto mono_class = (MonoClass*)_class;

        MonoMethod* method = nullptr;
        void* iterator = nullptr;

        methods.clear();
        while ((method = mono_class_get_methods(mono_class, &iterator)) != nullptr) {
            methods.push_back(method);
        }
    }

    void* script_engine::call_method(void* object, void* method, void** arguments) {
        if (method == nullptr) {
            throw std::runtime_error("attempted to call nullptr!");
        }
        auto mono_method = (MonoMethod*)method;

        MonoObject* exc = nullptr;
        void* returned = mono_runtime_invoke(mono_method, object, arguments, &exc);
        handle_exception(exc);

        return returned;
    }

    void script_engine::handle_exception(void* exception) {
        if (exception != nullptr) {
            script_helpers::report_exception(exception);
        }
    }

    void* script_engine::get_property(void* _class, const std::string& name) {
        auto mono_class = (MonoClass*)_class;
        return mono_class_get_property_from_name(mono_class, name.c_str());
    }

    void script_engine::iterate_properties(void* _class, std::vector<void*>& properties) {
        auto mono_class = (MonoClass*)_class;

        MonoProperty* property = nullptr;
        void* iterator = nullptr;

        properties.clear();
        while ((property = mono_class_get_properties(mono_class, &iterator)) != nullptr) {
            properties.push_back(property);
        }
    }

    std::string script_engine::get_property_name(void* property) {
        auto mono_property = (MonoProperty*)property;
        return mono_property_get_name(mono_property);
    }

    void* script_engine::get_property_type(void* property) {
        auto mono_property = (MonoProperty*)property;

        auto method = mono_property_get_get_method(mono_property);
        if (method != nullptr) {
            return get_method_return_type(method);
        }

        method = mono_property_get_set_method(mono_property);
        if (method != nullptr) {
            void* iterator = nullptr;
            auto signature = mono_method_signature(method);

            auto type = mono_signature_get_params(signature, &iterator);
            return mono_class_from_mono_type(type);
        }

        return nullptr;
    }

    uint32_t script_engine::get_property_accessors(void* property) {
        auto mono_property = (MonoProperty*)property;
        uint32_t accessors = property_accessor_none;

        auto method = mono_property_get_get_method(mono_property);
        if (method != nullptr) {
            accessors |= property_accessor_get;
        }

        method = mono_property_get_set_method(mono_property);
        if (method != nullptr) {
            accessors |= property_accessor_set;
        }

        return accessors;
    }

    uint32_t script_engine::get_property_visibility(void* property) {
        auto mono_property = (MonoProperty*)property;

        MonoMethod* property_method = mono_property_get_get_method(mono_property);
        if (property_method == nullptr) {
            property_method = mono_property_get_set_method(mono_property);
        }

        return get_method_visibility(property_method);
    }

    void* script_engine::get_field(void* _class, const std::string& name) {
        auto mono_class = (MonoClass*)_class;
        return mono_class_get_field_from_name(mono_class, name.c_str());
    }

    void script_engine::iterate_fields(void* _class, std::vector<void*>& fields) {
        auto mono_class = (MonoClass*)_class;

        MonoClassField* field = nullptr;
        void* iterator = nullptr;

        fields.clear();
        while ((field = mono_class_get_fields(mono_class, &iterator)) != nullptr) {
            fields.push_back(field);
        }
    }

    std::string script_engine::get_field_name(void* field) {
        auto mono_field = (MonoClassField*)field;
        return mono_field_get_name(mono_field);
    }

    void* script_engine::get_field_type(void* field) {
        auto mono_field = (MonoClassField*)field;
        auto type = mono_field_get_type(mono_field);
        return mono_class_from_mono_type(type);
    }

    void* script_engine::to_managed_string(const std::string& native_string) {
        return mono_string_new(script_engine_data->script_domain, native_string.c_str());
    }

    std::string script_engine::from_managed_string(void* managed_string) {
        auto mono_string = (MonoString*)managed_string;
        return mono_string_to_utf8(mono_string);
    }

    void* script_engine::to_reflection_type(void* _class) {
        auto mono_class = (MonoClass*)_class;
        auto type = mono_class_get_type(mono_class);
        return mono_type_get_object(script_engine_data->script_domain, type);
    }

    void* script_engine::from_reflection_type(void* reflection_type) {
        auto mono_object = (MonoReflectionType*)reflection_type;
        auto type = mono_reflection_type_get_type(mono_object);
        return mono_class_from_mono_type(type);
    }

    void* script_engine::to_reflection_property(void* property) {
        auto mono_property = (MonoProperty*)property;
        auto mono_class = mono_property_get_parent(mono_property);

        return mono_property_get_object(script_engine_data->script_domain, mono_class,
                                        mono_property);
    }

    void* script_engine::get_property_value(void* object, void* property, void** arguments) {
        auto mono_property = (MonoProperty*)property;

        MonoObject* exc = nullptr;
        void* value = mono_property_get_value(mono_property, object, arguments, &exc);
        handle_exception(exc);

        return value;
    }

    void script_engine::set_property_value(void* object, void* property, void** arguments) {
        auto mono_property = (MonoProperty*)property;

        MonoObject* exc = nullptr;
        mono_property_set_value(mono_property, object, arguments, &exc);
        handle_exception(exc);
    }

    void* script_engine::get_field_value(void* object, void* field) {
        auto mono_object = (MonoObject*)object;
        auto mono_field = (MonoClassField*)field;

        return mono_field_get_value_object(script_engine_data->script_domain, mono_field,
                                           mono_object);
    }

    void script_engine::set_field_value(void* object, void* field, void* value) {
        auto mono_object = (MonoObject*)object;
        auto mono_field = (MonoClassField*)field;

        mono_field_set_value(mono_object, mono_field, value);
    }
} // namespace sge

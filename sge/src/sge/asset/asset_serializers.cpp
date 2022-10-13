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
#include "sge/asset/asset_serializers.h"
#include "sge/renderer/shader.h"
#include "sge/renderer/texture.h"
#include "sge/scene/prefab.h"
#include "sge/asset/project.h"
#include "sge/asset/sound.h"
#include "sge/scene/shape.h"

namespace sge {
    class shader_serializer : public asset_serializer {
    protected:
        virtual bool serialize_impl(const fs::path& path, ref<asset> _asset) override {
            return true;
        }

        virtual bool deserialize_impl(const fs::path& path, ref<asset>& _asset) override {
            if (!fs::exists(path)) {
                return false;
            }

            _asset = shader::create(path);
            return true;
        }
    };

    class texture2d_serializer : public asset_serializer {
    protected:
        virtual bool serialize_impl(const fs::path& path, ref<asset> _asset) override {
            auto texture = _asset.as<texture_2d>();
            texture_2d::serialize_settings(texture, path);

            return true;
        }

        virtual bool deserialize_impl(const fs::path& path, ref<asset>& _asset) override {
            if (!fs::exists(path)) {
                return false;
            }

            auto texture = texture_2d::load(path);
            if (texture) {
                _asset = texture;
                return true;
            }

            return false;
        }
    };

    class prefab_serializer : public asset_serializer {
    protected:
        virtual bool serialize_impl(const fs::path& path, ref<asset> _asset) override {
            auto _prefab = _asset.as<prefab>();
            return prefab::serialize(_prefab, path);
        }

        virtual bool deserialize_impl(const fs::path& path, ref<asset>& _asset) override {
            _asset = ref<prefab>::create(path);
            return true;
        }
    };

    class sound_serializer : public asset_serializer {
    protected:
        virtual bool serialize_impl(const fs::path& path, ref<asset> _asset) override {
            return true;
        }

        virtual bool deserialize_impl(const fs::path& path, ref<asset>& _asset) override {
            _asset = ref<sound>::create(path);
            return true;
        }
    };

    class shape_serializer : public asset_serializer {
    protected:
        virtual bool serialize_impl(const fs::path& path, ref<asset> _asset) override {
            auto _shape = _asset.as<shape>();
            return shape::serialize(_shape, path);
        }

        virtual bool deserialize_impl(const fs::path& path, ref<asset>& _asset) override {
            _asset = ref<shape>::create(path);
            return true;
        }
    };

    static std::unordered_map<asset_type, std::unique_ptr<asset_serializer>> asset_serializers;
    template <typename T>
    static void add_serializer(asset_type type) {
        static_assert(std::is_base_of_v<asset_serializer, T>,
                      "the given type must derive from asset_serializer!");

        if (asset_serializers.find(type) == asset_serializers.end()) {
            auto instance = new T;
            auto unique_ptr = std::unique_ptr<asset_serializer>(instance);

            asset_serializers.insert(std::make_pair(type, std::move(unique_ptr)));
        }
    }

    void asset_serializer::init() {
        add_serializer<shader_serializer>(asset_type::shader);
        add_serializer<texture2d_serializer>(asset_type::texture_2d);
        add_serializer<prefab_serializer>(asset_type::prefab);
        add_serializer<sound_serializer>(asset_type::sound);
        add_serializer<shape_serializer>(asset_type::shape);
    }

    bool asset_serializer::serialize(ref<asset> _asset) {
        if (!_asset ||
            (asset_serializers.find(_asset->get_asset_type()) == asset_serializers.end())) {
            return false;
        }

        fs::path path = _asset->get_path();
        if (path.is_relative()) {
            fs::path asset_dir = project::get().get_asset_dir();
            path = asset_dir / path;
        }

        try {
            auto& serializer = asset_serializers[_asset->get_asset_type()];
            return serializer->serialize_impl(path, _asset);
        } catch (const std::exception& exc) {
            spdlog::warn("error serializing asset at {0}: {1}", path.string(), exc.what());
            return false;
        }
    }

    bool asset_serializer::deserialize(const asset_desc& desc, ref<asset>& _asset) {
        if (!desc.type.has_value() ||
            (asset_serializers.find(desc.type.value()) == asset_serializers.end())) {
            return false;
        }

        fs::path path = desc.path;
        if (path.is_relative()) {
            fs::path asset_dir = project::get().get_asset_dir();
            path = asset_dir / path;
        }

        bool succeeded = false;
        try {
            auto& serializer = asset_serializers[desc.type.value()];
            succeeded = serializer->deserialize_impl(path, _asset);
        } catch (const std::exception& exc) {
            spdlog::warn("error deserializing asset at {0}: {1}", path.string(), exc.what());
        }

        if (succeeded && desc.id.has_value()) {
            _asset->id = desc.id.value();
        }

        return succeeded;
    }
} // namespace sge
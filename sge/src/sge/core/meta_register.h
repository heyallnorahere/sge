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

#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>

// This parttern for automatic registration on load is borrowed from
// https://github.com/suVrik/hooker.galore/blob/6405e85d665347bdce6b0106c010443bc183f3be/sources/core/meta/registration.h

#define META_REGISTER_CAT_IMPL(a, b) a##b
#define META_REGISTER_CAT(a, b) META_REGISTER_CAT_IMPL(a, b)

#define META_REGISTER                                                                              \
    static void meta_auto_register_function_();                                                    \
    namespace {                                                                                    \
        struct meta_auto_register__ {                                                              \
            meta_auto_register__() { meta_auto_register_function_(); }                             \
        };                                                                                         \
    }                                                                                              \
    static const meta_auto_register__ META_REGISTER_CAT(auto_register__, __LINE__);                \
    static void meta_auto_register_function_()
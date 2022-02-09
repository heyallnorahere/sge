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
#include "sge/core/environment.h"
#ifdef SGE_PLATFORM_WINDOWS
#include "sge/platform/windows/windows_environment.h"
#endif

namespace sge {
    bool environment::set(const std::string& key, const std::string& value) {
#ifdef SGE_PLATFORM_WINDOWS
        return windows_setenv(key, value);
#else
        // todo: implement
        return false;
#endif
    }

    std::string environment::get(const std::string& key) {
        std::string value;

#ifdef SGE_PLATFORM_WINDOWS
        value = windows_getenv(key);
#else
        // todo: implement
#endif

        return value;
    }

    bool environment::has(const std::string& key) {
#ifdef SGE_PLATFORM_WINDOWS
        return windows_hasenv(key);
#else
        // todo: implement
        return false;
#endif
    }
} // namespace sge

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
#include "sge/core/guid.h"

#include <random>

namespace sge {
    static std::random_device random_device;
    static std::mt19937_64 random_engine(random_device());
    static std::uniform_int_distribution<uint64_t> uniform_distribution;

    void guid::regenerate() { m_guid = uniform_distribution(random_engine); }
} // namespace sge

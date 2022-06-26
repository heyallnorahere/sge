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
#include <volk.h>
#include <vk_mem_alloc.h>

namespace sge {
    inline std::string vk_result_name(VkResult result) {
        switch (result) {
            // todo(nora): vulkan error codes
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_SUCCESS:
            return "VK_SUCCESS";
        default:
            return "unknown/unimplemented";
        }
    }

    inline void check_vk_result(VkResult result) {
        if (result != VK_SUCCESS) {
            throw std::runtime_error("vulkan error thrown: " + vk_result_name(result));
        }
    }

    template <typename T>
    inline T vk_init() {
        T data;
        memset(&data, 0, sizeof(T));
        return data;
    }
    
    template <typename T>
    inline T vk_init(VkStructureType struct_type) {
        T data = vk_init<T>();
        data.sType = struct_type;
        return data;
    }
} // namespace sge
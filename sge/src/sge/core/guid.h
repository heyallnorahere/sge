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
    class guid {
    public:
        guid() { regenerate(); }

        guid(uint64_t id) { m_guid = id; }
        guid& operator=(uint64_t id) {
            m_guid = id;
            return *this;
        }

        guid(const guid& other) { m_guid = other.m_guid; }
        guid& operator=(const guid& other) {
            m_guid = other.m_guid;
            return *this;
        }

        void regenerate();

        operator uint64_t() const { return m_guid; }
        bool operator==(const guid& other) const { return m_guid == other.m_guid; }
        bool operator!=(const guid& other) const { return !(*this == other); }

    private:
        uint64_t m_guid;
    };
} // namespace sge

namespace std {
    template <>
    struct hash<sge::guid> {
        size_t operator()(const sge::guid& guid) const {
            hash<uint64_t> hasher;
            return hasher(guid);
        }
    };
} // namespace std

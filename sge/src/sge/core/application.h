/*
   Copyright 2021 Nora Beda

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
    class application : public ref_counted {
    public:
        static void set(ref<application> app);
        static ref<application> get();

        application(const std::string& title);

        void init();
        void shutdown();

    protected:
        virtual void load_content() = 0;
        virtual void unload_content() = 0;

    private:
        std::string m_title;
    };
}
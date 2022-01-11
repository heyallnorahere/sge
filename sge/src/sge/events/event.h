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
    enum class event_id {
        none = 0,
        window_close,
        window_resize,
        key_pressed,
        key_released,
        key_typed,
        mouse_moved,
        mouse_scrolled,
        mouse_button
    };

#define EVENT_ID_DECL(id)                                                                          \
    static event_id get_static_id() { return event_id::id; }                                       \
    virtual event_id get_id() override { return get_static_id(); }                                 \
    virtual std::string get_name() override { return #id; }

    class event {
    public:
        virtual ~event() = default;

        bool handled = false;

        virtual event_id get_id() = 0;
        virtual std::string get_name() = 0;
    };

    class event_dispatcher {
    public:
        event_dispatcher(event& e) : m_event(e) {}

        template <typename T, typename Func> bool dispatch(const Func& func) {
            if (m_event.get_id() == T::get_static_id()) {
                m_event.handled |= func((T&)m_event);
                return true;
            }
            return false;
        }

    private:
        event& m_event;
    };

#define SGE_BIND_EVENT_FUNC(func)                                                                  \
    [this](auto&&... args) -> decltype(auto) {                                                     \
        return func(std::forward<decltype(args)>(args)...);                                  \
    }
} // namespace sge
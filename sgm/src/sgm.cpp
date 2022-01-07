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

#include "sgmpch.h"
#include "sgm.h"
#include "editor_layer.h"
#include "panels/panels.h"
#include "editor_scene.h"
namespace sgm {
    void sgm_app::on_init() {
        editor_scene::create();

        m_editor_layer = new editor_layer;
        push_layer(m_editor_layer);

        m_editor_layer->add_panel<renderer_info_panel>();
        m_editor_layer->add_panel<viewport_panel>();
        m_editor_layer->add_panel<scene_hierarchy_panel>();
    }

    void sgm_app::on_shutdown() {
        pop_layer(m_editor_layer);
        delete m_editor_layer;

        editor_scene::destroy();
    }
} // namespace sgm
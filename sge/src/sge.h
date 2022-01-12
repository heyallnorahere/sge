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
// this header is NOT to be included by the core sge library.

// precompiled header
#include "sgepch.h"

// core
#include "sge/core/application.h"
#include "sge/core/window.h"
#include "sge/core/input.h"

// events
#include "sge/events/event.h"
#include "sge/events/window_events.h"
#include "sge/events/input_events.h"

// imgui extensions
#include "sge/imgui/imgui_extensions.h"

// scene
#include "sge/scene/scene.h"
#include "sge/scene/scene_serializer.h"
#include "sge/scene/components.h"
#include "sge/scene/entity.h"
#include "sge/scene/editor_camera.h"

// script engine
#include "sge/script/script_engine.h"
#include "sge/script/garbage_collector.h"

// main
#ifdef SGE_INCLUDE_MAIN
#include "sge/core/main.h"
#endif
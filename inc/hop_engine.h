/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

// provides the entire header directory as a single include. if you definitely only need particular
// files, then skip including this file as it will increase compile time and dependency.

#include <glm/glm.hpp>

#include "basic_components.h"
#include "buffer.h"
#include "common.h"
#include "debug.h"
#include "deserialise.h"
#include "engine.h"
#include "events.h"
#include "input.h"
#include "material.h"
#include "mesh.h"
#include "node_view.h"
#include "package.h"
#include "render_graph.h"
#include "graphics_server.h"
#include "scene.h"
#include "framebuffer.h"
#include "texture.h"
#include "user_interface.h"
#include "random.h"
#include "window.h"
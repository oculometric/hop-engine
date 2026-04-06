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
#include "render_server.h"
#include "scene.h"
#include "swapchain.h"
#include "texture.h"
#include "user_interface.h"

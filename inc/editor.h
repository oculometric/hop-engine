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

#include "hop_engine.h"

namespace HopEngine
{

class Editor : public Application
{
private:
    Ref<Scene> view_3d;
    Ref<Scene> view_nodes;
    WeakRef<NodeView> node_view;
    WeakRef<StaticMeshComponent> cube;
    Ref<UICanvas> main_canvas;
    WeakRef<Material> ssao_material;
    int samples = 4;
    float radius = 4.0f;
    float power = 1.4f;
    bool use_smoothstep = false;
    float strength = 1.0f;

public:
    void awake() override;
    void update(float delta_time) override;
    void drawImGui() override;
};

}
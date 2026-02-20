#pragma once

#include <vector>

namespace HopEngine
{

struct FrameStats
{
    float imgui_time = 0.0f;
    float build_time = 0.0f;
    float record_time = 0.0f;
    float render_time = 0.0f;
    std::vector<float> pass_times;
    float update_time = 0.0f;
    float delta_time = 0.0f;
    size_t draw_calls = 0;
    size_t pipeline_rebinds = 0;
    size_t triangles = 0;
    size_t vertices = 0;
    size_t passes = 0;
    size_t cameras = 0;
    size_t lights = 0;
};

}
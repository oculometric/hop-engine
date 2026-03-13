#pragma once

#include <vector>

namespace HopEngine
{

struct FrameStats final
{
    float record_time = 0.0f;
    float render_time = 0.0f;
    float update_time = 0.0f;
    std::vector<float> pass_times;
    size_t draw_calls = 0;
    size_t pipeline_rebinds = 0;
    size_t triangles = 0;
    size_t passes = 0;
    size_t cameras = 0;
};

}
#if !defined(STANDALONE)

#include "common.h"
#include "engine.h"

struct SceneFuncSet
{
    std::wstring name;
    HopEngine::Ref<HopEngine::Scene>(*init_func)();
    HopEngine::UpdateFunc update_func;
    HopEngine::ImGuiDrawFunc imgui_func;
};

SceneFuncSet getAshaScene();
SceneFuncSet getNodeScene();
SceneFuncSet getMuseumScene();

#endif
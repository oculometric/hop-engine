#if !defined(STANDALONE)

#include "common.h"

struct SceneFuncSet
{
    std::wstring name;
    HopEngine::Ref<HopEngine::Scene>(*init_func)();
    void(*update_func)(HopEngine::Ref<HopEngine::Scene>, float);
    void(*imgui_func)(HopEngine::Ref<HopEngine::Scene>, float);
};

SceneFuncSet getAshaScene();
SceneFuncSet getNodeScene();
SceneFuncSet getMuseumScene();

#endif
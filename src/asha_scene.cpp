#include <random>
#include <imgui.h>

#include "hop_engine.h"
#include "main.h"

using namespace HopEngine;

static WeakRef<StaticMesh> asha;
static WeakRef<Material> cc_material;
static WeakRef<Gizmo> gizmo;

static Ref<Scene> initAshaScene()
{
    Ref<Scene> scene = new Scene();
    Ref<Shader> shader = Engine::loadShader("res://psx");
    Ref<Sampler> sampler = new Sampler(SamplerBuilder().filter(VK_FILTER_NEAREST));
    asha = scene->insertObject<StaticMesh>(new StaticMesh(
        Engine::loadMesh("res://samples/asha.obj"),
        Engine::keepLoaded(new Material(
            shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE).stencilWrite(1)
        ))));
    asha->material->setTexture("albedo", Engine::loadTexture("res://samples/asha.png"));
    asha->material->setSampler("albedo", sampler);
    asha->transform.setLocalPosition({ 0, 0, -0.9f });

    Ref<StaticMesh> bunny = scene->insertObject<StaticMesh>(new StaticMesh(
        Engine::loadMesh("res://samples/bunny.obj"),
        Engine::keepLoaded(new Material(shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE).stencilWrite(2)
        ))));
    bunny->material->setTexture("albedo", Engine::loadTexture("res://samples/bunny.png"));
    bunny->material->setSampler("albedo", sampler);
    bunny->setParent(asha);
    bunny->transform.setLocalPosition({ 0, -0.5f, 0.9f });
    bunny->transform.scaleLocal({ 2, 2, 2 });

    Ref<StaticMesh> tux = scene->insertObject<StaticMesh>(new StaticMesh(
        Engine::loadMesh("res://tux.obj"),
        Engine::keepLoaded(new Material(shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE)))
    ));
    tux->material->setTexture("albedo", Engine::loadTexture("res://tux.png"));
    tux->material->setSampler("albedo", sampler);
    tux->transform.translateLocal({ 2, 0, 0 });

    auto sun_lamp = scene->insertObject<Light>(new Light(Light::DIRECTIONAL));
    sun_lamp->transform.rotateLocal({ -17.0f, -34.0f, -189.0f });
    sun_lamp->colour = { 1.0f, 1.0f, 1.0f };

    scene->getCamera(0)->transform.lookAt(glm::vec3(0.5f, -1.5f, 0.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));

    gizmo = scene->insertObject<Gizmo>(new Gizmo());
    scene->skybox = Engine::loadTexture("res://samples/nasa_goddard_gaia_dr2_deep_star_map.png");

    Ref<Camera> second_cam = new Camera();
    scene->insertObject(second_cam);
    scene->setCameraSlot(second_cam, 1);
    second_cam->transform.lookAt({ 0, -2.0f, 0.7f }, { 0, 0, 0.7f }, { 0, 0, 1 });
    second_cam->fov = 20.0f;

    Ref<Camera> third_cam = new Camera();
    scene->insertObject(third_cam);
    scene->setCameraSlot(third_cam, 2);
    third_cam->transform.lookAt({ 0, -0.2f, 0.8f }, { 0, 0, 0.7f }, { 0, 0, 1 });
    third_cam->fov = 120.0f;

    scene->render_graph = RenderGraph::deserialise("res://test_graph.hrgr");

    glm::vec4 samples[24];
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::default_random_engine rand;
    for (int i = 0; i < 24; ++i)
    {
        glm::vec4 s = glm::vec4((dist(rand) * 2.0f) - 1.0f, (dist(rand) * 2.0f) - 1.0f, -dist(rand), 0.0f);
        const float fi = static_cast<float>(i) / 24.0f;
        glm::vec4 v = glm::normalize(s * (0.05f + ((1.0f - 0.05f) * fi * fi)));
        samples[i] = v;
    }
    scene->render_graph->getMaterialForStep(3)->setUniform("samples", samples, sizeof(glm::vec4) * 16);
    cc_material = scene->render_graph->getMaterialForStep(5);
    cc_material->setFloatUniform("gamma", 1.0f);
    cc_material->setFloatUniform("exposure", 1.0f);
    cc_material->setFloatUniform("offset", 0.0f);
    
    Engine::debugClearSelection(asha.cast<Object>(), asha->material, scene->getCamera(0));
    return scene;
}

static void updateAshaScene(Ref<Scene> scene, float delta_time)
{
    static float total_time = 0;
    total_time += delta_time;

    Engine::debugCamera(delta_time);
    
    gizmo->trackObject(Engine::getDebugSelection(), scene->getCamera(0));
    
    Input::resetMouseDelta();
}

static void imGuiAshaScene(Ref<Scene> scene, float delta_time)
{
    ImGui::Begin("colour correction", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("this controls the final post processing step");
    static float gamma = 0;
    static float new_gamma = 1.0f;
    static float exposure = 0;
    static float new_exposure = 1.0f;
    static float offset = 0;
    static float new_offset = 0.0f;
    ImGui::SliderFloat("gamma", &new_gamma, 0.001f, 4.0f);
    ImGui::SliderFloat("exposure", &new_exposure, 0.001f, 16.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("offset", &new_offset, -2.0f, 2.0f);
    if (gamma != new_gamma)
        cc_material->setFloatUniform("gamma", new_gamma);
    if (exposure != new_exposure)
        cc_material->setFloatUniform("exposure", new_exposure);
    if (offset != new_offset)
        cc_material->setFloatUniform("offset", new_offset);
    gamma = new_gamma;
    exposure = new_exposure;
    offset = new_offset;
    ImGui::End();
}

SceneFuncSet getAshaScene()
{
    return { L"bunnygirl", initAshaScene, updateAshaScene, imGuiAshaScene };
}
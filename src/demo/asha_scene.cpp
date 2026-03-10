#if !defined(STANDALONE)

#include <random>
#include <imgui/imgui.h>

#include "hop_engine.h"
#include "../main.h"

using namespace HopEngine;

static WeakRef<StaticMesh> asha;
static WeakRef<Material> cc_material;
static WeakRef<Gizmo> gizmo;

static Ref<Scene> initAshaScene()
{
    Ref<Scene> scene = Scene::create();
    // load and configure asha's shader, mesh, and material
    const Ref<Shader> shader = Engine::loadShader("res://engine/samples/psx.glsl");
    asha = scene->insertObject<StaticMesh>(StaticMesh::create(
        Engine::loadMesh("res://engine/samples/asha.obj"),
        new Material(shader, PipelineBuilder().cullMode(CULL_NONE).stencilWrite(1))
    ));
    asha->material->setTexture("albedo", Engine::loadTexture("res://engine/samples/asha.png"));
    const Ref<Sampler> sampler = Engine::makeSampler(SamplerBuilder().filter(FILTER_NEAREST));
    asha->material->setSampler("albedo", sampler);
    asha->transform.setLocalPosition({ 0, 0, -0.9f });

    // create the bunny
    Ref<StaticMesh> bunny = scene->insertObject<StaticMesh>(StaticMesh::create(
        Engine::loadMesh("res://engine/samples/bunny.obj"),
        new Material(shader, PipelineBuilder().cullMode(CULL_NONE).stencilWrite(2))
    ));
    bunny->material->setTexture("albedo", Engine::loadTexture("res://engine/samples/bunny.png"));
    bunny->material->setSampler("albedo", sampler);
    asha->addChild(bunny);
    bunny->transform.setLocalPosition({ 0, -0.5f, 0.9f });
    bunny->transform.scaleLocal({ 2, 2, 2 });

    // create tux
    Ref<StaticMesh> tux = scene->insertObject<StaticMesh>(StaticMesh::create(
        Engine::loadMesh("res://tux.obj"),
        new Material(shader, PipelineBuilder().cullMode(CULL_NONE))
    ));
    tux->material->setTexture("albedo", Engine::loadTexture("res://tux.png"));
    tux->material->setSampler("albedo", sampler);
    tux->transform.translateLocal({ 2, 0, 0 });

    // initialise sun and main camera
    auto sun_lamp = scene->insertObject<Light>(Light::create(Light::DIRECTIONAL));
    sun_lamp->transform.setLocalEuler({ -17.0f, -34.0f, -189.0f });
    sun_lamp->colour = { 1.0f, 1.0f, 1.0f };

    scene->getCamera(0)->transform.lookAt(glm::vec3(0.5f, -1.5f, 0.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));

    // demo gizmo
    gizmo = scene->insertObject<Gizmo>(Gizmo::create());
    // set skybox texture
    scene->skybox = Engine::loadTexture("res://engine/samples/nasa_goddard_gaia_dr2_deep_star_map.png");

    // initialise the other two cameras. these will render into different slots in the render graph
    Ref<Camera> second_cam = Camera::create();
    scene->insertObject(second_cam);
    scene->setCameraSlot(second_cam, 1);
    second_cam->transform.lookAt({ 0, -2.0f, 0.7f }, { 0, 0, 0.7f }, { 0, 0, 1 });
    second_cam->fov = 20.0f;

    Ref<Camera> third_cam = Camera::create();
    scene->insertObject(third_cam);
    scene->setCameraSlot(third_cam, 2);
    third_cam->transform.lookAt({ 0, -0.2f, 0.8f }, { 0, 0, 0.7f }, { 0, 0, 1 });
    third_cam->fov = 120.0f;

    // oh yeah, load the render graph
    scene->render_graph = RenderGraph::deserialise("res://test_graph.hrgr");

    // configure the samples for the SSAO shader. TODO: this should probably be baked into the shader as a constant
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
    // configure the colour correction material to make no changes
    cc_material = scene->render_graph->getMaterialForStep(5);
    cc_material->setFloatUniform("gamma", 1.0f);
    cc_material->setFloatUniform("exposure", 1.0f);
    cc_material->setFloatUniform("offset", 0.0f);
    
    Engine::setScene(scene);
    Engine::debugClearSelection(asha.cast<Object>(), asha->material, scene->getCamera(0));
    RenderServer::setTitle("Demo Scene - Asha");
    
    return scene;
}

static void updateAshaScene(float delta_time)
{
    // tick the debug camera
    Engine::debugCamera();
    
    // let the gizmo do things
    gizmo->trackObject(Engine::getDebugSelection(), Engine::getScene()->getCamera(0));
}

static void imGuiAshaScene()
{
    {
        // draw the colour correction controls
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
    
    Engine::drawImGuiDebug();
}

SceneFuncSet getAshaScene()
{
    return { L"bunnygirl", initAshaScene, updateAshaScene, imGuiAshaScene };
}

#endif
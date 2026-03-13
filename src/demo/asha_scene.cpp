#if !defined(STANDALONE)

#include <random>
#include <imgui/imgui.h>

#include "hop_engine.h"
#include "../main.h"

using namespace HopEngine;

AshaApp::AshaApp()
{
    Ref<Scene> scene = Scene::create();

    // load and configure asha's shader, mesh, and material
    const Ref<Shader> shader = Engine::loadShader("res://engine/samples/psx.glsl");
    asha = scene->addObject("asha");
    auto sm_comp = asha->addComponent<StaticMeshComponent>();
    sm_comp->mesh =  Engine::loadMesh("res://engine/samples/asha.obj");
    sm_comp->material = new Material(shader, Pipeline::Builder().cullMode(Pipeline::CULL_NONE).stencilWrite(1));
    sm_comp->material->setTexture("albedo", Engine::loadTexture("res://engine/samples/asha.png"));
    const Ref<Sampler> sampler = Engine::makeSampler(Sampler::Builder().filter(Sampler::FILTER_NEAREST));
    sm_comp->material->setSampler("albedo", sampler);
    asha->transform.setLocalPosition({ 0, 0, -0.9f });

    // add some text
    auto text = scene->addObject("text");
    auto text_comp = text->addComponent<TextComponent>();
    text_comp->setText("bunny party!!");
    text_comp->setTint({ 0, 1, 0 });
    text->transform.setLocalPosition({ -1.2f, 0.5f, 1.4f });
    text->transform.setLocalEuler({ 90, 0, 0 });
    text_comp->camera_mask = 0b110;

    // create the bunny
    auto bunny = scene->addObject("bunny");
    sm_comp = bunny->addComponent<StaticMeshComponent>();
    sm_comp->mesh = Engine::loadMesh("res://engine/samples/bunny.obj");
    sm_comp->material = new Material(shader, Pipeline::Builder().cullMode(Pipeline::CULL_NONE).stencilWrite(2));
    sm_comp->material->setTexture("albedo", Engine::loadTexture("res://engine/samples/bunny.png"));
    sm_comp->material->setSampler("albedo", sampler);
    asha->addChild(bunny);
    bunny->transform.setLocalPosition({ 0, -0.5f, 0.9f });
    bunny->transform.scaleLocal({ 2, 2, 2 });

    // initialise sun
    auto sun_lamp = scene->addObject("sun");
    auto light_comp = sun_lamp->addComponent<LightComponent>();
    light_comp->type = LightComponent::DIRECTIONAL;
    light_comp->colour = { 1.0f, 1.0f, 1.0f };
    sun_lamp->transform.setLocalEuler({ -17.0f, -34.0f, -189.0f });

    // init main camera
    auto camera = scene->addObject("camera");
    camera->addComponent<CameraComponent>();
    camera->transform.lookAt(glm::vec3(0.5f, -1.5f, 0.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));

    // initialise the other two cameras. these will render into different slots in the render graph
    auto second_cam = scene->addObject("camera2");
    auto cam_comp = second_cam->addComponent<CameraComponent>();
    cam_comp->camera_slot = 1;
    cam_comp->fov = 20.0f;
    second_cam->transform.lookAt({ 0, -2.0f, 0.7f }, { 0, 0, 0.7f }, { 0, 0, 1 });

    auto third_cam = scene->addObject("camera3");
    cam_comp = third_cam->addComponent<CameraComponent>();
    cam_comp->camera_slot = 2;
    cam_comp->fov = 120.0f;
    third_cam->transform.lookAt({ 0, -0.2f, 0.8f }, { 0, 0, 0.7f }, { 0, 0, 1 });

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
    
    // Engine::debugClearSelection(asha.cast<Object>(), asha->material, scene->getCamera(0));

    // set skybox texture
    scene->setSkybox(Engine::loadTexture("res://engine/samples/nasa_goddard_gaia_dr2_deep_star_map.png"));

    Engine::setScene(scene);
    RenderServer::setTitle("Demo Scene - Asha");
}

void AshaApp::update(float delta_time)
{
    Engine::debugCamera(Engine::getScene()->findObject("camera"));
}

void AshaApp::drawImGui()
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
    
    Engine::drawImGuiDebug();
}

#endif
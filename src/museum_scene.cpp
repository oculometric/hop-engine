#if !defined(STANDALONE)

#include <random>
#include <imgui.h>

#include "hop_engine.h"
#include "main.h"

using namespace HopEngine;

static WeakRef<Material> cc_material;

static WeakRef<StaticMesh> crt;

static WeakRef<StaticMesh> obj;
static WeakRef<StaticMesh> obj2;
static WeakRef<StaticMesh> obj3;
static WeakRef<StaticMesh> obj4;

static WeakRef<StaticMesh> spline_obj;
static Spline spline;
static float spline_progress = 0.0f;
static bool spline_tracked = false;

static bool camera_flythrough = false;
static float camera_flythrough_time = 0.0f;
static Spline flythrough_spline
{
    {
        { 12.0f, -9.0f, 1.73f },
        { 12.0f, -8.8f, 1.78f },
        { 12.0f, -8.5f, 1.83f },
        { 13.0f, -6.75f, 1.68f },
        { 14.1f, -4.5f, 1.41f },
        { 13.9f, -1.9f, 1.8f },
        { 12.2f, -1.1f, 1.8f },
        { 10.0f, -3.3f, 2.0f },
        { 9.3f, -4.0f, 1.9f },
        { 6.6f, -3.9f, 1.9f },
        { 3.1f, -3.9f, 1.9f },
        { 0.4f, -4.8f, 1.45f },
        { -0.3f, -3.8f, 1.77f },
        { -0.0f, 1.6f, 6.5f },
        { 1.9f, 6.0f, 9.1f },
        { 10.0f, 3.2f, 8.8f },
        { 11.8f, -1.8f, 3.8f },
        { 11.4f, -1.8f, 3.8f },
    },
    false
};


Ref<Scene> initMuseumScene()
{
    Debug::setLogLevel(Debug::DEBUG_WARNING);
    Ref<Scene> scene = Scene::deserialise("res://museum/Museum.hscn");
    if (!scene) return scene;
    
    obj = scene->findObject<StaticMesh>("orrery_core");
    obj2 = scene->findObject<StaticMesh>("orrery_mid");
    obj3 = scene->findObject<StaticMesh>("orrery_orbit_a");
    obj4 = scene->findObject<StaticMesh>("orrery_orbit_b");
    
    spline_obj = scene->insertObject<StaticMesh>(
        new StaticMesh(
            Engine::loadMesh("res://museum/IcePlanet.obj"), 
            Engine::loadMaterial("res://museum/IcePlanet.hmat")));
    spline_obj->transform.setLocalScale({ 0.3f, 0.3f, 0.3f });
    spline.loop = true;
    spline.points = {
        { 10.5, -9.5, 1 },
        { 13, -10.5, 2.5 },
        { 8.5, -8.5, 3.5 },
        { 11, -7.5, 4.5 },
        { 14, -8, 1.5 }
    };
    
    crt = scene->findObject<StaticMesh>("crt_screen");
    
    auto fog_mat = scene->render_graph->getMaterialForStep("fog");
    fog_mat->setFloatUniform("fog_start", 4.0f);
    fog_mat->setFloatUniform("fog_end", 24.0f);
    fog_mat->setFloatUniform("fog_exponent", 0.5f);
    fog_mat->setVec4Uniform("fog_colour", { 0.005, 0.006, 0.002, 0 });
    
    constexpr int SAMPLES_COUNT = 24;
    glm::vec4 samples[SAMPLES_COUNT];
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::default_random_engine rand;
    for (int i = 0; i < SAMPLES_COUNT; ++i)
    {
        glm::vec3 s = glm::vec3(
            (dist(rand) * 2.0f) - 1.0f,
            (dist(rand) * 2.0f) - 1.0f,
            dist(rand));
        s = glm::normalize(s);
        const float fi = static_cast<float>(i) / SAMPLES_COUNT;
        glm::vec3 v = s * glm::mix(0.1f, 1.0f, fi * fi);
        samples[i] = glm::vec4(v, 0.0f);
    }
    scene->render_graph->getMaterialForStep("ssao")->setUniform("samples", samples, sizeof(glm::vec4) * SAMPLES_COUNT);
    scene->render_graph->setSkipStep("blurred2", true);
    
    cc_material = scene->render_graph->getMaterialForStep("colour_grading");
    cc_material->setFloatUniform("gamma", 1.0f);
    cc_material->setFloatUniform("exposure", 1.0f);
    cc_material->setFloatUniform("offset", 0.0f);
    cc_material->setTexture("lut", Engine::loadTexture3D("res://museum/lut.png", 8, 8));
    cc_material->setSampler("lut", new Sampler(SamplerBuilder().address(ADDRESS_CLAMP_EDGE)));
    cc_material->setFloatUniform("use_lut", 1);
    Engine::debugClearSelection(WeakRef<Object>(), WeakRef<Material>(), scene->getCamera(0));

    return scene;
}


static void updateMuseumScene(Ref<Scene> scene, float delta_time)
{
    static float total_time = 0;
    total_time += delta_time;

    Engine::debugCamera(delta_time);

    if (Input::isMouseDown(Input::MOUSE_LEFT))
    {
        glm::vec2 mouse_clip = ((Input::getMousePosition() / RenderServer::getFramebufferSize()) * 2.0f) - 1.0f;
        glm::mat4 screen_to_world = glm::inverse(scene->getCamera(0)->getWorldToScreenMatrix());
        glm::vec4 mouse_transformed = screen_to_world * glm::vec4(mouse_clip, 1.0f, 1.0f);
        mouse_transformed /= mouse_transformed.w;
        glm::vec3 mouse_world = glm::normalize(glm::xyz(mouse_transformed) - scene->getCamera(0)->transform.getPosition());
        Engine::debugSelect(scene->raycast(scene->getCamera(0)->transform.getPosition(), mouse_world));
    }
    
    obj->transform.rotateLocal({ 0, 0, 20 * delta_time });
    obj2->transform.rotateLocal({ 35 * delta_time, 0, 0 });
    obj3->transform.rotateLocal({ 0, 0, 10 * delta_time });
    obj4->transform.rotateLocal({ 0, 0, -16 * delta_time });
    
    spline_progress += delta_time * 0.1f;
    spline_obj->transform.setLocalPosition(spline[spline_progress]);
    spline_obj->transform.lookAt(spline[spline_progress], spline[spline_progress - 0.01f], { 0, 0, 1 });
    if (spline_tracked)
    {
        WeakRef<Camera> camera = scene->getCamera(0);
        camera->transform.lookAt(camera->transform.getLocalPosition(), spline[spline_progress], { 0, 0, 1 });
    }
    
    crt->material->setTexture("albedo_tex", scene->render_graph->getFinalImage().first);
    
    Input::resetMouseDelta();
}


static void imGuiMuseumScene(Ref<Scene> scene, float delta_time)
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
    
    ImGui::Begin("spline control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Checkbox("camera track planet spline", &spline_tracked);
    if (ImGui::Checkbox("camera flythrough", &camera_flythrough))
        camera_flythrough_time = 0.0f;
    if (camera_flythrough)
    {
        WeakRef<Camera> camera = scene->getCamera(0);
        if (camera_flythrough_time >= 0.999f)
        {
            camera_flythrough = false;
            camera_flythrough_time = 0.0f;
        }
        camera->transform.lookAt(flythrough_spline[camera_flythrough_time], flythrough_spline[camera_flythrough_time + 0.01f], {0, 0, 1});
        camera_flythrough_time += delta_time * 0.035f;
    }
    ImGui::End();
}

SceneFuncSet getMuseumScene()
{
    return { L"museum", initMuseumScene, updateMuseumScene, imGuiMuseumScene };
}

#endif
#include <hop_engine.h>

using namespace HopEngine;

class MyGame : public Application
{
private:
    WeakRef<CameraComponent> camera;

public:
    void awake() override
    {
        // load the package containing our resources
        Package::importDeferredPackage("resources.hop");

        // update the window title and icon
        RenderServer::setTitle("David's Crate");
        RenderServer::setIcon("res://david_crate.jpg");

        // create a scene and make it current
        auto scene = Scene::create("scene");
        Engine::setScene(scene);

        // add a camera
        camera = scene->addObject<CameraComponent>("camera");
        camera->getTransform().lookAt(glm::vec3{ 2, 2, 2 }, glm::vec3{ 0, 0, 0 }, glm::vec3{ 0, 0, 1 });

        // add a crate, using the builtin cube and a custom material
        auto mesh_obj      = scene->addObject<StaticMeshComponent>("mesh");
        mesh_obj->material = Engine::loadMaterial("res://material.hmat");
        mesh_obj->mesh     = Engine::loadMesh("res://engine/meshes/cube.obj");
    }

    void update(float delta_time) override { Engine::debugCamera(camera->getOwner()); }
};

int main()
{
    HopEngine::init(Engine::InitParams{ false, Debug::DEBUG_INFO, false });
    Engine::startApplication<MyGame>();
    HopEngine::destroy();

    return 0;
}
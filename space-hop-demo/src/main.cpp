#include <hop_engine.h>

using namespace HopEngine;

class MyGame : public Application
{
private:
    Ref<StaticMeshComponent> mesh_obj;
    float remaining_time = 8.0f;
    glm::vec3 angular_speed = {4.0f, -2.6f, 52.5f};
    Ref<Material> sky_material;
    Ref<UILabel> title_label;
    Ref<UILabel> credit_label;
    std::string title_text_target;
    size_t title_text_offset;
    std::string credit_text_target;
    size_t credit_text_offset;
    float text_typing_cooldown = 0.0f;
    Ref<UICanvas> canvas;
    Ref<UIIcon> me_img;
    bool img_visible = false;
    float img_visible_timer = 0.0f;

    std::vector<std::tuple<std::string, std::string, std::string>> images = 
    {
        { "res://nasa/a.jpg", "Webb Studies Star Clusters",                                     "ESA/Webb, NASA & CSA, A. Pedrini,\nA. Adamo (Stockholm University) and the FEAST JWST team" },
        { "res://nasa/b.jpg", "Hubble Spots a Starry Spiral",                                   "ESA/Hubble & NASA, D. Thilker and the PHANGS-HST Team" },
        { "res://nasa/c.jpg", "Starstruck",                                                     "NASA" },
        { "res://nasa/d.jpg", "Webb Spots Details in Nearby Spiral Galaxy",                     "ESA/Webb, NASA & CSA, A. Leroy" },
        { "res://nasa/e.jpg", "Hubble Spots Lens-Shaped Galaxy",                                "ESA/Hubble & NASA, R. J. Foley (UC Santa Cruz),\nDark Energy Survey/DOE/FNAL/DECam/CTIO/NOIRLab/NSF/AURA; Acknowledgment: Mehmet Yüksek" },
        { "res://nasa/f.png", "Goldstone's DSS-15 Antenna and the Milky Way",                   "NASA/JPL-Caltech" },
        { "res://nasa/g.jpg", "A Galactic Embrace",                                             "X-ray: NASA/CXC/SAO; Infrared: NASA/ESA/CSA/STScI/Webb;\nImage Processing: NASA/CXC/SAO/L. Frattare" },
        { "res://nasa/h.jpg", "Hubble Glimpses Galactic Gas Making a Getaway",                  "ESA/Hubble & NASA, S. Veilleux, J. Wang, J. Greene" },
        { "res://nasa/i.jpg", "A Dance of Galaxies",                                            "ESA/Webb, NASA & CSA, A. Adamo (Stockholm University),\nG. Bortolini, and the FEAST JWST team" },
        { "res://nasa/j.jpg", "Massive Stars Make Their Mark in Hubble Image",                  "ESA/Hubble & NASA, F. Annibali, S. Hong" },
        { "res://nasa/k.jpg", "XRISM Finds Chlorine, Potassium in Cas A",                       "X-ray: NASA/CXC/SAO; Optical: NASA/ESA/STScI; IR: NASA/ESA/CSA/STScI/Milisavljevic et al.,\nNASA/JPL/CalTech; Image Processing: NASA/CXC/SAO/J. Schmidt and K. Arcand" },
        { "res://nasa/l.jpg", "NASA X-Ray Mission Gets Fresh Look at 2,000-Year-Old Supernova", "X-ray: Chandra: NASA/CXC/SAO, XMM: ESA/XMM-NEWTON, IXPE:NASA/MSFC;\nOptical: NSF/NOIRLab; Image Processing: NASA/CXC/SAO/J. Schmidt" },
    };

public:
    void awake() override
    {
        // load the package containing our resources
        Package::importDeferredPackage("resources.hop");

        // update the window title and icon
        RenderServer::setTitle("HopEngine Space Demo");
        RenderServer::setBorderless(true);

        auto scene = Scene::create("scene");
        sky_material = new Material(Engine::loadShader("res://stars.glsl"), Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthWrite(false).depthTest(false));
        sky_material->setTexture("tex", Engine::loadTexture("res://nasa/d.jpg"));
        for (const auto& i : images)
        {
            Engine::loadTexture(std::get<0>(i));
        }
        scene->sky = new Sky(sky_material, true);

        auto camera = scene->addObject<CameraComponent>("camera");
        camera->fov = 34.6f;
        camera->getTransform().setLocalEuler({ 90, 0, 0 });
        camera->getTransform().setPosition({ 0, -0.521947f, 0.210511f });

        mesh_obj = scene->addObject<StaticMeshComponent>("mesh");
        mesh_obj->material = Engine::loadMaterial("res://ComputerMonitor.hmat");
        mesh_obj->mesh = Engine::loadMesh("res://ComputerMonitor.obj");

        scene->render_graph = new RenderGraph(RenderGraph::Builder()
            .addCameraStep("camera", 0)
            .addPostprocessStep("stylised", Engine::loadShader("res://postprocess.glsl"))
            .bindTexture("camera", RenderGraph::TextureInput(0, 0))
            .clearColour({ 0, 0, 0 }, true)
            .setResolution(0.5f)
            .filtering(Sampler::FILTER_NEAREST)
        );

        auto world_canvas = scene->addObject<UICanvasComponent>("canvas");
        world_canvas->getCanvas()->setWorldSpace(false);
        world_canvas->getCanvas()->resize({ 1024, 768 });

        canvas = world_canvas->getCanvas();
        EventServer::subscribe(RenderServer::EVENT_TYPE_RESIZE, [](void* data, size_t size, void* inst) -> void {
            reinterpret_cast<MyGame*>(inst)->canvas->resize(RenderServer::getFramebufferSize());
        }, this);

        auto offsetter = canvas->addElement<UICanvasElement>();
        title_label = canvas->addChild<UILabel>(offsetter).strong();
        title_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT, UIRenderer::TEXT_FLAGS_UNDERLINE });
        title_label->setColour({ 1.0f, 0.6f, 0.0f });
        title_label->setExternalAnchor(UITransform::ANCHOR_BOTTOM_RIGHT);
        title_label->setInternalAnchor(UITransform::ANCHOR_BOTTOM_RIGHT);
        title_label->setPosition({ 0, -58.0f });
        credit_label = canvas->addChild<UILabel>(offsetter).strong();
        credit_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT, UIRenderer::TEXT_FLAGS_ITALIC });
        credit_label->setColour({ 0.8f, 0.8f, 0.8f });
        credit_label->setExternalAnchor(UITransform::ANCHOR_BOTTOM_RIGHT);
        credit_label->setInternalAnchor(UITransform::ANCHOR_BOTTOM_RIGHT);
        credit_label->setPosition({ 0, -36.0f });
        offsetter->setPosition({ -16.0f, -16.0f });
        offsetter->setExternalAnchor(UITransform::ANCHOR_BOTTOM_RIGHT);
        offsetter->setInternalAnchor(UITransform::ANCHOR_BOTTOM_RIGHT);
        auto me_label = canvas->addElement<UILabel>().strong();
        me_label->setColour({ 1.0f, 0.0f, 0.0f });
        me_label->setPosition({ 16.0f, 16.0f });
        me_label->setText("created by Cassette Costen");
        me_img = canvas->addElement<UIIcon>().strong();
        me_img->setIcon(7, true);
        me_img->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        me_img->setInternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        me_img->setPosition({ -16.0f, 8.0f });
        me_img->setSize(glm::vec2(72.0f));

        auto overlay = scene->addObject<StaticMeshComponent>("overlay");
        overlay->mesh = RenderServer::getQuad().strong();
        overlay->material = Engine::loadMaterial("res://overlay.hmat");
        overlay->getTransform().setLocalEuler({ 90, 0, 0 });
        overlay->getTransform().setLocalScale({ 1.3333f, 1.0f, 1.0f });
        overlay->getTransform().setLocalPosition({ 0.0f, 3.0f, camera->getTransform().getLocalPosition().z });

        reset();

        Engine::setScene(scene);
        RenderServer::setSize({ 1024, 768 });
    }

    float random(float min, float max)
    {
        float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        return (r * (max - min)) + min;
    }

    void update(float delta_time) override
    {
        mesh_obj->getTransform().translate({ 0, delta_time * 0.7f, 0 });
        mesh_obj->getTransform().rotate(angular_speed * delta_time);
        remaining_time -= delta_time;
        text_typing_cooldown -= delta_time;
        img_visible_timer -= delta_time;
        if (img_visible_timer <= 0.0f)
        {
            img_visible = !img_visible;
            me_img->setPosition({ img_visible ? -16.0f : 10000.0f, 8.0f });
            img_visible_timer = 0.75f;
        }
        if (text_typing_cooldown <= 0.0f)
        {
            if (title_text_offset > 0)
            {
                --title_text_offset;
                auto temp = std::string(title_text_target.size() - title_text_offset, ' ');
                memcpy(temp.data(), title_text_target.data(), temp.size());
                title_label->setText(temp);
                text_typing_cooldown = 0.05f;
            }
            else if (credit_text_offset > 0)
            {
                --credit_text_offset;
                auto temp = std::string(credit_text_target.size() - credit_text_offset, ' ');
                memcpy(temp.data(), credit_text_target.data(), temp.size());
                credit_label->setText(temp);
                text_typing_cooldown = 0.02f;
            }
        }
        if (remaining_time <= 0.0f)
        {
            remaining_time = 8.0f;
            reset();
        }

        static bool borderless = true;
        if (Input::wasKeyPressed('B'))
        {
            RenderServer::setBorderless(!borderless);
            borderless = !borderless;
        }
        if (Input::wasKeyPressed('F'))
        {
            RenderServer::setSize({ RenderServer::getFramebufferSize().y * (4.0f / 3.0f), RenderServer::getFramebufferSize().y });
        }
    }

    void reset()
    {
        mesh_obj->getTransform().setMatrix(glm::mat4(1.0f));
        angular_speed = glm::vec3{ random(-10.0f, 10.0f), random(-8.0f, 8.0f), random(-60.0f, 60.0f) };
        if (Engine::getScene())
        {
            auto tex = Engine::getScene()->render_graph->getFinalImage();
            mesh_obj->material->setTexture("screen", tex->duplicate());
        }
        auto image = images[rand() % images.size()];
        sky_material->setTexture("tex", Engine::loadTexture(std::get<0>(image)));
        title_text_target = std::get<1>(image);
        title_text_offset = title_text_target.size();
        credit_text_target = std::get<2>(image);
        credit_text_offset = credit_text_target.size();
        title_label->setText("");
        credit_label->setText("");
    }
};

int main()
{
    HopEngine::init(Engine::InitParams{false, false, Debug::DEBUG_INFO, false});
    Engine::startApplication<MyGame>();
    HopEngine::destroy();

    return 0;
}

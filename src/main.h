#if !defined(STANDALONE)

#include "common.h"
#include "engine.h"
#include "node_view.h"

using namespace HopEngine;

class AshaApp : public Application
{
private:
    WeakRef<Object> asha;
    WeakRef<Material> cc_material;

public:
    AshaApp();
    ~AshaApp() = default;

    void update(float delta_time) override;
    void drawImGui() override;
};

class NodeApp : public Application
{
private:
    WeakRef<NodeView> node_view;
    WeakRef<NodeView::Style> style;

public:
    NodeApp();
    ~NodeApp() = default;

    void update(float delta_time) override;
    void drawImGui() override;
};

class MuseumApp : public Application
{
private:
    WeakRef<Material> cc_material;

    WeakRef<Object> crt;

    WeakRef<Object> obj;
    WeakRef<Object> obj2;
    WeakRef<Object> obj3;
    WeakRef<Object> obj4;

    WeakRef<Object> normal_demo_1;
    WeakRef<Object> normal_demo_2;

    WeakRef<Object> spline_obj;
    Spline spline;
    float spline_progress = 0.0f;
    bool spline_tracked   = false;

    bool camera_flythrough       = false;
    float camera_flythrough_time = 0.0f;
    Spline flythrough_spline{
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

public:
    MuseumApp();
    ~MuseumApp() = default;

    void update(float delta_time) override;
    void drawImGui() override;
};

#endif
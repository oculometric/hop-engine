#include "hop_engine.h"

namespace HopEngine
{

class Editor : public Application
{
private:
    Ref<Scene> view_3d;
    Ref<Scene> view_nodes;
    WeakRef<NodeView> node_view;

public:
    Editor();

    void update(float delta_time) override;
};

}
#include "debug.h"
#include "engine.h"
#include "events.h"
#include "input.h"
#include "package.h"
#include "render_server.h"
#include "user_interface.h"

#define IN_PLACE_INIT(var, type, args)                   \
    var = reinterpret_cast<type*>(malloc(sizeof(type))); \
    new (var) type args

using namespace HopEngine;

extern unsigned char engine_hop_raw[];
extern unsigned long long engine_hop_raw_size;

static Debug* debug                = nullptr;
static Engine* engine              = nullptr;
static EventServer* event_server   = nullptr;
static Package* package            = nullptr;
static RenderServer* render_server = nullptr;
static Input* input                = nullptr;
static UIManager* ui_manager       = nullptr;

class HopEngine::InitMachine final
{
public:
    static void initialise(const Engine::InitParams& params)
    {
        // TODO: checks for success, checks for already-inited
        bool success = true;

        if (!debug)
        {
            Debug::InitParams debug_params{ Debug::DEBUG_FAULT, params.debug_log_level,
                params.create_log_file };
            IN_PLACE_INIT(debug, Debug, (debug_params, success));
            if (success) DBG_INFO("initialised debug");
            else
                return;
        }

        if (!engine)
        {
            IN_PLACE_INIT(engine, Engine, (params, success));
            if (success) DBG_INFO("initialised engine");
            else
                return;
        }

        if (!event_server)
        {
            EventServer::InitParams event_server_params{};
            IN_PLACE_INIT(event_server, EventServer, (event_server_params, success));
            if (success) DBG_INFO("initialised event server");
            else
                return;
        }

        if (!package)
        {
            Package::InitParams package_params{};
            IN_PLACE_INIT(package, Package, (package_params, success));
            if (success) DBG_INFO("initialised package manager");
            else
                return;
            DataBlock engine_hop(engine_hop_raw_size);
            memcpy(engine_hop.data(), engine_hop_raw, engine_hop.size());
            Package::importPackage(engine_hop);
        }

        if (!render_server)
        {
            RenderServer::InitParams render_server_params{ params.enable_vulkan_validation,
                params.enable_transparent_window };
            IN_PLACE_INIT(render_server, RenderServer, (render_server_params, success));
            if (success) DBG_INFO("initialised render server");
            else
                return;
        }

        if (!input)
        {
            Input::InitParams input_params{};
            IN_PLACE_INIT(input, Input, (input_params, success));
            if (success) DBG_INFO("initialised input manager");
            else
                return;
        }

        if (!ui_manager)
        {
            IN_PLACE_INIT(ui_manager, UIManager, ());
            DBG_INFO("initialised UI manager");
        }

        EventServer::dispatch(Engine::EVENT_TYPE_INIT_FINISH);
    }

    static void destroy()
    {
        if (event_server) EventServer::dispatch(Engine::EVENT_TYPE_DESTROY_START);
        if (engine) Engine::reset();

        if (ui_manager)
        {
            delete ui_manager;
            ui_manager = nullptr;
            DBG_INFO("destroyed UI manager");
        }

        if (input)
        {
            delete input;
            input = nullptr;
            DBG_INFO("destroyed input manager");
        }

        if (render_server)
        {
            delete render_server;
            render_server = nullptr;
            DBG_INFO("destroyed render server");
        }

        if (package)
        {
            delete package;
            package = nullptr;
            DBG_INFO("destroyed package manager");
        }

        if (event_server)
        {
            delete event_server;
            event_server = nullptr;
            DBG_INFO("destroyed event server");
        }

        if (engine)
        {
            if (Engine::countTrackedObjects() > 0)
            {
                DBG_ERROR(
                    "uh oh! there are objects still allocated! prepare for vulkan errors and possibly crashes! see below:");
                Engine::summariseTrackedObjects();
            }
            else
                DBG_INFO("good girl for cleaning up!");

            delete engine;
            engine = nullptr;
            DBG_INFO("destroyed engine");
        }

        if (debug)
        {
            delete debug;
            debug = nullptr;
        }
    }
};

void HopEngine::init(const Engine::InitParams& params) { InitMachine::initialise(params); }

void HopEngine::destroy() { InitMachine::destroy(); }

Engine* Engine::getInstance()
{
    if (!engine) DBG_ERROR("engine instance has not been initialised!");
    return engine;
}
Debug* Debug::getInstance() { return debug; }
EventServer* EventServer::getInstance()
{
    if (!event_server) DBG_ERROR("event server instance has not been initialised!");
    return event_server;
}
Package* Package::getInstance()
{
    if (!package) DBG_ERROR("package manager instance has not been initialised!");
    return package;
}
RenderServer* RenderServer::getInstance()
{
    if (!render_server) DBG_ERROR("render server instance has not been initialised!");
    return render_server;
}
Input* Input::getInstance()
{
    if (!input) DBG_ERROR("input manager instance has not been initialised!");
    return input;
}
UIManager* UIManager::getInstance() { return ui_manager; }

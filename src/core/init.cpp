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
        IN_PLACE_INIT(debug, Debug, (Debug::DEBUG_FAULT, params.create_log_file));
        Debug::setLogLevel(params.debug_log_level);
        DBG_INFO("initialised debug");

        IN_PLACE_INIT(event_server, EventServer, ());
        DBG_INFO("initialised event server");

        IN_PLACE_INIT(package, Package, ());
        DBG_INFO("initialised package manager");
        DataBlock engine_hop(engine_hop_raw_size);
        memcpy(engine_hop.data(), engine_hop_raw, engine_hop.size());
        Package::importPackage(engine_hop);

        IN_PLACE_INIT(engine, Engine, (params));
        DBG_INFO("initialised engine");
        IN_PLACE_INIT(render_server, RenderServer, (params.enable_vulkan_validation));
        DBG_INFO("initialised render server");
        IN_PLACE_INIT(input, Input, ());
        DBG_INFO("initialised input manager");
        IN_PLACE_INIT(ui_manager, UIManager, ());
        DBG_INFO("initialised UI manager");

        EventServer::dispatch(Engine::EVENT_TYPE_INIT_FINISH);
    }

    static void destroy()
    {
        EventServer::dispatch(Engine::EVENT_TYPE_DESTROY_START);
        // TODO: checks for is-inited
        Engine::reset();

        delete ui_manager;
        ui_manager = nullptr;
        DBG_INFO("destroyed UI manager");

        delete input;
        input = nullptr;
        DBG_INFO("destroyed input manager");
        delete package;
        package = nullptr;
        DBG_INFO("destroyed package manager");
        delete render_server;
        render_server = nullptr;
        DBG_INFO("destroyed render server");
        delete event_server;
        event_server = nullptr;
        DBG_INFO("destroyed event server");
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

        delete debug;
        debug = nullptr;
    }
};

void HopEngine::init(const Engine::InitParams& params) { InitMachine::initialise(params); }

void HopEngine::destroy() { InitMachine::destroy(); }

// TODO: checks here!
Engine* Engine::getInstance() { return engine; }
Debug* Debug::getInstance() { return debug; }
EventServer* EventServer::getInstance() { return event_server; }
Package* Package::getInstance() { return package; }
RenderServer* RenderServer::getInstance() { return render_server; }
Input* Input::getInstance() { return input; }
UIManager* UIManager::getInstance() { return ui_manager; }

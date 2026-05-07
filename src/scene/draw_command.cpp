#include "material.h"
#include "render_server.h"
#include "scene.h"

using namespace HopEngine;

bool DrawCommand::operator()(const DrawCommand& a, const DrawCommand& b) const
{ return DrawCommand::compare(a, b); }

bool DrawCommand::compare(const DrawCommand& a, const DrawCommand& b)
{
    if (a.draw_priority > b.draw_priority) return true;
    if (a.draw_priority < b.draw_priority) return false;
    if (a.material->getShader() < b.material->getShader()) return true;
    if (a.material->getShader() > b.material->getShader()) return false;
    if (a.material < b.material) return true;
    if (a.material > b.material) return false;
    if (a.uniforms < b.uniforms) return true;
    if (a.uniforms > b.uniforms) return false;
    if (a.mesh < b.mesh) return true;
    if (a.mesh > b.mesh) return false;
    return false;
}

Sky::Sky(Ref<Texture> skybox_texture)
{
    uniforms = RenderServer::createObjectUniforms();
    setSkyboxCubemap(skybox_texture);
}

Sky::Sky(Ref<Material> custom_material, bool render_cube)
{
    uniforms = RenderServer::createObjectUniforms();
    setCustomMaterial(custom_material, render_cube);
}

void Sky::setSkyboxCubemap(Ref<Texture> skybox_texture)
{
    if (render_custom_material || !material)
    {
        material               = new Material(Engine::loadShader("res://engine/shaders/skybox.glsl"),
            Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthWrite(false).depthTest(false));
        render_custom_material = false;
    }
    render_as_cube = true;
    material->setTexture("tex", skybox_texture);
}

void Sky::setCustomMaterial(Ref<Material> custom_material, bool render_cube)
{
    render_custom_material = true;
    material               = custom_material;
    render_as_cube         = render_cube;
}

DrawCommand Sky::getDrawCommand() const
{
    return DrawCommand(material,
        render_as_cube ? RenderServer::getSkyboxCube() : RenderServer::getSkySphere(), uniforms)
        .priority(1000);
}

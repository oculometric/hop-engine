#include "basic_components.h"

#include "engine.h"
#include "mesh.h"
#include "font.h"
#include "material.h"
#include "texture.h"

using namespace HopEngine;
using namespace std;

void TextComponent::awake()
{
    font = new Font("res://engine/font.bmp", glm::ivec2{ 10, 18 });
    material = new Material(Engine::loadShader("res://engine/shaders/text.glsl"));
    material->setTexture(0, font->getAtlas());
    material->setSampler(0, Engine::makeSampler(Sampler::Builder().filter(Sampler::FILTER_NEAREST)));
    
    setText("Sample text");
}

static glm::vec2 flipUV(glm::vec2 v)
{
    return { v.x, 1.0f - v.y };
}

void TextComponent::updateGeometry()
{
    if (!mesh)
        mesh = new Mesh({ Vertex() }, { 0 }, true);
    
    vector<Vertex> vertices; vertices.reserve(text.size() * 4);
    vector<uint16_t> indices; indices.reserve(text.size() * 6);
    glm::vec2 position = { 0.0f, 0.0f };
    for (char c : text)
    {
        if (c == '\n')
        {
            position.y += font->getGlyphSize().y * 0.02f;
            position.x = 0.0f;
            continue;
        }
        glm::vec2 uv_base = font->getGlyphUVOffset(c);
        glm::vec2 uv_size = font->getGlyphUVSize();

        glm::vec2 uv_bl = flipUV(uv_base + glm::vec2{ 0, uv_size.y });
        glm::vec2 uv_br = flipUV(uv_base + uv_size);
        glm::vec2 uv_tl = flipUV(uv_base);
        glm::vec2 uv_tr = flipUV(uv_base + glm::vec2{ uv_size.x, 0 });

        glm::vec2 char_size = font->getGlyphSize() * 0.02f;
        float top_inset = 0;
        glm::vec4 pos_bl = { position.x, (-position.y - char_size.y) - top_inset, 0, 1 };
        glm::vec4 pos_br = { position.x + char_size.x, (-position.y - char_size.y) - top_inset, 0, 1 };
        glm::vec4 pos_tl = { position.x, -position.y - top_inset, 0, 1 };
        glm::vec4 pos_tr = { position.x + char_size.x, -position.y - top_inset, 0, 1 };

        uint16_t v_off = static_cast<uint16_t>(vertices.size());
        vertices.push_back(Vertex{ pos_bl, glm::vec4(tint, 1), { 0, 0, 0.5f, 0 }, {}, uv_bl });
        vertices.push_back(Vertex{ pos_br, glm::vec4(tint, 1), { 0, 0, 0.5f, 0 }, {}, uv_br });
        vertices.push_back(Vertex{ pos_tl, glm::vec4(tint, 1), { 0, 0, 0.5f, 0 }, {}, uv_tl });
        vertices.push_back(Vertex{ pos_tr, glm::vec4(tint, 1), { 0, 0, 0.5f, 0 }, {}, uv_tr });

        indices.push_back(v_off + 1);
        indices.push_back(v_off + 3);
        indices.push_back(v_off + 0);
        indices.push_back(v_off + 3);
        indices.push_back(v_off + 2);
        indices.push_back(v_off + 0);
        
        position.x += font->getGlyphSize().x * 0.02f;
    }
    mesh->updateData(vertices, indices);
    
    // TODO: expanded rendering with proper font, etc
}

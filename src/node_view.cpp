#include "node_view.h"

#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

NodeView::NodeView() : Object(nullptr, nullptr)
{
    material = new Material(new Shader("res://node_shader", false), VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL);
    material->setTexture("sliced_texture", new Texture("res://node_frame.png"));
    material->setSampler("sliced_texture", new Sampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    material->setTexture("text_atlas", new Texture("res://font.bmp"));
    material->setSampler("text_atlas", new Sampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT));

    updateMesh();
}

void NodeView::addFrame(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 tint, vector<Vertex>& vertices, vector<uint16_t>& indices)
{
    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    glm::vec3 segment_size = { size.x / 6.0f, size.y / 6.0f, 0 };
    vertices.push_back(Vertex{ { position.x, -position.y, layer * 0.001f }, segment_size, tint, {}, { 0, 1 } });
    vertices.push_back(Vertex{ { position.x + size.x, -position.y, layer * 0.001f }, segment_size, tint, {}, { 1, 1 } });
    vertices.push_back(Vertex{ { position.x, -position.y - size.y, layer * 0.001f }, segment_size, tint, {}, { 0, 0 } });
    vertices.push_back(Vertex{ { position.x + size.x, -position.y - size.y, layer * 0.001f }, segment_size, tint, {}, { 1, 0 } });

    indices.push_back(v_off + 0);
    indices.push_back(v_off + 3);
    indices.push_back(v_off + 1);
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 2);
    indices.push_back(v_off + 3);
}

glm::vec2 flipUV(glm::vec2 v)
{
    return { v.x, 1.0f - v.y };
}

void NodeView::addCharacter(char c, glm::vec2 position, int layer, vector<Vertex>& vertices, vector<uint16_t>& indices)
{
    float chars_horizontal = 32.0f;
    float chars_height = 320.0f / 18.0f;
    glm::vec2 uv_base = { glm::fract(c / chars_horizontal), glm::floor(c / chars_horizontal) / chars_height };
    glm::vec2 uv_size = { 1.0f / chars_horizontal, 1.0f / chars_height };

    glm::vec2 uv_bl = flipUV(uv_base + glm::vec2{ 0, uv_size.y });
    glm::vec2 uv_br = flipUV(uv_base + uv_size);
    glm::vec2 uv_tl = flipUV(uv_base);
    glm::vec2 uv_tr = flipUV(uv_base + glm::vec2{ uv_size.x, 0 });

    float z_height = layer * 0.001f;
    glm::vec2 size = glm::vec2{ 10.0f, 18.0f };
    glm::vec3 pos_bl = { position.x, -position.y - size.y, z_height };
    glm::vec3 pos_br = { position.x + size.x, -position.y - size.y, z_height };
    glm::vec3 pos_tl = { position.x, -position.y, z_height };
    glm::vec3 pos_tr = { position.x + size.x, -position.y, z_height };

    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    vertices.push_back(Vertex{ pos_bl, { 0, 0, 1 }, {}, {}, uv_bl });
    vertices.push_back(Vertex{ pos_br, { 0, 0, 1 }, {}, {}, uv_br });
    vertices.push_back(Vertex{ pos_tl, { 0, 0, 1 }, {}, {}, uv_tl });
    vertices.push_back(Vertex{ pos_tr, { 0, 0, 1 }, {}, {}, uv_tr });

    indices.push_back(v_off + 0);
    indices.push_back(v_off + 3);
    indices.push_back(v_off + 1);
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 2);
    indices.push_back(v_off + 3);
}

void NodeView::updateMesh()
{
    if (boxes.empty())
    {
        mesh = nullptr;
        return;
    }
    
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    for (const Node& box : boxes)
    {
        addFrame(box.position, glm::vec2(box.size) * 6.0f, 0, { 1, 1, 1 }, vertices, indices);
        addFrame(box.position, glm::vec2{ box.size.x, 5 } * 6.0f, 1, { 1, 0, 0 }, vertices, indices);
        glm::vec2 character_start = glm::vec2{ 12.0f, 7.0f };
        glm::vec2 character_offset = character_start;
        for (char c : box.title)
        {
            addCharacter(c, glm::vec2(box.position) + character_offset, 2, vertices, indices);
            character_offset.x += 9.0f;
        }
        //addFrame(box.position + glm::ivec2{ 0, box.size.y - 5 }, { box.size.x, 5 }, 1, vertices, indices);
        /*size_t horizontal_offset = 0;
        for (char c : box.title)
        {
            addCharacter(c, glm::vec2(box.position) + glm::vec2{ ((float)horizontal_offset * 10.0f / 6.0f) + 1, box.size.y - (2 * 18.0f / 6.0f) }, 2, vertices, indices);
            horizontal_offset++;
        }
        horizontal_offset = 0;
        size_t height_offset = 0;
        for (char c : box.description)
        {
            if (c == '\n' || horizontal_offset >= box.size.x - 2)
            {
                height_offset += 2;
                horizontal_offset = 0;
                if (c == '\n')
                    continue;
            }
            addCharacter(c, glm::vec2(box.position) + glm::vec2{ ((float)horizontal_offset * 10.0f / 6.0f) + 1, box.size.y - (6 + height_offset) }, 2, vertices, indices);
            horizontal_offset++;
        }*/
    }
    mesh = new Mesh(vertices, indices);
}

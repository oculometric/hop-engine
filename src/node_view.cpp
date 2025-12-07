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

void NodeView::addFrame(glm::vec2 position, glm::vec2 size, int layer, vector<Vertex>& vertices, vector<uint16_t>& indices)
{
    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    vertices.push_back(Vertex{ { position.x, position.y, layer * 0.01f }, { size.x, size.y, 0 }, {}, {}, { 0, 0 } });
    vertices.push_back(Vertex{ { position.x + size.x, position.y, layer * 0.01f }, { size.x, size.y, 0 }, {}, {}, { 1, 0 } });
    vertices.push_back(Vertex{ { position.x, position.y + size.y, layer * 0.01f }, { size.x, size.y, 0 }, {}, {}, { 0, 1 } });
    vertices.push_back(Vertex{ { position.x + size.x, position.y + size.y, layer * 0.01f }, { size.x, size.y, 0 }, {}, {}, { 1, 1 } });

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

void NodeView::addCharacter(char c, glm::vec2 position, glm::vec2 size, int layer, vector<Vertex>& vertices, vector<uint16_t>& indices)
{
    float chars_horizontal = 32.0f;
    float chars_height = 320.0f / 18.0f;
    glm::vec2 uv_base = { glm::fract(c / chars_horizontal), glm::floor(c / chars_horizontal) / chars_height };
    glm::vec2 uv_size = { 1.0f / chars_horizontal, 1.0f / chars_height };

    glm::vec2 uv_tl = flipUV(uv_base);
    glm::vec2 uv_tr = flipUV(uv_base + glm::vec2{ uv_size.x, 0 });
    glm::vec2 uv_bl = flipUV(uv_base + glm::vec2{ 0, uv_size.y });
    glm::vec2 uv_br = flipUV(uv_base + uv_size);

    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    vertices.push_back(Vertex{ { position.x, position.y, layer * 0.01f }, { 0, 0, 1 }, {}, {}, uv_bl });
    vertices.push_back(Vertex{ { position.x + size.x, position.y, layer * 0.01f }, { 0, 0, 1 }, {}, {}, uv_br });
    vertices.push_back(Vertex{ { position.x, position.y + size.y, layer * 0.01f }, { 0, 0, 1 }, {}, {}, uv_tl });
    vertices.push_back(Vertex{ { position.x + size.x, position.y + size.y, layer * 0.01f }, { 0, 0, 1 }, {}, {}, uv_tr });

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
        addFrame(box.position, box.size, 0, vertices, indices);
        addFrame(box.position + glm::ivec2{ 0, box.size.y - 4 }, { box.size.x, 4 }, 1, vertices, indices);
        size_t horizontal_offset = 0;
        for (char c : box.title)
        {
            addCharacter(c, box.position + glm::ivec2{ horizontal_offset + 1, box.size.y - 3 }, { 1, 2 }, 2, vertices, indices);
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
            addCharacter(c, box.position + glm::ivec2{ horizontal_offset + 1, box.size.y - (6 + height_offset) }, { 1, 2 }, 2, vertices, indices);
            horizontal_offset++;
        }
    }
    mesh = new Mesh(vertices, indices);
}

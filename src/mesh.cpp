#include "mesh.h"

#include <stdexcept>
#include <fstream>
#include <glm/gtc/matrix_access.hpp>
#include <sstream>

#include "graphics_environment.h"
#include "buffer.h"
#include "package.h"

using namespace HopEngine;
using namespace std;

Mesh::Mesh(string path)
{
    vector<Vertex> verts;
    vector<uint16_t> inds;

    if (readFileToArrays(path, verts, inds))
        createFromArrays(verts, inds);
    else
        DBG_ERROR("failed to load mesh " + path);

    DBG_INFO("created mesh from " + path + " with " + to_string(verts.size()) + " vertices and " + to_string(inds.size()) + " indices");
}

Mesh::Mesh(vector<Vertex> vertices, vector<uint16_t> indices)
{
    createFromArrays(vertices, indices);

    DBG_INFO("created mesh from arrays with " + to_string(vertices.size()) + " vertices and " + to_string(indices.size()) + " indices");
}

Mesh::~Mesh()
{
    DBG_INFO("destroying mesh " + PTR(this));
    vertex_buffer = nullptr;
    index_buffer = nullptr;
}

VkBuffer Mesh::getVertexBuffer()
{
    return vertex_buffer->getBuffer();
}

VkBuffer HopEngine::Mesh::getIndexBuffer()
{
    return index_buffer->getBuffer();
}

VkVertexInputBindingDescription Mesh::getBindingDescription()
{
    VkVertexInputBindingDescription binding_description{ };
    binding_description.binding = 0;
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding_description.stride = sizeof(Vertex);

    return binding_description;
}

array<VkVertexInputAttributeDescription, 5> Mesh::getAttributeDescriptions()
{
    array<VkVertexInputAttributeDescription, 5> attributes;
    attributes[0].binding = 0;
    attributes[0].location = 0;
    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset = offsetof(Vertex, position);

    attributes[1].binding = 0;
    attributes[1].location = 1;
    attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[1].offset = offsetof(Vertex, colour);

    attributes[2].binding = 0;
    attributes[2].location = 2;
    attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[2].offset = offsetof(Vertex, normal);

    attributes[3].binding = 0;
    attributes[3].location = 3;
    attributes[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[3].offset = offsetof(Vertex, tangent);

    attributes[4].binding = 0;
    attributes[4].location = 4;
    attributes[4].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[4].offset = offsetof(Vertex, uv);

    return attributes;
}

struct FaceCorner { uint16_t co; uint16_t uv; uint16_t vn; };

// splits a formatted OBJ face corner into its component indices
static inline FaceCorner splitOBJFaceCorner(string str)
{
    FaceCorner fci = { 0,0,0 };
    size_t first_break_ind = str.find('/');
    fci.co = static_cast<uint16_t>(stoi(str.substr(0, first_break_ind)) - 1);
    if (first_break_ind == string::npos) return fci;
    size_t second_break_ind = str.find('/', first_break_ind + 1);
    if (second_break_ind != first_break_ind + 1)
        fci.uv = static_cast<uint16_t>(stoi(str.substr(first_break_ind + 1, second_break_ind - first_break_ind)) - 1);
    fci.vn = static_cast<uint16_t>(stoi(str.substr(second_break_ind + 1, str.find('/', second_break_ind + 1) - second_break_ind)) - 1);

    return fci;
}

struct FaceCornerReference
{
    uint16_t normal_index;
    uint16_t uv_index;
    uint16_t transferred_vert_index;
};

static glm::vec3 computeTangent(glm::vec3 co_a, glm::vec3 co_b, glm::vec3 co_c, glm::vec2 uv_a, glm::vec2 uv_b, glm::vec2 uv_c)
{
    // vector from the target vertex to the second vertex
    glm::vec3 ab = { co_b.x - co_a.x, co_b.y - co_a.y, co_b.z - co_a.z };
    // vector from the target vertex to the third vertex
    glm::vec3 ac = { co_c.x - co_a.x, co_c.y - co_a.y, co_c.z - co_a.z };
    // delta uv between target and second
    glm::vec2 uv_ab = { uv_b.x - uv_a.x, uv_b.y - uv_a.y };
    // delta uv between target and third
    glm::vec2 uv_ac = { uv_c.x - uv_a.x, uv_c.y - uv_a.y };
    // matrix representing UVs
    glm::mat3 uv_mat = glm::mat3
    (
        uv_ab.x, uv_ac.x, 0,
        uv_ab.y, uv_ac.y, 0,
        0, 0, 1
    );
    // matrix representing vectors between vertices
    glm::mat3 vec_mat = glm::mat3
    (
        ab.x, ac.x, 0,
        ab.y, ac.y, 0,
        ab.z, ac.z, 0
    );

    // we should be able to express the vectors from A->B and A->C with reference to the difference in UV coordinate and the tangent and bitangent:
    //
    // AB = (duv_ab.x * T) + (duv_ab.y * B)
    // AC = (duv_ac.x * T) + (duv_ac.y * B)
    // 
    // this gives us 6 simultaneous equations for the XYZ coordinates of the tangent and bitangent
    // these can be expressed and solved with matrices:
    // 
    // [ AB.x  AC.x  0 ]     [ T.x  B.x  N.x ]   [ duv_ab.x  duv_ac.x  0 ]
    // [ AB.y  AC.y  0 ]  =  [ T.y  B.y  N.y ] * [ duv_ab.y  duv_ac.y  0 ]
    // [ AB.z  AC.z  0 ]     [ T.z  B.z  N.z ]   [ 0         0         1 ]
    //

    vec_mat = vec_mat * glm::inverse(uv_mat);

    return glm::normalize(glm::vec3{ vec_mat[0] }); // extract tangent
}

bool Mesh::readFileToArrays(string path, vector<Vertex>& verts, vector<uint16_t>& inds)
{
    auto file_data = Package::tryLoadFile(path);
    stringstream file(string((char*)(file_data.data())));

    // vectors to load data into
    vector<glm::vec3> tmp_co;
    vector<glm::vec3> tmp_cl;
    vector<FaceCorner> tmp_fc;
    vector<glm::vec2> tmp_uv;
    vector<glm::vec3> tmp_vn;

    // temporary locations for reading data to
    string tmps;
    glm::vec3 tmp3;
    glm::vec3 tmp2;

    // repeat for every line in the file
    while (!file.eof())
    {
        file >> tmps;
        if (tmps == "v")
        {
            // read a vertex coordinate
            file >> tmp3.x;
            file >> tmp3.y;
            file >> tmp3.z;
            tmp_co.push_back(tmp3);
            if (file.peek() != '\n')
            {
                file >> tmp3.x;
                file >> tmp3.y;
                file >> tmp3.z;
                tmp_cl.push_back(tmp3);
            }
            else
            {
                tmp_cl.push_back({ 0, 0, 0 });
            }
        }
        else if (tmps == "vn")
        {
            // read a face corner normal
            file >> tmp3.x;
            file >> tmp3.y;
            file >> tmp3.z;
            tmp_vn.push_back(tmp3);
        }
        else if (tmps == "vt")
        {
            // read a face corner uv (texture coordinate)
            file >> tmp2.x;
            file >> tmp2.y;
            tmp_uv.push_back(tmp2);
        }
        else if (tmps == "f")
        {
            // read a face (only supports triangles)
            file >> tmps;
            tmp_fc.push_back(splitOBJFaceCorner(tmps));
            file >> tmps;
            tmp_fc.push_back(splitOBJFaceCorner(tmps));
            file >> tmps;
            tmp_fc.push_back(splitOBJFaceCorner(tmps));

            swap(tmp_fc[tmp_fc.size() - 1], tmp_fc[tmp_fc.size() - 3]);
        }
        file.ignore(SIZE_MAX, '\n');
    }

    // swap the first and last face corner of each triangle, to flip the face order
    for (uint32_t i = 0; i < tmp_fc.size() - 2; i += 3)
    {
        FaceCorner fc_i = tmp_fc[i];
        tmp_fc[i] = tmp_fc[i + 2];
        tmp_fc[i + 2] = fc_i;
    }

    // for each coordinate, stores a list of all the times it has been used by a face corner, and what the normal/uv index was for that face corner
    // this allows us to tell when we should split a vertex (i.e. if it has already been used by another face corner but which had a different normal and/or a different uv)
    vector<vector<FaceCornerReference>> fc_normal_uses(tmp_co.size(), vector<FaceCornerReference>());

    verts.clear();
    inds.clear();

    for (FaceCorner fc : tmp_fc)
    {
        bool found_matching_vertex = false;
        uint16_t match = 0;
        for (FaceCornerReference existing : fc_normal_uses[fc.co])
        {
            if (existing.normal_index == fc.vn && existing.uv_index == fc.uv)
            {
                found_matching_vertex = true;
                match = existing.transferred_vert_index;
                break;
            }
        }

        if (found_matching_vertex)
        {
            inds.push_back(match);
        }
        else
        {
            Vertex new_vert;
            new_vert.position = tmp_co[fc.co];
            new_vert.colour = tmp_cl[fc.co];
            if (tmp_vn.size() > fc.uv)
                new_vert.normal = tmp_vn[fc.vn];
            if (tmp_uv.size() > fc.uv)
                new_vert.uv = tmp_uv[fc.uv];

            uint16_t new_index = static_cast<uint16_t>(verts.size());
            fc_normal_uses[fc.co].push_back(FaceCornerReference{ fc.vn, fc.uv, new_index });

            inds.push_back(new_index);
            verts.push_back(new_vert);
        }
    }

    if (tmp_vn.size() == 0)
    {
        for (size_t i = 0; i < inds.size() - 2; i += 3)
        {
            const uint16_t i0 = inds[i];
            const uint16_t i1 = inds[i + 1];
            const uint16_t i2 = inds[i + 2];

            const glm::vec3 v0 = verts[i0].position;
            const glm::vec3 v1 = verts[i1].position;
            const glm::vec3 v2 = verts[i2].position;

            glm::vec3 e01 = v1 - v0;
            glm::vec3 e02 = v2 - v0;
            glm::vec3 e12 = v2 - v1;
            glm::vec3 normal = glm::cross(e01, e02);

            verts[i0].normal += normal;
            verts[i1].normal += normal;
            verts[i2].normal += normal;
        }

        for (Vertex& vert : verts)
            vert.normal = glm::normalize(vert.normal);
    }

    // compute tangents
    vector<bool> touched = vector<bool>(verts.size(), false);
    for (uint32_t tri = 0; tri < inds.size() / 3; tri++)
    {
        uint16_t v0 = inds[(tri * 3) + 0]; Vertex f0 = verts[v0];
        uint16_t v1 = inds[(tri * 3) + 1]; Vertex f1 = verts[v1];
        uint16_t v2 = inds[(tri * 3) + 2]; Vertex f2 = verts[v2];

        if (!touched[v1]) verts[v1].tangent = computeTangent(f1.position, f0.position, f2.position, f1.uv, f0.uv, f2.uv);
        if (!touched[v2]) verts[v2].tangent = computeTangent(f2.position, f0.position, f1.position, f2.uv, f0.uv, f1.uv);
        if (!touched[v0]) verts[v0].tangent = computeTangent(f0.position, f1.position, f2.position, f0.uv, f1.uv, f2.uv);

        touched[v0] = true; touched[v1] = true; touched[v2] = true;
    }

    // transform from Z back Y up space into Z up Y forward space
    for (Vertex& fv : verts)
    {
        fv.position = { fv.position.x, -fv.position.z, fv.position.y };
        fv.normal = { fv.normal.x, -fv.normal.z, fv.normal.y };
        fv.tangent = { fv.tangent.x, -fv.tangent.z, fv.tangent.y };
    }

    return true;
}

void Mesh::createFromArrays(vector<Vertex> verts, vector<uint16_t> inds)
{
    Ref<Buffer> staging_buffer = new Buffer(sizeof(Vertex) * verts.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer->mapMemory(), verts.data(), staging_buffer->getSize());
    staging_buffer->unmapMemory();
    vertex_buffer = new Buffer(staging_buffer->getSize(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    staging_buffer->copyToBuffer(vertex_buffer);

    staging_buffer = new Buffer(sizeof(uint16_t) * inds.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer->mapMemory(), inds.data(), staging_buffer->getSize());
    staging_buffer->unmapMemory();
    index_buffer = new Buffer(staging_buffer->getSize(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    staging_buffer->copyToBuffer(index_buffer);

    index_count = inds.size();
}

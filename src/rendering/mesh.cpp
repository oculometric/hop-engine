#include "mesh.h"

#include <fstream>
#include <sstream>

#include "render_server.h"
#include "buffer.h"
#include "command_buffer.h"
#include "package.h"

using namespace HopEngine;
using namespace std;

Mesh::Mesh(const string& path)
{
    origin = path;
    vector<Vertex> verts;
    vector<uint16_t> inds;
    
    if (readFileToArrays(path, verts, inds))
    {
        createFromArrays(verts, inds);
        is_renderable = true;        
    }
    else
        DBG_ERROR("failed to load mesh " + path);

    DBG_VERBOSE("created mesh from " + path + " with " + ::to_string(verts.size()) + " vertices and " + ::to_string(inds.size()) + " indices");
}

Mesh::Mesh(const vector<Vertex>& vertices, const vector<uint16_t>& indices, const bool keep_accessible)
{
    accessible = keep_accessible;
    if (!keep_accessible)
        createFromArrays(vertices, indices);
    else
    {
        vertex_buffer = new Buffer(sizeof(Vertex) * vertices.size(), Buffer::BUFFER_USAGE_VERTEX,
                                   MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
        memcpy(vertex_buffer->mapMemory(), vertices.data(), vertex_buffer->getSize());
        vertex_buffer->unmapMemory();

        index_buffer = new Buffer(sizeof(uint16_t) * indices.size(), Buffer::BUFFER_USAGE_INDEX,
                                  MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
        memcpy(index_buffer->mapMemory(), indices.data(), index_buffer->getSize());
        index_buffer->unmapMemory();

        vertex_space = vertices.size();
        index_space = indices.size();
        index_count = index_space;
        recomputeBoundingBox(vertices);
    }
    is_renderable = true;        
    DBG_VERBOSE("created mesh from arrays with " + ::to_string(vertices.size()) + " vertices and " + ::to_string(indices.size()) + " indices");
}

Mesh::~Mesh()
{
    DBG_VERBOSE("destroying mesh '" + getOrigin() + '\'');
}

struct BinaryMeshHeader
{
    char signature[4];
    uint32_t vertex_count;
    uint32_t vertex_stride;
    uint32_t index_count;
};

vector<uint8_t> Mesh::encodeBinaryMesh(const string& path)
{
    vector<Vertex> verts;
    vector<uint16_t> inds;
    readFileToArrays(path, verts, inds);
    
    BinaryMeshHeader header;
    header.signature[0] = 'H';
    header.signature[1] = 'B';
    header.signature[2] = 'M';
    header.signature[3] = 'R';
    header.vertex_count = static_cast<uint32_t>(verts.size());
    header.vertex_stride = sizeof(Vertex);
    header.index_count = static_cast<uint32_t>(inds.size());
    
    vector<uint8_t> data(sizeof(BinaryMeshHeader)
        + (verts.size() * header.vertex_stride)
        + (inds.size() * sizeof(uint16_t)));
    memcpy(data.data(), &header, sizeof(BinaryMeshHeader));
    memcpy(data.data() + sizeof(BinaryMeshHeader), verts.data(), verts.size() * header.vertex_stride);
    memcpy(data.data() + sizeof(BinaryMeshHeader) + (verts.size() * header.vertex_stride), inds.data(), inds.size() * sizeof(uint16_t));
    
    return data;
}

void Mesh::updateData(const vector<Vertex>& vertices, const vector<uint16_t>& indices, size_t vertex_alloc, size_t index_alloc)
{
    if (!accessible)
    {
        DBG_WARNING("attempted to update mesh '" + getOrigin() + "' which is not accessible to CPU memory");
        return;
    }

    vertex_alloc = max(vertex_alloc, vertices.size());
    if (vertex_alloc != vertex_space)
    {
        vertex_buffer = new Buffer(sizeof(Vertex) * vertex_alloc, Buffer::BUFFER_USAGE_VERTEX,
                                   MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
    }

    memcpy(vertex_buffer->mapMemory(), vertices.data(), vertices.size() * sizeof(Vertex));
    vertex_buffer->unmapMemory();
    vertex_space = vertex_alloc;
    vertex_count = vertices.size();

    index_alloc = max(index_alloc, indices.size());
    if (index_alloc != index_space)
    {
        index_buffer = new Buffer(sizeof(uint16_t) * index_alloc, Buffer::BUFFER_USAGE_INDEX,
                                  MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
    }

    memcpy(index_buffer->mapMemory(), indices.data(), indices.size() * sizeof(uint16_t));
    index_buffer->unmapMemory();
    index_space = index_alloc;
    index_count = indices.size();
    
    recomputeBoundingBox(vertices);
}

void Mesh::draw(WeakRef<DrawCommandBuffer> command_buffer)
{
    vertex_buffer->bind(command_buffer, 0);
    index_buffer->bind(command_buffer, 1);
    command_buffer->drawMeshInternal(getIndexCount());
}

bool Mesh::decodeBinaryMesh(const vector<uint8_t>& data, vector<Vertex>& verts, vector<uint16_t>& inds)
{
    BinaryMeshHeader header = *reinterpret_cast<const BinaryMeshHeader*>(data.data());
    if (header.vertex_stride != sizeof(Vertex))
    {
        DBG_ERROR("error loading binary mesh: invalid vertex stride");
        return false;
    }
    if (sizeof(BinaryMeshHeader) + (static_cast<size_t>(header.vertex_count) * header.vertex_stride) + (static_cast<size_t>(header.index_count) * sizeof(uint16_t)) != data.size())
    {
        DBG_ERROR("error loading binary mesh: invalid vertex stride");
        return false;
    }
    
    verts.resize(header.vertex_count);
    inds.resize(header.index_count);
    memcpy(verts.data(), data.data() + sizeof(BinaryMeshHeader), verts.size() * header.vertex_stride);
    memcpy(inds.data(), data.data() + sizeof(BinaryMeshHeader) + (verts.size() * header.vertex_stride), inds.size() * sizeof(uint16_t));
    
    return true;
}

struct FaceCorner { uint16_t co; uint16_t uv; uint16_t vn; };

// splits a formatted OBJ face corner into its component indices
static FaceCorner splitOBJFaceCorner(const string& str)
{
    FaceCorner fci = { 0,0,0 };
    const size_t first_break_ind = str.find('/');
    fci.co = static_cast<uint16_t>(stoi(str.substr(0, first_break_ind)) - 1);
    if (first_break_ind == string::npos) return fci;
    const size_t second_break_ind = str.find('/', first_break_ind + 1);
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
    glm::mat3 uv_mat = glm::mat3(glm::vec3(uv_ab, 0), glm::vec3(uv_ac, 0), { 0, 0, 1 });
    // matrix representing vectors between vertices
    glm::mat3 vec_mat = glm::mat3(ab, ac, { 0,0,0 });

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

    vec_mat = (vec_mat) * glm::inverse((uv_mat));

    return glm::normalize(glm::vec3{ vec_mat[0] }); // extract tangent
}

bool Mesh::readFileToArrays(const string& path, vector<Vertex>& verts, vector<uint16_t>& inds)
{
    const auto file_data = Package::load(path);
    if (file_data.size() < 4)
    {
        DBG_WARNING("mesh file " + path + " is empty or does not exist");
        return false;
    }
    if (file_data[0] == 'H'
        && file_data[1] == 'B'
        && file_data[2] == 'M'
        && file_data[3] == 'R')
    {
        return decodeBinaryMesh(file_data, verts, inds);
    }
    const auto string_data = string(reinterpret_cast<const char*>(file_data.data()), file_data.size());
    auto stream = stringstream(string_data);

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
    string line;
    while (getline(stream, line))
    {
        auto file = stringstream(line);
        file >> tmps;
        if (tmps == "v")
        {
            // read a vertex coordinate
            file >> tmp3.x;
            file >> tmp3.y;
            file >> tmp3.z;
            tmp_co.push_back(tmp3);
            auto peeked = file.peek();
            if (peeked != -1 && peeked != '\n' && peeked != '\r')
            {
                file >> tmp3.x;
                file >> tmp3.y;
                file >> tmp3.z;
                tmp_cl.push_back(tmp3);
            }
            else
            {
                tmp_cl.emplace_back(1, 1, 1);
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
            tmp_uv.emplace_back(tmp2);
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
        }
    }

    // for each coordinate, stores a list of all the times it has been used by a face corner, and what the normal/uv index was for that face corner
    // this allows us to tell when we should split a vertex (i.e. if it has already been used by another face corner but which had a different normal and/or a different uv)
    vector<vector<FaceCornerReference>> fc_normal_uses(tmp_co.size(), vector<FaceCornerReference>());

    verts.clear();
    inds.clear();

    for (auto [co, uv, vn] : tmp_fc)
    {
        bool found_matching_vertex = false;
        uint16_t match = 0;
        for (auto [normal_index, uv_index, transferred_vert_index] : fc_normal_uses[co])
        {
            if (normal_index == vn && uv_index == uv)
            {
                found_matching_vertex = true;
                match = transferred_vert_index;
                break;
            }
        }

        if (found_matching_vertex)
            inds.push_back(match);
        else
        {
            Vertex new_vert;
            new_vert.position = glm::vec4(tmp_co[co], 1);
            new_vert.colour = glm::vec4(tmp_cl[co], 0);
            if (vn < tmp_vn.size())
                new_vert.normal = glm::vec4(tmp_vn[vn], 0);
            if (uv < tmp_uv.size())
                new_vert.uv = tmp_uv[uv];

            uint16_t new_index = static_cast<uint16_t>(verts.size());
            fc_normal_uses[co].push_back(FaceCornerReference{ vn, uv, new_index });

            inds.push_back(new_index);
            verts.push_back(new_vert);
        }
    }

    if (tmp_vn.empty())
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
            glm::vec4 normal = glm::vec4(glm::cross(e01, e02), 0);

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

        if (!touched[v1]) verts[v1].tangent = glm::vec4(computeTangent(f1.position, f0.position, f2.position, f1.uv, f0.uv, f2.uv), 1);
        if (!touched[v2]) verts[v2].tangent = glm::vec4(computeTangent(f2.position, f0.position, f1.position, f2.uv, f0.uv, f1.uv), 1);
        if (!touched[v0]) verts[v0].tangent = glm::vec4(computeTangent(f0.position, f1.position, f2.position, f0.uv, f1.uv, f2.uv), 1);

        touched[v0] = true; touched[v1] = true; touched[v2] = true;
    }

    // transform from Z back Y up space into Z up Y forward space
    for (Vertex& fv : verts)
    {
        fv.position = { fv.position.x, -fv.position.z, fv.position.y, 1 };
        fv.normal = { fv.normal.x, -fv.normal.z, fv.normal.y, 0 };
        fv.tangent = { fv.tangent.x, -fv.tangent.z, fv.tangent.y, 0 };
    }

    return true;
}

void Mesh::createFromArrays(const vector<Vertex>& verts, const vector<uint16_t>& inds)
{
    Ref<Buffer> staging_buffer = new Buffer(sizeof(Vertex) * verts.size(), Buffer::BUFFER_USAGE_TRANSFER_SRC,
        MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
    memcpy(staging_buffer->mapMemory(), verts.data(), staging_buffer->getSize());
    staging_buffer->unmapMemory();
    vertex_buffer = new Buffer(staging_buffer->getSize(),
        Buffer::BUFFER_USAGE_VERTEX | Buffer::BUFFER_USAGE_TRANSFER_DST, MEMORY_PROPERTY_DEVICE_LOCAL);
    staging_buffer->copyToBuffer(vertex_buffer);

    staging_buffer = new Buffer(sizeof(uint16_t) * inds.size(), Buffer::BUFFER_USAGE_TRANSFER_SRC,
        MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
    memcpy(staging_buffer->mapMemory(), inds.data(), staging_buffer->getSize());
    staging_buffer->unmapMemory();
    index_buffer = new Buffer(staging_buffer->getSize(),
        Buffer::BUFFER_USAGE_INDEX | Buffer::BUFFER_USAGE_TRANSFER_DST, MEMORY_PROPERTY_DEVICE_LOCAL);
    staging_buffer->copyToBuffer(index_buffer);

    vertex_space = verts.size();
    index_space = inds.size();
    vertex_count = verts.size();
    index_count = index_space;
    
    recomputeBoundingBox(verts);
}

void Mesh::recomputeBoundingBox(const vector<Vertex>& verts)
{
    if (verts.empty())
    {
        bounding_box = BoundingBox{ { 0, 0, 0 }, { 0.25f, 0.25f, 0.25f } };
        return;
    }
    
    glm::vec3 min = verts[0].position;
    glm::vec3 max = verts[0].position;
    
    for (const auto& vert : verts)
    {
        min = glm::min(glm::vec3(vert.position), min);
        max = glm::max(glm::vec3(vert.position), max);
    }
    
    bounding_box = BoundingBox{ (min + max) * 0.5f, max - min };
}

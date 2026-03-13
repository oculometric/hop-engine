#include "mesh.h"

#include <fstream>
#include <sstream>

#include "render_server.h"
#include "buffer.h"
#include "command_buffer.h"
#include "package.h"

using namespace HopEngine;
using namespace std;

// Mesh::Mesh(const string& path)
// {
//     origin = path;
//     vector<Vertex> verts;
//     vector<uint16_t> inds;
    
//     if (readFileToArrays(path, verts, inds))
//     {
//         uploadFromArrays(verts, inds);
//         is_renderable = true;        
//     }
//     else
//         DBG_ERROR("failed to load mesh " + path);

//     DBG_VERBOSE("created mesh from " + path + " with " + ::to_string(verts.size()) + " vertices and " + ::to_string(inds.size()) + " indices");
// }

/*
*/


Mesh::Mesh(const vector<Vertex>& vertices, const vector<uint16_t>& indices, const bool keep_accessible)
{
    accessible = keep_accessible;
    vertex_capacity = vertices.size();
    index_capacity = indices.size();
    uploadFromArrays(vertices, indices);
    DBG_VERBOSE("created mesh from arrays with " + ::to_string(vertices.size()) + " vertices and " + ::to_string(indices.size()) + " indices");
}

Mesh::~Mesh()
{
    DBG_VERBOSE("destroying mesh '" + getOrigin() + '\'');
}

Ref<Mesh> Mesh::loadMesh(const string& path)
{
    const auto file_data = Package::load(path);
    if (file_data.size() < 4)
    {
        DBG_WARNING("mesh file " + path + " is empty or does not exist");
        return nullptr;
    }
    
    vector<Vertex> verts;
    vector<uint16_t> inds;

    if (strncmp(reinterpret_cast<const char*>(file_data.data()), "HBMR", 4) == 0)
    {
        if (decodeBinaryMesh(file_data, verts, inds))
            return new Mesh(verts, inds);
        else
            return nullptr;
    }
    else
    {
        if (readOBJ(file_data, verts, inds))
            return new Mesh(verts, inds);
        else
            return nullptr;
    }
}

DataBlock Mesh::convertToBinaryMesh(const string& obj_path)
{
    const auto file_data = Package::load(obj_path);
    if (file_data.size() < 4)
    {
        DBG_ERROR("mesh file " + obj_path + " is empty or does not exist");
        return { };
    }

    if (strncmp(reinterpret_cast<const char*>(file_data.data()), "HBMR", 4) == 0)
    {
        DBG_WARNING("mesh file " + obj_path + " is a binary mesh, not an OBJ file");
        return file_data;
    }

    vector<Vertex> verts;
    vector<uint16_t> inds;

    if (!readOBJ(file_data, verts, inds))
    {
        DBG_ERROR("mesh file " + obj_path + " could not be read");
        return { };
    }

    return encodeBinaryMesh(verts, inds);
}

void Mesh::updateData(const vector<Vertex>& vertices, const vector<uint16_t>& indices, size_t vertex_alloc, size_t index_alloc)
{
    if (!accessible)
    {
        DBG_WARNING("attempted to update mesh '" + getOrigin() + "' which is not accessible to CPU memory");
        return;
    }

    vertex_capacity = max(vertex_alloc, vertices.size());
    index_capacity = max(index_alloc, indices.size());
    uploadFromArrays(vertices, indices);
    
    recomputeBoundingBox(vertices);
}

void Mesh::draw(WeakRef<DrawCommandBuffer> command_buffer)
{
    if (!vertex_buffer || !index_buffer)
        return;
    vertex_buffer->bind(command_buffer, 0);
    index_buffer->bind(command_buffer, 1);
    command_buffer->drawMeshInternal(getIndexCount());
}

void Mesh::uploadFromArrays(const vector<Vertex>& verts, const vector<uint16_t>& inds)
{
    size_t vertex_buffer_size = sizeof(Vertex) * vertex_capacity;
    size_t index_buffer_size = sizeof(uint16_t) * index_capacity;
    size_t vertex_data_size = sizeof(Vertex) * verts.size();
    size_t index_data_size = sizeof(uint16_t) * inds.size();

    if (accessible)
    {
        if (!vertex_buffer || vertex_buffer->getSize() != vertex_buffer_size)
            vertex_buffer = new Buffer(vertex_buffer_size, Buffer::BUFFER_USAGE_VERTEX,
                MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
        memcpy(vertex_buffer->mapMemory(), verts.data(), vertex_data_size);
        vertex_buffer->unmapMemory();

        if (!index_buffer || index_buffer->getSize() != index_buffer_size)
            index_buffer = new Buffer(index_buffer_size, Buffer::BUFFER_USAGE_INDEX,
                MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
        memcpy(index_buffer->mapMemory(), inds.data(), index_data_size);
        index_buffer->unmapMemory();
    }
    else
    {
        Ref<Buffer> staging_buffer = new Buffer(vertex_data_size, Buffer::BUFFER_USAGE_TRANSFER_SRC,
            MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
        memcpy(staging_buffer->mapMemory(), verts.data(), vertex_data_size);
        staging_buffer->unmapMemory();
        vertex_buffer = new Buffer(vertex_buffer_size,
            Buffer::BUFFER_USAGE_VERTEX | Buffer::BUFFER_USAGE_TRANSFER_DST, MEMORY_PROPERTY_DEVICE_LOCAL);
        staging_buffer->copyToBuffer(vertex_buffer);

        staging_buffer = new Buffer(index_data_size, Buffer::BUFFER_USAGE_TRANSFER_SRC,
            MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
        memcpy(staging_buffer->mapMemory(), inds.data(), index_data_size);
        staging_buffer->unmapMemory();
        index_buffer = new Buffer(index_buffer_size,
            Buffer::BUFFER_USAGE_INDEX | Buffer::BUFFER_USAGE_TRANSFER_DST, MEMORY_PROPERTY_DEVICE_LOCAL);
        staging_buffer->copyToBuffer(index_buffer);
    }
    
    vertex_count = verts.size();
    index_count = inds.size();
    
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

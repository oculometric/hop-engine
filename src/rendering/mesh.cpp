#include "mesh.h"

#include "buffer.h"
#include "command_buffer.h"
#include "package.h"
#include "graphics_server.h"

using namespace HopEngine;

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices,
    const bool keep_accessible)
{
    accessible      = keep_accessible;
    vertex_capacity = vertices.size();
    index_capacity  = indices.size();
    uploadFromArrays(vertices, indices);
    DBG_VERBOSE("created mesh from arrays with " + std::to_string(vertices.size()) + " vertices and " +
                std::to_string(indices.size()) + " indices");
}

Mesh::~Mesh() { DBG_VERBOSE("destroying mesh '" + getOrigin() + '\''); }

void Mesh::updateData(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices,
    size_t vertex_alloc, size_t index_alloc)
{
    if (!accessible)
    {
        DBG_WARNING("attempted to update mesh '" + getOrigin() + "' which is not accessible to CPU memory");
        return;
    }

    vertex_capacity = glm::max(vertex_alloc, vertices.size());
    index_capacity  = glm::max(index_alloc, indices.size());
    uploadFromArrays(vertices, indices);

    recomputeBoundingBox(vertices);
}

Ref<Mesh> Mesh::loadMesh(const std::string& path)
{
    const auto file_data = Package::load(path);
    if (file_data.size() < 4)
    {
        DBG_WARNING("mesh file " + path + " is empty or does not exist");
        return nullptr;
    }

    std::vector<Vertex> verts;
    std::vector<uint16_t> inds;

    if (strncmp(reinterpret_cast<const char*>(file_data.data()), "HBMR", 4) == 0)
    {
        if (!decodeBinaryMesh(file_data, verts, inds)) return nullptr;
    }
    else
    {
        if (!readOBJ(file_data, verts, inds)) return nullptr;
    }
    auto m    = new Mesh(verts, inds);
    m->origin = path;
    return m;
}

void Mesh::draw(WeakRef<DrawCommandBuffer> command_buffer)
{
    if (!vertex_buffer || !index_buffer)
    {
        GraphicsServer::getDefaultMesh()->draw(command_buffer);
        return;
    }
    vertex_buffer->bind(command_buffer);
    index_buffer->bind(command_buffer);
    command_buffer->drawMeshInternal(getIndexCount());
}

void Mesh::uploadFromArrays(const std::vector<Vertex>& verts, const std::vector<uint16_t>& inds)
{
    size_t vertex_buffer_size = sizeof(Vertex) * vertex_capacity;
    size_t index_buffer_size  = sizeof(uint16_t) * index_capacity;
    size_t vertex_data_size   = sizeof(Vertex) * verts.size();
    size_t index_data_size    = sizeof(uint16_t) * inds.size();

    if (accessible)
    {
        if (!vertex_buffer || vertex_buffer->getSize() != vertex_buffer_size)
            vertex_buffer = new Buffer(glm::max(vertex_buffer_size, static_cast<size_t>(4)),
                Buffer::BUFFER_USAGE_VERTEX, MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
        memcpy(vertex_buffer->mapMemory(), verts.data(), vertex_data_size);
        vertex_buffer->unmapMemory();

        if (!index_buffer || index_buffer->getSize() != index_buffer_size)
            index_buffer = new Buffer(glm::max(index_buffer_size, static_cast<size_t>(4)),
                Buffer::BUFFER_USAGE_INDEX, MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
        memcpy(index_buffer->mapMemory(), inds.data(), index_data_size);
        index_buffer->unmapMemory();
    }
    else
    {
        Ref<Buffer> staging_buffer = new Buffer(glm::max(vertex_data_size, static_cast<size_t>(4)),
            Buffer::BUFFER_USAGE_TRANSFER_SRC,
            MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
        memcpy(staging_buffer->mapMemory(), verts.data(), vertex_data_size);
        staging_buffer->unmapMemory();
        vertex_buffer = new Buffer(glm::max(vertex_buffer_size, static_cast<size_t>(4)),
            Buffer::BUFFER_USAGE_VERTEX | Buffer::BUFFER_USAGE_TRANSFER_DST, MEMORY_PROPERTY_DEVICE_LOCAL);
        staging_buffer->copyToBuffer(vertex_buffer);

        staging_buffer =
            new Buffer(glm::max(index_data_size, static_cast<size_t>(4)), Buffer::BUFFER_USAGE_TRANSFER_SRC,
                MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
        memcpy(staging_buffer->mapMemory(), inds.data(), index_data_size);
        staging_buffer->unmapMemory();
        index_buffer = new Buffer(glm::max(index_buffer_size, static_cast<size_t>(4)),
            Buffer::BUFFER_USAGE_INDEX | Buffer::BUFFER_USAGE_TRANSFER_DST, MEMORY_PROPERTY_DEVICE_LOCAL);
        staging_buffer->copyToBuffer(index_buffer);
    }

    vertex_count = verts.size();
    index_count  = inds.size();

    recomputeBoundingBox(verts);
}

void Mesh::recomputeBoundingBox(const std::vector<Vertex>& verts)
{
    if (verts.empty())
    {
        bounding_box = BoundingBox{
            {     0,     0,     0 },
            { 0.25f, 0.25f, 0.25f }
        };
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

#include "mesh.h"
#include "package.h"

using namespace HopEngine;

struct BinaryMeshHeader
{
    char signature[4];
    uint32_t vertex_count;
    uint32_t vertex_stride;
    uint32_t index_count;
};

DataBlock Mesh::encodeBinaryMesh(const std::vector<Vertex>& verts, const std::vector<uint16_t>& inds)
{
    BinaryMeshHeader header;
    header.signature[0]  = 'H';
    header.signature[1]  = 'B';
    header.signature[2]  = 'M';
    header.signature[3]  = 'R';
    header.vertex_count  = static_cast<uint32_t>(verts.size());
    header.vertex_stride = sizeof(Vertex);
    header.index_count   = static_cast<uint32_t>(inds.size());

    DataBlock data(sizeof(BinaryMeshHeader) + (verts.size() * header.vertex_stride) +
                   (inds.size() * sizeof(uint16_t)));
    memcpy(data.data(), &header, sizeof(BinaryMeshHeader));
    memcpy(data.data() + sizeof(BinaryMeshHeader), verts.data(), verts.size() * header.vertex_stride);
    memcpy(data.data() + sizeof(BinaryMeshHeader) + (verts.size() * header.vertex_stride), inds.data(),
        inds.size() * sizeof(uint16_t));

    return data;
}

bool Mesh::decodeBinaryMesh(const DataBlock& data, std::vector<Vertex>& verts, std::vector<uint16_t>& inds)
{
    BinaryMeshHeader header = *reinterpret_cast<const BinaryMeshHeader*>(data.data());
    if (data.size() < sizeof(BinaryMeshHeader))
    {
        DBG_ERROR("error loading binary mesh: truncated header");
        return false;
    }
    if (header.vertex_stride != sizeof(Vertex))
    {
        DBG_ERROR("error loading binary mesh: invalid vertex stride");
        return false;
    }
    if (strncmp(header.signature, "HBMR", 4) != 0)
    {
        DBG_ERROR("error loading binary mesh: invalid header signature");
        return false;
    }
    if (sizeof(BinaryMeshHeader) + (static_cast<size_t>(header.vertex_count) * header.vertex_stride) +
            (static_cast<size_t>(header.index_count) * sizeof(uint16_t)) !=
        data.size())
    {
        DBG_ERROR("error loading binary mesh: size mismatch");
        return false;
    }

    verts.resize(header.vertex_count);
    inds.resize(header.index_count);
    memcpy(verts.data(), data.data() + sizeof(BinaryMeshHeader), verts.size() * header.vertex_stride);
    memcpy(inds.data(), data.data() + sizeof(BinaryMeshHeader) + (verts.size() * header.vertex_stride),
        inds.size() * sizeof(uint16_t));

    return true;
}

DataBlock Mesh::convertToBinaryMesh(const std::string& obj_path)
{
    const auto file_data = Package::load(obj_path);
    if (file_data.size() < 4)
    {
        DBG_ERROR("mesh file " + obj_path + " is empty or does not exist");
        return {};
    }

    if (strncmp(reinterpret_cast<const char*>(file_data.data()), "HBMR", 4) == 0)
    {
        DBG_WARNING("mesh file " + obj_path + " is a binary mesh, not an OBJ file");
        return file_data;
    }

    std::vector<Vertex> verts;
    std::vector<uint16_t> inds;

    if (!readOBJ(file_data, verts, inds))
    {
        DBG_ERROR("mesh file " + obj_path + " could not be read");
        return {};
    }

    auto result = encodeBinaryMesh(verts, inds);

    float compression_ratio = static_cast<float>(result.size()) / static_cast<float>(file_data.size());
    DBG_INFO("OBJ mesh file " + obj_path + " went from " + std::to_string(file_data.size()) + " to " +
             std::to_string(result.size()) + " bytes during compression (" +
             std::to_string(compression_ratio) + "x compression ratio).");

    return result;
}
#include "mesh.h"

#include "package.h"

using namespace HopEngine;
using namespace std;


struct BinaryMeshHeader
{
    char signature[4];
    uint32_t vertex_count;
    uint32_t vertex_stride;
    uint32_t index_count;
};

DataBlock Mesh::encodeBinaryMesh(vector<Vertex>& verts, vector<uint16_t>& inds)
{
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

bool Mesh::decodeBinaryMesh(const DataBlock& data, vector<Vertex>& verts, vector<uint16_t>& inds)
{
    BinaryMeshHeader header = *reinterpret_cast<const BinaryMeshHeader*>(data.data());
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
    if (sizeof(BinaryMeshHeader) + (static_cast<size_t>(header.vertex_count) * header.vertex_stride) + (static_cast<size_t>(header.index_count) * sizeof(uint16_t)) != data.size())
    {
        DBG_ERROR("error loading binary mesh: size");
        return false;
    }
    
    verts.resize(header.vertex_count);
    inds.resize(header.index_count);
    memcpy(verts.data(), data.data() + sizeof(BinaryMeshHeader), verts.size() * header.vertex_stride);
    memcpy(inds.data(), data.data() + sizeof(BinaryMeshHeader) + (verts.size() * header.vertex_stride), inds.size() * sizeof(uint16_t));
    
    return true;
}

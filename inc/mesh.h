#pragma once

#include "common.h"
#include "math_helpers.h"

#include <array>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace HopEngine
{

/**
 * @brief GPU-renderable triangle mesh class.
 */
class Mesh final : public Destructible
{
public:
    /**
     * @brief universally shared vertex format.
     */
    struct Vertex final
    {
        glm::vec4 position;
        glm::vec4 colour;
        glm::vec4 normal;
        glm::vec4 tangent;
        glm::vec3 uv;
    };

private:
    std::string origin; // if not empty, contains the path from which this mesh was loaded
    Ref<Buffer> vertex_buffer;  // GPU vertex buffer
    Ref<Buffer> index_buffer;   // GPU index buffer
    size_t vertex_capacity = 0; // capacity of the GPU vertex buffer in vertices
    size_t vertex_count    = 0; // actual number of vertices populated in the GPU buffer
    size_t index_capacity  = 0; // capacity of the GPU index buffer in indices
    size_t index_count     = 0; // actual number of indices populated in the GPU buffer
    // if `true` the buffers are configured to be CPU accessible for efficient frequent updating of data
    bool accessible = false;
    BoundingBox bounding_box; // model-space AABB encapsulating the mesh

public:
    DELETE_CONSTRUCTORS(Mesh);
    /**
     * @brief constructs a new mesh, including GPU buffers, based on arrays of vertices and indices. buffer
     * capacity is calculated to match the number of elements.
     * @param vertices array of vertices making up the mesh.
     * @param indices array of indices making up the mesh.
     * @param keep_accessible if `true`, the mesh will be kept in CPU-visible GPU memory, allowing the mesh
     * data to be updated later at minimal cost. however, this may make rendering performance for the mesh
     * slightly worse.
     */
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices,
        bool keep_accessible = false);
    ~Mesh() override;

    /**
     * @brief constructs a mesh instance from an OBJ or binary mesh format. the type is detected
     * automatically based on the contents of the target file.
     * @param path path to the target mesh.
     * @returns constructed mesh object, or `nullptr` if the mesh could not be decoded.
     */
    static Ref<Mesh> loadMesh(const std::string& path);
    /**
     * @brief converts an OBJ mesh into a binary mesh format, often saving more than 30% in storage cost for
     * large meshes as well as significantly reducing loading times.
     * @param obj_path path to the target OBJ input mesh.
     * @returns byte array representing the binary encoded mesh.
     */
    static DataBlock convertToBinaryMesh(const std::string& obj_path);

    std::string getOrigin() const
    {
        if (this == nullptr) return "0x0";
        return origin.empty() ? PTR(this) : origin;
    }
    size_t getVertexCount() const { return vertex_count; }
    size_t getIndexCount() const { return index_count; }
    BoundingBox getBoundingBox() const { return bounding_box; }
    /**
     * @brief updates the mesh data based on new vertex and index arrays. if the mesh was not made
     * accessible at construction, this function returns immediately. vertex and index buffer capacity will
     * be modified if necessary to match the values specified by `vertex_alloc` and `index_alloc` (or the
     * size of the `vertices` and `indices` arrays, whichever is larger). these last two arguments can be
     * used to minimise the amount of buffer reallocation performed due to frequent mesh updates.
     * @param vertices new vertex data for the mesh.
     * @param indices new index data for the mesh.
     * @param vertex_alloc new preferred capacity for the vertex array (in vertices).
     * @param index_alloc new preferred capacity for the index array (in indices).
     */
    void updateData(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices,
        size_t vertex_alloc = 0, size_t index_alloc = 0);

    /**
     * @brief issues commands to bind and draw the mesh into a GPU graphics command buffer. a render pass
     * should have already been started and a material should already have been bound.
     * @param command_buffer graphics command buffer into which commands will be issued.
     */
    void draw(WeakRef<DrawCommandBuffer> command_buffer);

private:
    /**
     * @brief converts arrays of vertices and indices into a binary mesh encoding format for efficient
     * storage or interchange.
     * @param verts vertex array.
     * @param inds index array.
     * @returns byte array representing the binary encoded mesh.
     */
    static DataBlock encodeBinaryMesh(const std::vector<Vertex>& verts, const std::vector<uint16_t>& inds);
    /**
     * @brief extracts arrays of vertices and indices from a binary mesh encoding format.
     * @param data byte array containing the binary encoded mesh.
     * @param verts output array for the mesh vertices. cleared during mesh loading.
     * @param inds output array for the mesh indices. cleared during mesh loading.
     * @returns `true` if the mesh was loaded successfully, or `false` if an issue occurred, such as an
     * invalid signature, unsupported vertex stride, or mismatched data size.
     */
    static bool decodeBinaryMesh(const DataBlock& data, std::vector<Vertex>& verts,
        std::vector<uint16_t>& inds);
    /**
     * @brief extracts arrays of vertices and indices from an OBJ data file.
     * @param data byte array containing the OBJ mesh file.
     * @param verts output array for the mesh vertices. cleared during mesh loading.
     * @param inds output array for the mesh indices. cleared during mesh loading.
     * @returns `true` if the mesh was loaded successfully, otherwise `false`.
     */
    static bool readOBJ(const DataBlock& data, std::vector<Vertex>& verts, std::vector<uint16_t>& inds);

    /**
     * @brief updates the internal data of the mesh, resizing the GPU buffers if needed.
     * @param verts new vertex buffer data.
     * @param inds new index buffer data.
     */
    void uploadFromArrays(const std::vector<Vertex>& verts, const std::vector<uint16_t>& inds);
    /**
     * @brief recalculates the model-space AABB based on the given array of vertices.
     * @param verts mesh vertex data.
     */
    void recomputeBoundingBox(const std::vector<Vertex>& verts);
};

} // namespace HopEngine

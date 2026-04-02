#pragma once

#include "common.h"

namespace HopEngine
{

/**
 * @brief bitflag enum listing the required properties of a block of memory.
 */
enum MemoryProperties
{
    MEMORY_PROPERTY_DEVICE_LOCAL  = 1,
    MEMORY_PROPERTY_HOST_VISIBLE  = 2,
    MEMORY_PROPERTY_HOST_COHERENT = 4,
    MEMORY_PROPERTY_HOST_CACHED   = 8,
};
ENUM_OPERATOR(MemoryProperties);
TO_STRING_DEC(MemoryProperties);

/**
 * @brief encapsulates a GPU buffer object and its associated memory.
 */
class Buffer final
{
public:
    /**
     * @brief bitflag enum which describes which tasks a buffer may be used for on the GPU.
     */
    enum Usage
    {
        BUFFER_USAGE_TRANSFER_SRC = 1,
        BUFFER_USAGE_TRANSFER_DST = 2,
        BUFFER_USAGE_UNIFORM      = 4,
        BUFFER_USAGE_VERTEX       = 8,
        BUFFER_USAGE_INDEX        = 16
    };

private:
    GPUHandle buffer   = nullptr; // actual GPU buffer object
    GPUHandle memory   = nullptr; // GPU object for the memory allocated to the buffer
    size_t buffer_size = 0;       // size of the buffer in bytes
    void* mapped       = nullptr; // pointer to host-accessible version of device memory

public:
    DELETE_CONSTRUCTORS(Buffer);
    /**
     * @brief allocates a buffer on the GPU of the specified size, with provision
     * for the specified usage type and properties. these values must be set correctly
     * or you will receive Vulkan validation errors and other bad omens.
     * @param size required size of the buffer. final buffer may be bigger due to alignment.
     * @param usage intended usage. multiple may be specified.
     * @param properties required properties, such as being accessible from the CPU. multiple
     * may be specified.
     */
    Buffer(size_t size, Usage usage, MemoryProperties properties);
    ~Buffer();

    /**
     * @brief tries to find a suitable Vulkan GPU memory type index for the given inputs.
     * @param type_bits should be the value of a VkMemoryRequirements::memoryTypeBits
     * struct field.
     * @param _properties memory property flags for the target memory type.
     * @return Vulkan GPU memory type index.
     */
    static uint32_t findMemoryType(uint32_t type_bits, MemoryProperties _properties);

    GPUHandle getHandle() const { return buffer; }
    size_t getSize() const { return buffer_size; }
    /**
     * @brief requests for the buffer to be mapped into CPU-accessible memory. will fail
     * if the buffer was not initialised with \code MEMORY_PROPERTY_HOST_VISIBLE\endcode.
     * the mapped memory will be available until \code unmapMemory()\endcode is called,
     * or the buffer is destroyed. repeated calls will not result in the memory being
     * remapped.
     * @return a pointer to the allocated memory.
     */
    void* mapMemory();
    /**
     * @brief deallocates the memory allocated by \code mapMemory()\endcode, after which
     * the pointer returned by that function is invalid. called automatically when the
     * buffer object destructs.
     */
    void unmapMemory();
    /**
     * @brief copies the contents of this buffer into another, on the GPU (no memory
     * needs to be mapped CPU-side).
     * @param other the destination buffer.
     */
    void copyToBuffer(const Ref<Buffer>& other) const;

    /**
     * @brief binds the buffer for rendering in a command buffer, either as a vertex or
     * index buffer.
     * @param command_buffer command buffer to issue a bind command into.
     * @param type 0 to bind as a vertex buffer, 1 to bind as an index buffer.
     */
    void bind(WeakRef<DrawCommandBuffer> command_buffer, int type); // TODO: eliminate type, look at usage bits to figure it out!
};

ENUM_OPERATOR(Buffer::Usage)
TO_STRING_DEC(Buffer::Usage);

} // namespace HopEngine

#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "buffer.h"
#include "render_server.h"
#include "package.h"

using namespace HopEngine;
using namespace std;

TO_STRING_DEF_BITFLAGS(Texture::Usage, 5, VARGS("TRANSFER_SRC", "TRANSFER_DST", "SAMPLED", "COLOR_ATTACHMENT", "DEPTH_STENCIL_ATTACHMENT"));

TO_STRING_DEF(Texture::Format, 4, VARGS("R8G8B8A8_SRGB", "D32_SFLOAT_S8_UINT", "R16G16B16A16_SFLOAT", "B8G8R8A8_SRGB"));

TO_STRING_DEF(Texture::Layout, 8, VARGS("UNDEFINED", "PRESENT_SRC", "COLOR_ATTACHMENT", "DEPTH_STENCIL_ATTACHMENT", "SHADER_READ_ONLY", "DEPTH_STENCIL_READ_ONLY", "TRANSFER_SRC", "TRANSFER_DST"))

Texture::Texture(const size_t _width, const size_t _height, const Texture::Format _format, const Texture::Builder& builder)
{
    format = _format;
    usage = builder.usage_flags;
    extent =
    { 
        static_cast<uint32_t>(_width) / builder.layer_arrangement.x, 
        static_cast<uint32_t>(_height) / builder.layer_arrangement.y, 
        builder.layer_arrangement.x * builder.layer_arrangement.y 
    };

    if (builder.data_ptr != nullptr || extent.x == 0 || extent.y == 0)
    {
        loadFromMemory(builder.data_ptr, builder.layer_arrangement);
        DBG_VERBOSE("created image from memory with size " + ::to_string(extent.x) + "x" + ::to_string(extent.y) + " and format " + to_string(format));
    }
    else
    {
        if (extent.x == 0)
        {
            extent.x = 1;
            DBG_WARNING("image width is not allowed to be zero");
        }
        if (extent.y == 0)
        {
            DBG_WARNING("image height is not allowed to be zero");
            extent.y = 1;
        }
        createImage();
        DBG_VERBOSE("created blank image with size " + ::to_string(extent.x) + "x" + ::to_string(extent.y) + " and format " + to_string(format));
    }
}

Texture::Texture(const string& file, const Texture::Builder& builder)
{
    origin = file;
    const auto file_data = Package::tryLoadFile(file);
    int img_width, img_height, img_channels;
    stbi_uc* pixels = stbi_load_from_memory(file_data.data(), static_cast<int>(file_data.size()), &img_width, &img_height, &img_channels, STBI_rgb_alpha);
    format = FORMAT_R8G8B8A8_SRGB;
    usage = builder.usage_flags;
    extent = {
        static_cast<uint32_t>(img_width) / builder.layer_arrangement.x, 
        static_cast<uint32_t>(img_height) / builder.layer_arrangement.y, 
        builder.layer_arrangement.x * builder.layer_arrangement.y
    };

    if (pixels == nullptr)
    {
        DBG_ERROR("failed to load image '" + file + "'");
        extent = { 1, 1, 1 };
        createImage();
    }
    else
    {
        if (builder.layer_arrangement.x == 1 && builder.layer_arrangement.y == 1)
        {
            const size_t row_size = static_cast<size_t>(img_width) * 4;
            const size_t col_size = static_cast<size_t>(img_height);
            void* tmp = new uint8_t[row_size];
            for (size_t i = 0; i < col_size / 2; ++i)
            {
                memcpy(tmp, pixels + (i * row_size), row_size);
                memcpy(pixels + (i * row_size), pixels + ((col_size - i - 1) * row_size), row_size);
                memcpy(pixels + ((col_size - i - 1) * row_size), tmp, row_size);
            }
        }

        loadFromMemory(pixels, builder.layer_arrangement);
        stbi_image_free(pixels);

        DBG_VERBOSE("created image from " + file + " with size " + ::to_string(extent.x) + "x" + ::to_string(extent.y) + " and format " + to_string(format));
    }
}

Texture::~Texture()
{
    DBG_VERBOSE("destroying image '" + getOrigin() + '\'');
    destroyResources();
}

Texture::Format Texture::getDepthFormat()
{ return FORMAT_D32_SFLOAT_S8_UINT; }

Texture::Format Texture::getDataFormat()
{ return FORMAT_R16G16B16A16_SFLOAT; }

void Texture::loadFromMemory(void* data, glm::u32vec2 layers)
{
    const VkDeviceSize image_length = static_cast<VkDeviceSize>(extent.x) * extent.y * extent.z * 4;
    Ref<Buffer> staging_buffer = new Buffer(image_length, BUFFER_USAGE_TRANSFER_SRC, MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
    
    if (layers.x != 1 || layers.y != 1)
    {
        const size_t input_width = static_cast<size_t>(extent.x) * layers.x * 4;
        const size_t layer_width = static_cast<size_t>(extent.x) * 4;
        const size_t layer_height = extent.y;
        vector<uint8_t> rearranged(image_length);
        
        for (size_t slice = 0; slice < extent.z; ++slice)
        {
            size_t origin_offset = (layer_width * (slice % layers.x)) + (input_width * (slice / layers.x) * layer_height);
            size_t destination_offset = layer_width * layer_height * slice;
            for (size_t row = 0; row < layer_height; ++row)
            {
                memcpy(rearranged.data() + destination_offset, static_cast<uint8_t*>(data) + origin_offset, layer_width);
                destination_offset += layer_width;
                origin_offset += input_width;
            }
        }
        memcpy(staging_buffer->mapMemory(), rearranged.data(), image_length);
    }
    else
    {
        memcpy(staging_buffer->mapMemory(), data, image_length);
    }
    staging_buffer->unmapMemory();

    createImage();
    transitionLayout(LAYOUT_TRANSFER_DST);
    copyBufferToImage(staging_buffer);
    transitionLayout(LAYOUT_SHADER_READ_ONLY);
}

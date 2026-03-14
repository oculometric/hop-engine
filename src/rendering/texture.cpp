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

Texture::Texture(glm::u32vec3 image_extent, Format image_format, void* data_ptr)
{
    format = image_format;
    switch (format)
    {
    case FORMAT_SWAPCHAIN:
        usage = IMAGE_USAGE_COLOR_ATTACHMENT; break;
    case FORMAT_DEPTH:
        usage = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT | IMAGE_USAGE_SAMPLED; break;
    case FORMAT_FLOAT_16X4:
    case FORMAT_SRGB_8X4:
        usage = IMAGE_USAGE_COLOR_ATTACHMENT | IMAGE_USAGE_SAMPLED; break;
    }
    if (data_ptr) usage = usage | IMAGE_USAGE_TRANSFER_DST;

    extent = glm::max(image_extent, glm::u32vec3{ 1, 1, 1 });

    createImage();

    if (data_ptr && (usage & IMAGE_USAGE_SAMPLED))
    {
        uploadData(data_ptr);
        DBG_VERBOSE("created image from memory with size " + ::to_string(extent.x) + "x" + ::to_string(extent.y) + "x" + ::to_string(extent.z) + " and format " + to_string(format));
    }
    else
        DBG_VERBOSE("created blank image with size " + ::to_string(extent.x) + "x" + ::to_string(extent.y) + "x" + ::to_string(extent.z) + " and format " + to_string(format));

    createView();
}

Texture::~Texture()
{
    DBG_VERBOSE("destroying image '" + getOrigin() + '\'');
    destroyResources();
}

Ref<Texture> Texture::loadImage(const string& path)
{
    const auto file_data = Package::load(path);
    if (file_data.empty())
    {
        DBG_ERROR("could not load image '" + path + "'");
        return nullptr;
    }

    int img_width, img_height, img_channels;
    stbi_uc* pixels = stbi_load_from_memory(file_data.data(), static_cast<int>(file_data.size()), &img_width, &img_height, &img_channels, STBI_rgb_alpha);
    if (!pixels)
    {
        DBG_ERROR("failed to load image '" + path + "'");
        return nullptr;
    }

    // flip the texture upside down
    const uint32_t row_size = static_cast<uint32_t>(img_width) * 4;
    const uint32_t col_size = static_cast<uint32_t>(img_height);
    void* tmp = new uint8_t[row_size];
    for (uint32_t i = 0; i < col_size / 2; ++i)
    {
        memcpy(tmp, pixels + (i * row_size), row_size);
        memcpy(pixels + (i * row_size), pixels + ((col_size - i - 1) * row_size), row_size);
        memcpy(pixels + ((col_size - i - 1) * row_size), tmp, row_size);
    }

    Ref<Texture> t = new Texture({ static_cast<uint32_t>(img_width), static_cast<uint32_t>(img_height), 1 }, FORMAT_SRGB_8X4, pixels);
    t->origin = path;
    DBG_VERBOSE("created image from " + path + " with size " + ::to_string(extent.x) + "x" + ::to_string(extent.y) + " and format " + to_string(format));
    stbi_image_free(pixels);
    return t;
}

Ref<Texture> Texture::loadImage3D(const string& path, glm::u32vec2 segments)
{
    if (segments == glm::u32vec2{ 1, 1 })
        return loadImage(path);
    else if (segments.x == 0 || segments.y == 0)
    {
        DBG_ERROR("a 3D image must have a valid segments argument when loading");
        return nullptr;
    }

    const auto file_data = Package::load(path);
    if (file_data.empty())
    {
        DBG_ERROR("could not load image '" + path + "'");
        return nullptr;
    }

    int img_width, img_height, img_channels;
    stbi_uc* pixels = stbi_load_from_memory(file_data.data(), static_cast<int>(file_data.size()), &img_width, &img_height, &img_channels, STBI_rgb_alpha);
    if (!pixels)
    {
        DBG_ERROR("failed to load image '" + path + "'");
        return nullptr;
    }

    const uint32_t image_length = static_cast<uint32_t>(img_width) * static_cast<uint32_t>(img_height) * 4;
    const uint32_t input_width = static_cast<uint32_t>(img_width) * 4;
    const uint32_t layer_width = static_cast<uint32_t>(img_width) / segments.x;
    const uint32_t layer_height = static_cast<uint32_t>(img_height) / segments.y;
    const uint32_t layers = segments.x * segments.y;
    if (layer_width == 0 || layer_height == 0)
    {
        stbi_image_free(pixels);
        DBG_ERROR("a 3D image may not have layer dimensions of zero");
        return nullptr;
    }

    vector<uint8_t> rearranged(image_length);
    
    for (uint32_t slice = 0; slice < layers; ++slice)
    {
        uint32_t origin_offset = ((slice % segments.x) * layer_width * 4) + ((slice / segments.x) * input_width * layer_height);
        uint32_t destination_offset = layer_width * layer_height * 4 * slice;
        for (size_t row = 0; row < layer_height; ++row)
        {
            memcpy(rearranged.data() + destination_offset, static_cast<uint8_t*>(pixels) + origin_offset, layer_width * 4);
            destination_offset += layer_width * 4;
            origin_offset += input_width;
        }
    }

    Ref<Texture> t = new Texture({ layer_width, layer_height, layers }, FORMAT_SRGB_8X4, rearranged.data());
    t->origin = path;
    DBG_VERBOSE("created image from " + path + " with size " + ::to_string(extent.x) + "x" + ::to_string(extent.y) + "x" + ::to_string(extent.z) + " and format " + to_string(format));
    stbi_image_free(pixels);

    return t;
}

void Texture::uploadData(void* data)
{
    const VkDeviceSize image_length = static_cast<VkDeviceSize>(extent.x * extent.y * extent.z * 4);
    Ref<Buffer> staging_buffer = new Buffer(image_length, Buffer::BUFFER_USAGE_TRANSFER_SRC, MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);
    memcpy(staging_buffer->mapMemory(), data, image_length);
    transitionLayout(LAYOUT_TRANSFER_DST);
    copyBufferToImage(staging_buffer);
    transitionLayout(LAYOUT_SHADER_READ_ONLY);
}

/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"

#include <glm/glm.hpp>
#include <string>

namespace HopEngine
{

/**
 * @brief manages a GPU texture sampler object.
 */
class Sampler final : public Destructible
{
public:
    /**
     * @brief enumerates texture interpolation (filtering) modes.
     */
    enum Filter
    {
        FILTER_NEAREST, // interpolate to the nearest texel value (aka point filtering)
        FILTER_LINEAR   // interpolate linearly between the neighbouring texel
    };

    /**
     * @brief enumerates texture addressing modes.
     */
    enum Address
    {
        ADDRESS_REPEAT,    // repeat the texture in each direction
        ADDRESS_MIRRORED,  // repeat the texture, flipped alternately
        ADDRESS_CLAMP_EDGE // use the nearest in-bounds texel
    };

private:
    GPUHandle sampler = nullptr;

public:
    DELETE_CONSTRUCTORS(Sampler);
    /**
     * @brief constructs a new sampler with the specified interpolation and addressing modes.
     * @param filter texture interpolation/filtering mode.
     * @param address texture addressing mode.
     */
    Sampler(Filter filtering_mode, Address address_mode);
    ~Sampler() override;

    GPUHandle getSampler() const { return sampler; }
};

/**
 * @brief GPU-usable 2 or 3 dimensional texture class.
 */
class Texture final : public Destructible
{
public:
    /**
     * @brief enumerates texture GPU usage type bitflags.
     */
    enum Usage
    {
        IMAGE_USAGE_DEFAULT          = 0,  // usage assumed from other parameters
        IMAGE_USAGE_WRITEABLE        = 1,  // can be copied/written from
        IMAGE_USAGE_READABLE         = 2,  // can be copied/written to
        IMAGE_USAGE_SHADER           = 4,  // can be sampled by a shader
        IMAGE_USAGE_COLOR_ATTACHMENT = 8,  // can be used as a colour attachment for a render pass
        IMAGE_USAGE_DEPTH_ATTACHMENT = 16, // can be used as a colour attachment for a depth pass
    };

    /**
     * @brief enumerates texture GPU texel format.
     */
    enum Format
    {
        FORMAT_SRGB_8X4,   // sRGB encoded, 4 channel RGBA, 8 bit uint per channel
        FORMAT_DEPTH,      // platform-appropriate depth format, usually 32 bit single channel float
        FORMAT_FLOAT_16X4, // linear encoded, 4 channel RGBA, 16 bit float per channel
        FORMAT_SWAPCHAIN,  // platform-appropriate swapchain format, usually sRGB BGRA 8 bit uint
    };

    /**
     * @brief enumerates texture GPU layout states.
     */
    enum Layout
    {
        LAYOUT_UNDEFINED,        // undefined layout
        LAYOUT_PRESENT_SRC,      // for presenting to the screen, needed by swapchain images
        LAYOUT_COLOR_ATTACHMENT, // input for render pass colour attachments
        LAYOUT_DEPTH_ATTACHMENT, // input for render pass depth-stencil attachments
        LAYOUT_SHADER_READ,      // for shaders to read from
        LAYOUT_DEPTH_READ,       // for shaders to read from, for depth textures
        LAYOUT_TRANSFER_SRC,     // for textures to be copied/written from
        LAYOUT_TRANSFER_DST,     // for textures to be copied/written to
    };

private:
    std::string origin;  // if not empty, contains the path from which this texture was loaded
    glm::u32vec3 extent; // size/resolution of the texture in pixels (in 2D textures, .z is always set to 1)
    Layout current_layout;           // current layout of the GPU image
    Format format;                   // texel/pixel format
    Usage usage;                     // GPU image usage flags
    GPUHandle image       = nullptr; // GPU image object handle
    GPUHandle memory      = nullptr; // GPU memory handle, backing the image
    GPUHandle view        = nullptr; // GPU image view handle, viewing onto the image
    size_t allocated_size = 0;       // size of the allocated image memory

public:
    DELETE_CONSTRUCTORS(Texture);
    /**
     * @brief constructs a new texture from a specified size, texel format, and raw data. if input data is
     * provided, the format MUST be `FORMAT_SRGB_8X4`, if input data is not provided, an empty texture is
     * created.
     * @param image_extent size of the image in pixels. all components must be at least 1. for a 2D texture,
     * the Z component should be set to 1.
     * @param image_format if `data_ptr` is set to `nullptr`, determines the texel format of the image data.
     * @param data_ptr optionally, provides data with which to initialise the texture. must be in sRGB RGBA
     * 8bpp uint format. pixels are arranged in rows, then columns, then planes (i.e. X, then Y, then Z).
     */
    Texture(glm::u32vec3 image_extent, Format image_format, void* data_ptr = nullptr);
    ~Texture() override;

    /**
     * @brief reads in a 2D texture from a file. PNG, JPEG, and BMP are supported.
     * @param path target path from which to read.
     * @returns newly constructed texture, or `nullptr` if there was an error loading the texture.
     */
    static Ref<Texture> loadImage(const std::string& path);
    /**
     * @brief reads in a 3D texture from a file. PNG, JPEG, and BMP are supported.
     * @param path target path from which to read.
     * @param segments how to slice the file data into 3D layers. layers are indexed left to right, top to
     * bottom.
     * @returns newly constructed texture, or `nullptr` if there was an error loading the texture.
     */
    static Ref<Texture> loadImage3D(const std::string& path, glm::u32vec2 segments);

    std::string getOrigin() const
    {
        if (this == nullptr) return "0x0";
        return origin.empty() ? PTR(this) : origin;
    }
    glm::u32vec3 getSize() const { return extent; }
    bool is3D() const { return (extent.z != 1); }
    Format getFormat() const { return format; }
    GPUHandle getView() const { return view; }

    /**
     * @brief transitions the GPU image object to a new layout. required for some operations, such as
     * copying, downloading, and binding to a material. may fail and report an error if an unsupported
     * layout transition is performed.
     * @param new_layout desired target layout.
     */
    void transitionLayout(Layout new_layout);
    /**
     * @brief extracts the contents of the image from the GPU and stores it as raw pixel data.
     * @returns raw data extracted from the texture.
     */
    DataBlock download();
    /**
     * @brief extracts the contents of the image from the GPU and stores it to a file.
     * @param path path to the target output file.
     */
    void storeImage(const std::string& path);

private:
    /**
     * @brief creates the actual GPU image object and its device memory.
     */
    void createImage();
    /**
     * @brief creates a GPU image view object covering the image.
     */
    void createView();
    /**
     * @brief load data into the image; size is assumed based on `extent` field.
     * @param data_ptr pointer to the raw data to load onto the GPU.
     */
    void uploadData(void* data_ptr);
    /**
     * @brief copies data from a buffer into the image's internal memory.
     * @param buffer target buffer to copy data from.
     */
    void copyBufferToImage(Ref<Buffer> buffer) const;
    /**
     * @brief destroys all internal GPU resources (image, image view, device memory).
     */
    void destroyResources();
};

TO_STRING_DECL(Sampler::Filter);
TO_STRING_DECL(Sampler::Address);
TO_STRING_DECL(Texture::Format);
ENUM_OPERATOR(Texture::Usage);
TO_STRING_DECL(Texture::Usage);
TO_STRING_DECL(Texture::Layout);

} // namespace HopEngine

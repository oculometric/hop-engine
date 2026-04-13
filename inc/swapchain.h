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
#include "texture.h"

#include <glm/vec2.hpp>
#include <vector>

namespace HopEngine
{

/**
 * @brief handles creating and managing the swapchain. controls final-pass image format as well as Vsync
 * and number of frames-in-flight (used by descriptor sets and similar). queries device support for
 * appropriate features.
 */
class Swapchain final : public Destructible
{
public:
    /**
     * @brief describes the surface and swapchain support characteristics for a particular physical
     * device. populated by calling `Swapchain::getSwapchainSupportInfo`.
     */
    struct SupportInfo final
    {
        glm::u32vec2 current_extent;                 // current size of the surface
        glm::u32vec2 min_extent;                     // minimum size of swapchain images
        glm::u32vec2 max_extent;                     // maximum size of swapchain images
        uint32_t min_image_count;                    // minimum number of swapchain images
        uint32_t max_image_count;                    // maximum number of swapchain images
        uint32_t current_transform;                  // surface transform (flip/rotation)
        bool supports_premultiplied_alpha_composite; // `true` if the surface supports transparency
        bool supports_immediate_present;             // `true` if the surface supports disabling Vsync
    };

private:
    GPUHandle swapchain    = nullptr;                   // swapchain GPU handle
    Texture::Format format = Texture::FORMAT_SWAPCHAIN; // swapchain image format
    glm::u32vec2 extent    = { 1, 1 };                  // size of the swapchain images in pixels
    // whether the presentation of images is locked to screen refresh rate
    bool vsync_enabled = true;
    // image GPU handles extracted from the swapchain (created by the driver automatically)
    std::vector<GPUHandle> images;
    // image view GPU handles, one for each image
    std::vector<GPUHandle> image_views;
    // semaphore GPU handles for whether each image is usable for rendering
    std::vector<GPUHandle> image_available_semaphores;
    // semaphore GPU handles for whether each image has finished being used by the render commands
    std::vector<GPUHandle> render_finished_semaphores;
    // fence GPU handles for whether we can start recording commands for each image
    std::vector<GPUHandle> in_flight_fences;
    size_t frame_index = SIZE_MAX; // internal frame index counter

public:
    DELETE_CONSTRUCTORS(Swapchain);
    /**
     * @brief creates a new swapchain using the `RenderServer`'s surface with a given size.
     * @param new_extent size of the surface in pixels. may be overriden based on the surface
     * `SupportInfo`.
     */
    Swapchain(glm::u32vec2 new_extent);
    ~Swapchain() override;

    /**
     * @brief queries a physical device for the support capabilities/characteristics for
     * surfaces/swapchains created on it.
     * @param device GPU handle for the physical device to be queried
     * @returns struct containing information about what the device supports (see `SupportInfo`).
     */
    static SupportInfo getSwapchainSupportInfo(GPUHandle device);
    /**
     * @brief queries the swapchain support info and calculates a valid extent for the swapchain images.
     * @param extent preferred size of the images in pixels.
     * @returns actual valid size for swapchain images in pixels.
     */
    static glm::u32vec2 computeActualExtent(glm::u32vec2 extent);
    /**
     * @brief queries the swapchain support info and calculates the number of images which should be in
     * the swapchain.
     * @returns number of images to be created with the swapchain.
     */
    static uint32_t computeImageCount();

    /**
     * @brief fetches the number of images in the swapchain, equal to the number of frames-in-flight.
     * @returns number of images (and thus image views) in the swapchain.
     */
    uint32_t getImageCount() const { return static_cast<uint32_t>(image_views.size()); }
    /**
     * @brief fetches the GPU handle of a image view for the swapchain.
     * @param i index of the image view to fetch. should match whatever `image_index` value was returned
     * by `acquireNextImage`, and must be less than the value returned by `getImageCount`.
     * @returns image view GPU handle.
     */
    GPUHandle getImageView(size_t i) const { return image_views[i]; }
    Texture::Format getFormat() const { return format; }
    glm::u32vec2 getExtent() const { return extent; }

    /**
     * @brief waits and attempts to acquire an image from the swapchain, for which a command buffer can
     * then be recorded.
     * @returns image index of the acquire image, or `UINT32_MAX` if the image could not be acquired.
     */
    uint32_t acquireNextImage();
    /**
     * @brief submits a recorded command buffer for a given image index in the swapchain.
     * @param command_buffer draw command buffer which has had graphics commands recorded into it.
     * @param image_index index of the swapchain image for which the commands were recorded. must match
     * the value returned by `acquireNextImage`.
     * @returns `true` if the command buffer was submitted successfully, or `false` if an error
     * occurred.
     */
    bool submitCommands(WeakRef<DrawCommandBuffer> command_buffer, uint32_t image_index);
    /**
     * @brief updates the size of the swapchain images/image views. should be called when the window is
     * resized. requires recreating the swapchain.
     * @param new_extent new size for the swapchain images in pixels. may be overriden when the device
     * swapchain capabilities are queried.
     */
    void resize(glm::u32vec2 new_extent);
    /**
     * @brief enables/disables Vsync, which clamps the rate of rendering to the screen's refresh rate.
     * enabled by default. requires recreating the swapchain.
     * @param enabled `true` if the framerate should be locked to the screen's refresh rate, `false` if
     * the framerate should be uncapped.
     */
    void setVsync(bool enabled);
    bool getVsync() const { return vsync_enabled; }

private:
    /**
     * @brief creates the swapchain GPU handle itself.
     */
    void createSwapchain();
    /**
     * @brief extracts images from the swapchain and then creates an appropriate image view around each.
     */
    void createImageViews();
    /**
     * @brief creates semaphores and fences for synchronising correctly with the GPU.
     */
    void createSyncObjects();
    /**
     * @brief destroys all resources created by `createSwapchain`, `createImageViews` and
     * `createSyncObjects`.
     */
    void destroyResources();
};

/**
 * @brief handles creating a render pass, with configurable attachments. manages textures and
 * framebuffers accordingly.
 */
class RenderPass final : public Destructible
{
public:
    /**
     * @brief description of how the render pass attachments will be configured.
     */
    struct Config final
    {
        // number of extra attachments, usually 3 for offscreen passes
        size_t additional_attachments = 0;
        // whether a depth attachment is present (needed for most 3D passes)
        bool has_depth_attachment = true;
        // format of the main colour attachment
        Texture::Format main_colour_format = Texture::FORMAT_FLOAT_16X4;
    };

    /**
     * @brief encpasulates the clear values for a render pass.
     */
    struct ClearValues final
    {
        glm::vec3 colour = { 1, 0, 1 };     // value used for the main colour attachment
        bool transparent = false;           // whether the main colour attachment should be transparent
        std::vector<glm::vec4> additionals; // values used for additional attachments
        bool depth_present = true;          // whether the depth attachment is present
        float depth        = 1.0f;          // value used for depth attachment
    };

private:
    GPUHandle render_pass = nullptr;     // render pass GPU handle
    Config output_config;                // output attachment configuration used
    std::vector<GPUHandle> framebuffers; // framebuffer GPU handles, 1 or matched to the swapchain
    std::vector<Ref<Texture>> textures;  // images in use by the framebuffers
    glm::u32vec2 extent;                 // size of the images
    WeakRef<Swapchain> swapchain;        // optionally, swapchain which this render pass uses

public:
    DELETE_CONSTRUCTORS(RenderPass);
    /**
     * @brief constructs a render pass around a swapchain. instead of being created, images and views
     * will be retrieved from the swapchain (extra attachments and the depth attachment will still be
     * managed internally), and the `main_colour_format` of `config` is ignored. the swapchain's extent
     * determines the extent of the render pass. framebuffers are still created internally.
     * @param target_swapchain swapchain for which this render pass will be created.
     * @param config configuration for additional attachments to the render pass.
     */
    RenderPass(const WeakRef<Swapchain>& target_swapchain, const Config& config);
    /**
     * @brief constructs a render pass independent from the swapchain, based on an extent and attachment
     * configuration. all required images, views, and framebuffers are created and managed internally.
     * @param image_extent size of the render pass images in pixels.
     * @param config configuration for additional attachments to the render pass.
     */
    RenderPass(glm::u32vec2 image_extent, const Config& config);
    ~RenderPass() override;

    GPUHandle getRenderPass() const { return render_pass; }
    Config getOutputConfig() const { return output_config; }
    /**
     * @brief fetches a given image from the list of managed images. behaviour is determined by the
     * number of attachments and whether the render pass was constructed around a swapchain. images are
     * ordered like so: main colour attachment (required); additional colour attachments (zero or more);
     * depth attachment (optional)
     * @param attachment attachment index. if created around a swapchain, the main colour attachment is
     * absent, and the first attachment will be either an additional colour attachment or the depth
     * attachment.
     * @returns texture which backs the attachment, or `nullptr` if `attachment` was out of bounds.
     */
    Ref<Texture> getImage(size_t attachment) const;
    glm::u32vec2 getExtent() const { return extent; }
    /**
     * @brief checks if this render pass is compatible with rendering into another. used to check if a
     * material can be rendered in a different render pass to that it was created for. render passes are
     * considered compatible if their configurations are identical.
     * @param other other render pass to check against.
     * @returns `true` if the other render pass is compatible, otherwise `false`.
     */
    bool isCompatible(const WeakRef<RenderPass>& other) const;
    /**
     * @brief creates a new render pass with the same configuration, but unique internal resources.
     * @returns newly created independent render pass.
     */
    Ref<RenderPass> duplicate() const;
    /**
     * @brief recreates the framebuffers and images at a new specified size.
     * @param new_extent new size in pixels for images. ignored if the render pass was created around a
     * swapchain.
     */
    void resize(glm::u32vec2 new_extent = { 0, 0 });
    /**
     * @brief starts the render pass in the given command buffer, for draw commands to be issued into
     * it.
     * @param command_buffer command buffer into which rendering commands will be issued.
     * @param clear_colour colour to use for clearing the main colour buffer.
     * @param transparent if `true`, the main colour buffer will be cleared to transparency. does not
     * cause the window to be transparent unless the swapchain supports transparency.
     */
    void begin(WeakRef<DrawCommandBuffer> command_buffer, glm::vec3 clear_colour,
        bool transparent = false);

private:
    /**
     * @brief creates the render pass GPU handle.
     */
    void createRenderPass();
    /**
     * @brief creates images, image views, and framebuffers.
     */
    void createResources();
    /**
     * @brief destroys resources created by `createRenderPass` `createResources`.
     */
    void destroyResources();
};

} // namespace HopEngine

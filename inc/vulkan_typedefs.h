#pragma once

// this file provides sufficient type definitions to allow header
// files to NOT include the vulkan headers (only in the CPP files).
// that means that the headers can be used without the vulkan SDK
// being installed. in future this shouldn't be necessary as i'll
// eventually eliminate the backend-implementation-related stuff
// (like VkBuffer handles) from header files.

#define HANDLE_TYPE(t) struct t##_T;\
    typedef t##_T* t

struct VkVertexInputAttributeDescription;
struct VkVertexInputBindingDescription;
struct VkPipelineShaderStageCreateInfo;
struct VkSurfaceFormatKHR;
struct VkSwapchainCreateInfoKHR;
struct VkSurfaceCapabilitiesKHR;
struct VkSurfaceFormatKHR;

union VkClearValue;

HANDLE_TYPE(VkDescriptorSet);
HANDLE_TYPE(VkBuffer);
HANDLE_TYPE(VkDescriptorSetLayout);
HANDLE_TYPE(VkPipelineLayout);
HANDLE_TYPE(VkShaderModule);
HANDLE_TYPE(VkDeviceMemory);
HANDLE_TYPE(VkCommandBuffer);
HANDLE_TYPE(VkRenderPass);
HANDLE_TYPE(VkFramebuffer);
HANDLE_TYPE(VkSampler);
HANDLE_TYPE(VkSurfaceKHR);
HANDLE_TYPE(VkPhysicalDevice);
HANDLE_TYPE(VkSwapchainKHR);
HANDLE_TYPE(VkImageView);
HANDLE_TYPE(VkImage);
HANDLE_TYPE(VkPipeline);
HANDLE_TYPE(VkInstance);
HANDLE_TYPE(VkDevice);
HANDLE_TYPE(VkQueue);
HANDLE_TYPE(VkCommandPool);
HANDLE_TYPE(VkSemaphore);
HANDLE_TYPE(VkFence);
HANDLE_TYPE(VkQueryPool);
HANDLE_TYPE(VkDescriptorPool);
HANDLE_TYPE(VkDebugUtilsMessengerEXT);

typedef uint64_t VkDeviceSize;

#define VK_NULL_HANDLE nullptr

template <typename T, typename S, int L> T convertFlags(S usage, const T* mapping)
{
    uint32_t flags = 0;
    for (size_t i = 0; i < L; ++i)
    {
        if (usage & (1 << i))
            flags = mapping[i] | flags;
    }
    return static_cast<T>(flags);
}

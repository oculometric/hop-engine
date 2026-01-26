#pragma once

#define HANDLE_TYPE(t) struct t##_T;\
    typedef t##_T* t

struct VkVertexInputAttributeDescription;
struct VkVertexInputBindingDescription;
struct VkPipelineShaderStageCreateInfo;
struct VkSurfaceFormatKHR;
struct VkSwapchainCreateInfoKHR;

union VkClearValue;

enum VkFormat : int;
enum VkImageLayout : int;
enum VkPresentModeKHR : int;
enum VkPipelineStageFlagBits : int;

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
    T flags = (T)0;
    for (size_t i = 0; i < L; ++i)
    {
        if (usage & (1 << i))
            flags = static_cast<T>(static_cast<uint32_t>(mapping[i]) | static_cast<uint32_t>(flags));
    }
    return flags;
}

#define ENUM_OPERATOR(t) inline t operator|(t a, t b) { return (t)((uint32_t)a | (uint32_t)b); }

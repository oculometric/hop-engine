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
    uint32_t flags = 0;
    for (size_t i = 0; i < L; ++i)
    {
        if (usage & (1 << i))
            flags = mapping[i] | flags;
    }
    return static_cast<T>(flags);
}

#define TO_STRING_DEC(t) std::string to_string(t value)
#define VARGS(...) __VA_ARGS__
#define TO_STRING_DEF_BITFLAGS(t, s, n) string HopEngine::to_string(const t value) \
{ \
    constexpr const char* names[s] = \
    { n }; \
    string result; \
    for (size_t i = 0; i < s; ++i) \
    { \
        if (value & (1 << i)) \
        { \
            result += names[i]; \
            result += " | "; \
        } \
    } \
    result.pop_back(); \
    result.pop_back(); \
    result.pop_back(); \
    return result; \
}
#define TO_STRING_DEF(t, s, n) string HopEngine::to_string(const t value) \
{ \
    constexpr const char* names[s] = \
    { n }; \
    return names[value]; \
}

#define ENUM_OPERATOR(t) inline t operator|(t a, t b) { return (t)((uint32_t)a | (uint32_t)b); }

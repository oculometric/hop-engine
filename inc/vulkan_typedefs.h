#pragma once

#define HANDLE_TYPE(t) struct t##_T;\
    typedef t##_T* t

struct VkVertexInputAttributeDescription;
struct VkVertexInputBindingDescription;
struct VkPipelineShaderStageCreateInfo;
struct VkClearValue;

HANDLE_TYPE(VkDescriptorSet);
HANDLE_TYPE(VkBuffer);
HANDLE_TYPE(VkDescriptorSetLayout);
HANDLE_TYPE(VkPipelineLayout);
HANDLE_TYPE(VkShaderModule);
HANDLE_TYPE(VkDeviceMemory);
HANDLE_TYPE(VkCommandBuffer);
HANDLE_TYPE(VkRenderPass);
HANDLE_TYPE(VkFramebuffer);

enum VkFormat : int;
enum VkImageLayout : int;

typedef uint64_t VkDeviceSize;

#define VK_NULL_HANDLE nullptr
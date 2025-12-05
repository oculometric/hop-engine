#pragma once

#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

class CommandBuffer
{
private:
	VkCommandBuffer buffer = VK_NULL_HANDLE;
	bool already_submitted = false;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(CommandBuffer);

	CommandBuffer();
	~CommandBuffer();

	inline VkCommandBuffer getBuffer() { return buffer; }
	void submit();
};

}

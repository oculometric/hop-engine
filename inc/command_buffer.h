#pragma once

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

class CommandBuffer : public Destructible
{
private:
	VkCommandBuffer buffer = VK_NULL_HANDLE;
	bool already_submitted = false;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(CommandBuffer);

	VkCommandBuffer getBuffer() const { return buffer; }
	void submit();
	
	CommandBuffer();
	~CommandBuffer() override;
};

}

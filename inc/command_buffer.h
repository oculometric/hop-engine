#pragma once

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{
/**
 * @brief represents a transient GPU command buffer to allow immediate command execution
 */
class CommandBuffer : public Destructible
{
private:
	VkCommandBuffer buffer = VK_NULL_HANDLE;	// GPU command buffer handle
	bool already_submitted = false;				// whether the command buffer has been submitted

public:
	DELETE_NOT_ALL_CONSTRUCTORS(CommandBuffer);
	CommandBuffer();
	~CommandBuffer() override;
	
	VkCommandBuffer getBuffer() const { return buffer; }
	/**
	 * @brief causes the command buffer to be submitted to the graphics queue, and waits
	 * for completion before returning.
	 */
	void submit();
};

}

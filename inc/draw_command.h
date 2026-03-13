#pragma once

#include "common.h"

#include "mesh.h"
#include "material.h"
#include "uniform_block.h"

namespace HopEngine
{

struct DrawCommand final
{
	// material to be used in the draw command. contains the shader and material
	// uniforms (descriptor set 2) in use
	WeakRef<Material> material;
	WeakRef<Mesh> mesh; // mesh to be used in the draw command
	// instance-specific uniforms to be bound (descriptor set 1)
	WeakRef<UniformBlock> uniforms;
	int draw_priority = 0; // ordering bias to force objects to render early/late
	// draw mask determining which camera slots the draw command should render in
	uint32_t camera_mask = 0xFFFFFFFF;

	/**
	 * @brief comparator for sorting draw commands.
	 * @param a first draw command.
	 * @param b second draw command.
	 * @return \code true\endcode if \code a\endcode should be ordered before
	 * \code b\endcode, otherwise \code false\endcode.
	 */
	bool operator()(const DrawCommand& a, const DrawCommand& b) const;

	DrawCommand() = default;
	DrawCommand(const WeakRef<Material>& _material, const WeakRef<Mesh>& _mesh, const WeakRef<UniformBlock>& _uniforms = WeakRef<UniformBlock>())
		: material(_material), mesh(_mesh), uniforms(_uniforms) { }

	DrawCommand& priority(const int value) { draw_priority = value; return *this; }
	DrawCommand& mask(const uint32_t value) { camera_mask = value; return *this; }
};

}

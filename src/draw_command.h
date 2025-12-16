#pragma once

#include "common.h"

namespace HopEngine
{

struct DrawCommand
{
	WeakRef<Material> material;
	WeakRef<Mesh> mesh;
	WeakRef<UniformBlock> uniforms;
	int draw_priority = 0;
	uint32_t camera_mask = 0xFFFFFFFF;

	bool operator()(const DrawCommand& a, const DrawCommand& b) const;

	inline DrawCommand(WeakRef<Material> _material, WeakRef<Mesh> _mesh, WeakRef<UniformBlock> _uniforms = WeakRef<UniformBlock>(nullptr))
		: material(_material), mesh(_mesh), uniforms(_uniforms) { }

	inline DrawCommand() { }

	inline DrawCommand priority(int value) { draw_priority = value; return *this; }
	inline DrawCommand mask(uint32_t value) { camera_mask = value; return *this; }
};

}

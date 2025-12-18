#include <imgui.h>
#include <map>
#include <string>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include "engine.h"
#include "object.h"
#include "scene.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"
#include "input.h"
#include "uniform_block.h"
#include "sampler.h"

using namespace HopEngine;
using namespace std;

static WeakRef<Object> selected_object;
static WeakRef<Material> selected_material;
static Camera* selected_camera;

void drawImGuiDebug(float delta_time)
{
	if (selected_object)
	{
		ImGui::Begin("object", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		selected_object->drawImGuiDebug();
		if (ImGui::Button("close"))
			selected_object = nullptr;
		ImGui::End();
	}

	if (selected_material)
	{
		ImGui::Begin("material", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		selected_material->drawImGuiDebug();
		if (ImGui::Button("close"))
			selected_material = nullptr;
		ImGui::End();
	}

	if (Engine::getScene())
	{
		ImGui::Begin("scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		Engine::getScene()->drawImGuiDebug();
		ImGui::End();
	}

	Engine::drawImGuiDebug();
}

void debugCamera(float delta_time)
{
	if (!selected_camera)
		return;

	glm::vec2 mouse_delta = Input::getMouseDelta() * 0.25f;
	if (Input::isMouseDown(GLFW_MOUSE_BUTTON_2))
		selected_camera->transform.rotateLocal({ -mouse_delta.y, 0, -mouse_delta.x });

	glm::mat4 camera_matrix = selected_camera->transform.getMatrix();
	glm::vec3 local_move_vector = glm::vec3{
		Input::getAxis('A', 'D'),
		Input::getAxis('Q', 'E'),
		Input::getAxis('W', 'S')
	} * delta_time * 1.5f;
	if (Input::isKeyDown(GLFW_KEY_LEFT_SHIFT))
		local_move_vector *= 3.0f;
	selected_camera->transform.translateLocal(camera_matrix * glm::vec4(local_move_vector, 0));
}

void debugClearSelection(WeakRef<Object> object, WeakRef<Material> material)
{
	selected_object = object;
	selected_material = material;
	if (Engine::getScene())
		selected_camera = Engine::getScene()->getCamera(0).get();
	else
		selected_camera = nullptr;
}

Ref<Texture> texturePicker(Ref<Texture> current, const char* str)
{
	auto options = Engine::getAllRefs<Texture>();
	options.push_back(WeakRef<Texture>(nullptr));
	int selected = 0;
	while (selected < (options.size() - 1) && options[selected] != current)
		++selected;
	string options_str;
	for (auto opt : options)
	{
		if (opt == nullptr)
			options_str += PTR(0);
		else
		{
			string origin = opt->getOrigin();
			if (origin.empty())
				options_str += PTR(opt.get());
			else
				options_str += origin;
		}
		options_str.resize(options_str.size() + 1);
	}
	ImGui::Combo(str, &selected, options_str.c_str());
	return options[selected];
}

Ref<Mesh> meshPicker(Ref<Mesh> current, const char* str)
{
	auto options = Engine::getAllRefs<Mesh>();
	options.push_back(WeakRef<Mesh>(nullptr));
	int selected = 0;
	while (selected < (options.size() - 1) && options[selected] != current)
		++selected;
	string options_str;
	for (auto opt : options)
	{
		if (opt == nullptr)
			options_str += PTR(0);
		else
		{
			string origin = opt->getOrigin();
			if (origin.empty())
				options_str += PTR(opt.get());
			else
				options_str += origin;
		}
		options_str.resize(options_str.size() + 1);
	}
	ImGui::Combo(str, &selected, options_str.c_str());
	return options[selected];
}

Ref<Material> materialPicker(Ref<Material> current, const char* str)
{
	auto options = Engine::getAllRefs<Material>();
	options.push_back(WeakRef<Material>(nullptr));
	int selected = 0;
	while (selected < (options.size() - 1) && options[selected] != current)
		++selected;
	string options_str;
	for (auto opt : options)
	{
		if (opt == nullptr)
			options_str += PTR(0);
		else
		{
			string origin;
			if (origin.empty())
				options_str += PTR(opt.get());
			else
				options_str += origin;
		}
		options_str.resize(options_str.size() + 1);
	}
	ImGui::Combo(str, &selected, options_str.c_str());
	return options[selected];
}

void Object::drawImGuiDebug()
{
	char name_space[128] = { 0 };
	memcpy(name_space, name.c_str(), name.size());
	if (ImGui::InputText("name", name_space, 128))
		name = name_space;
	ImGui::LabelText("id", "%s", PTR(this).c_str());
	if (ImGui::CollapsingHeader("local transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		glm::vec3 vec = transform.getLocalPosition();
		ImGui::DragFloat3("position", (float*)&vec, 0.02f);
		transform.setLocalPosition(vec);
		vec = transform.getLocalEuler();
		ImGui::DragFloat3("euler", (float*)&vec, 0.5f);
		transform.setLocalEuler(vec);
		vec = transform.getLocalScale();
		ImGui::DragFloat3("scale", (float*)&vec, 0.05f);
		transform.setLocalScale(vec);
	}
}

void Camera::drawImGuiDebug()
{
	Object::drawImGuiDebug();
	if (ImGui::CollapsingHeader("camera params", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("near clip", &near_clip, 0.1f, 0.001f, far_clip, "%.3f", ImGuiSliderFlags_Logarithmic);
		ImGui::DragFloat("far clip", &far_clip, 1.0f, near_clip, 1000.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
		ImGui::DragFloat("fov", &fov, 1.0f, 1.0f, 179.0f);
		ImGui::ColorEdit3("clear colour", (float*)&clear_colour);
		if (ImGui::Button("take control"))
			selected_camera = this;
	}
}

void StaticMesh::drawImGuiDebug()
{
	Object::drawImGuiDebug();
	if (ImGui::CollapsingHeader("static mesh params", ImGuiTreeNodeFlags_DefaultOpen))
	{
		mesh = meshPicker(mesh, "mesh data");
		if (mesh)
		{
			ImGui::LabelText("vertices", "%i", mesh->getVertexCount());
			ImGui::LabelText("triangles", "%i", mesh->getIndexCount() / 3);
			ImGui::LabelText("vertex size", "%i", sizeof(Vertex));
			ImGui::LabelText("vertex attributes", "%i", mesh->getAttributeDescriptions().size());
		}
		material = materialPicker(material, "material");
		if (material)
		{
			if (ImGui::Button("edit material"))
				selected_material = material;
		}
		if (ImGui::CollapsingHeader("camera mask"))
		{
			ImGui::InputScalar("bitflags", ImGuiDataType_U32, &camera_mask, nullptr, nullptr, nullptr, ImGuiInputTextFlags_CharsDecimal);
			ImGui::BeginTable("bitflag_table", 8);
			size_t i = 0;
			for (int y = 0; y < 4; ++y)
			{
				ImGui::TableNextRow();
				for (int x = 0; x < 8; ++x)
				{
					ImGui::TableSetColumnIndex(x);
					bool v = camera_mask & (1 << i);
					ImGui::Checkbox(("##xx" + to_string(i)).c_str(), &v);
					camera_mask &= ~(1 << i);
					if (v)
						camera_mask |= (1 << i);
					++i;
				}
			}
			ImGui::EndTable();
		}
	}
}

void Light::drawImGuiDebug()
{
	Object::drawImGuiDebug();
	if (ImGui::CollapsingHeader("light params", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const char* types = "POINT\0SPOT\0DIRECTIONAL";
		ImGui::Combo("type", (int*)&type, types);
		ImGui::ColorEdit3("colour", (float*)&colour);
		if (type == SPOT)
		{
			spot_angle *= glm::pi<float>() / 180.0f;
			ImGui::SliderAngle("spot angle", &spot_angle, 1.0f, 89.0f);
			spot_angle /= glm::pi<float>() / 180.0f;
		}
	}
}

void drawImGuiSceneTreeItem(multimap<Object*, WeakRef<Object>>& parent_map, WeakRef<Object> parent)
{
	auto range = parent_map.equal_range(parent.get());
	while (range.first != range.second)
	{
		WeakRef<Object> child = range.first->second;
		
		if (ImGui::TreeNode((child->name + " - " + PTR(child.get())).c_str()))
		{
			if (ImGui::Button("select"))
				selected_object = child;
			drawImGuiSceneTreeItem(parent_map, child);
			ImGui::TreePop();
		}
		++range.first;
	}
}

void Scene::drawImGuiDebug()
{
	ImGui::ColorEdit3("ambient light", (float*)&(ambient_colour));
	int current_skybox_index = 0;
	const char* skybox_items = "tex 1\0tex 2\0tex 3";
	skybox = texturePicker(skybox, "skybox");
	ImGui::LabelText("total objects", "%d", objects.size());
	multimap<Object*, WeakRef<Object>> parent_map;
	for (auto object : objects)
		parent_map.insert({ object->getParent().get(), object});
	parent_map.insert({ (nullptr), root });

	drawImGuiSceneTreeItem(parent_map, WeakRef<Object>(nullptr));
}

void Material::drawImGuiDebug()
{
	ImGui::LabelText("shader", "%s (%s)", shader->getOrigin().c_str(), PTR(shader.get()));
	if (ImGui::CollapsingHeader("pipeline config", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto config = pipeline->getConfig();
		ImGui::LabelText("culling", "%s", vk::to_string((vk::CullModeFlags)config.culling_mode));
		ImGui::LabelText("polygon", "%s", vk::to_string((vk::PolygonMode)config.polygon_mode));
		ImGui::LabelText("depth write", "%s", config.depth_write_enable ? "true" : "false");
		ImGui::LabelText("depth test", "%s", config.depth_test_enable ? "true" : "false");
		ImGui::LabelText("depth operation", "%s", vk::to_string((vk::CompareOp)config.depth_compare_op));
	}
	uniforms->drawImGuiDebug(texture_name_to_binding);
	// TODO: reload shader button
	ImGui::Button("reload shader");
}

void UniformBlock::drawImGuiDebug(map<string, uint32_t> texture_name_to_binding)
{
	map<uint32_t, string> binding_to_texture_name;
	for (const auto& pair : texture_name_to_binding)
		binding_to_texture_name[pair.second] = pair.first;

	if (ImGui::CollapsingHeader("texture bindings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const auto& pair : textures_in_use)
		{
			ImGui::PushID(pair.first);
			ImGui::LabelText("binding", "%i", pair.first);
			ImGui::LabelText("name", "%s", binding_to_texture_name[pair.first]);
			auto result = texturePicker(pair.second.first, "texture");
			if (result != pair.second.first)
				setTexture(pair.first, result);
			// TODO: change sampler as well!
			ImGui::LabelText("filter", "%s", vk::to_string((vk::Filter)pair.second.second->getBuilder().filtering_mode));
			ImGui::LabelText("address", "%s", vk::to_string((vk::SamplerAddressMode)pair.second.second->getBuilder().address_mode));
			ImGui::PopID();
		}
	}
	if (ImGui::CollapsingHeader("uniform variables", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const auto& block : layout.bindings)
		{
			if (block.type != UNIFORM)
				continue;
			ImGui::LabelText("binding", "%i", block.binding);
			ImGui::LabelText("block name", "%s", block.name);
			ImGui::LabelText("block size", "%i", block.buffer_size);
			ImGui::BeginTable("uniforms", 3, ImGuiTableFlags_Borders);
			ImGui::TableSetupColumn("name");
			ImGui::TableSetupColumn("size");
			ImGui::TableSetupColumn("offset");
			ImGui::TableHeadersRow();
			for (const auto& var : block.variables)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", var.name);
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%i", var.size);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%i", var.offset);
			}
			ImGui::EndTable();
		}
		// TODO: variables be modifiable
	}
}

void Engine::_drawImGuiDebug()
{
	ImGui::Begin("performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::LabelText("delta time", "%fms", last_frame_stats.delta_time * 1000.0f);
	ImGui::LabelText("smoothed FPS", "%f", smoothed_fps);
	if (ImGui::CollapsingHeader("time details", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::LabelText("render imgui", "%fms", last_frame_stats.imgui_time * 1000.0f);
		ImGui::LabelText("build buffers", "%fms", last_frame_stats.build_time * 1000.0f);
		ImGui::LabelText("record commands", "%fms", last_frame_stats.record_time * 1000.0f);
		ImGui::LabelText("render time", "%fms", last_frame_stats.render_time * 1000.0f); // TODO: find out how long it takes to render the image command buffer on the gpu
		ImGui::LabelText("update scene", "%fms", last_frame_stats.update_time * 1000.0f);
	}
	if (ImGui::CollapsingHeader("scene stats", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::LabelText("draw calls", "%i", last_frame_stats.draw_calls);
		ImGui::LabelText("pipeline rebinds", "%i", last_frame_stats.pipeline_rebinds);
		ImGui::LabelText("triangles", "%i", last_frame_stats.triangles);
		ImGui::LabelText("vertices", "%i", last_frame_stats.vertices);
		ImGui::LabelText("render passes", "%i", last_frame_stats.passes);
		ImGui::LabelText("camera rendering", "%i", last_frame_stats.cameras);
		ImGui::LabelText("lights rendering", "%i", last_frame_stats.lights);
	}
	ImGui::Spacing();
	ImGui::PlotLines("##xx", delta_time_history, 512, history_offset, "delta time", 0.0001f, 0.2f, ImVec2{0, 160}, 4);
	//ImGui::PlotLines("##xxx", fps_history, 512, history_offset, "FPS", 10.0f, 200.0f, ImVec2{ 0, 160 }, 4);
	ImGui::End();
}

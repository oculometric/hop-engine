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
#include "render_graph.h"
#include "graphics_environment.h"

// TODO: be able to load resources at runtime

using namespace HopEngine;
using namespace std;

static WeakRef<Object> selected_object;
static WeakRef<Material> selected_material;
static Camera* selected_camera;

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

void collectDescendents(multimap<Object*, WeakRef<Object>>& parent_map, Object* parent, vector<Object*>& descendents)
{
	auto range = parent_map.equal_range(parent);
	while (range.first != range.second)
	{
		descendents.push_back(range.first->second.get());
		collectDescendents(parent_map, range.first->second.get(), descendents);
		range.first++;
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

	if (ImGui::CollapsingHeader("object heirarchy", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		drawImGuiSceneTreeItem(parent_map, WeakRef<Object>(nullptr));
	ImGui::Text("selected: %s", selected_object ? (selected_object->name + " - " + PTR(selected_object.get())).c_str() : "none");
	bool disabled = !selected_object;
	if (disabled)
		ImGui::BeginDisabled();
	if (selected_object != root && ImGui::Button("remove from tree"))
	{
		vector<Object*> objects_to_remove;
		objects_to_remove.push_back(selected_object.get());
		collectDescendents(parent_map, selected_object.get(), objects_to_remove);
		for (Object* obj : objects_to_remove)
		{ // TODO: improve scene tree so that objects know about their children, can be removed
			auto it = objects.begin();
			while (it != objects.end())
			{
				if (it->get() == obj)
				{
					objects.erase(it);
					break;
				}
				++it;
			}
		}
		selected_object->setParent(WeakRef<Object>());
		selected_object = nullptr;
	}
	if (selected_object != root && ImGui::Button("reparent"))
	{
		ImGui::OpenPopup("new parent");
	}
	if (selected_object != root && ImGui::BeginPopup("new parent", ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (ImGui::SmallButton((root->name + " - " + PTR(root.get())).c_str()))
		{
			selected_object->setParent(root);
			ImGui::CloseCurrentPopup();
		}
		for (size_t i = 0; i < objects.size(); ++i)
		{
			const auto& object = objects[i];
			if (object == selected_object)
				continue;
			if (ImGui::SmallButton((object->name + " - " + PTR(object.get())).c_str()))
			{
				selected_object->setParent(object);
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::EndPopup();
	}
	if (ImGui::Button("add child"))
	{
		ImGui::OpenPopup("child type");
	}
	if (ImGui::BeginPopup("child type", ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (ImGui::Button("object"))
		{
			insertObject<Object>(new Object())->setParent(selected_object);
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::Button("static mesh"))
		{
			insertObject<StaticMesh>(new StaticMesh(RenderServer::getQuad(), RenderServer::getDefaultMaterial()))->setParent(selected_object);
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::Button("light"))
		{
			insertObject<Light>(new Light(Light::DIRECTIONAL))->setParent(selected_object);
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	if (disabled)
		ImGui::EndDisabled();
}

void Material::drawImGuiDebug()
{
	ImGui::LabelText("shader", "%s (%s)", shader->getOrigin().c_str(), PTR(shader.get()).c_str());
	if (ImGui::CollapsingHeader("pipeline config", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto config = pipeline->getConfig();
		ImGui::LabelText("culling", "%s", vk::to_string((vk::CullModeFlags)config.culling_mode).c_str());
		ImGui::LabelText("polygon", "%s", vk::to_string((vk::PolygonMode)config.polygon_mode).c_str());
		ImGui::LabelText("depth write", "%s", config.depth_write_enable ? "true" : "false");
		ImGui::LabelText("depth test", "%s", config.depth_test_enable ? "true" : "false");
		ImGui::LabelText("depth operation", "%s", vk::to_string((vk::CompareOp)config.depth_compare_op).c_str());
	}
	uniforms->drawImGuiDebug(texture_name_to_binding);
	// TODO: reload shader button
	ImGui::Button("reload shader");
}

bool Sampler::drawImGuiDebug()
{
	ImGui::LabelText("filter", "%s", vk::to_string((vk::Filter)builder.filtering_mode).c_str());
	VkFilter new_mode = builder.filtering_mode;
	if (ImGui::Button("switch filtering"))
		new_mode = (VkFilter)((new_mode + 1) % 2);
	ImGui::LabelText("address", "%s", vk::to_string((vk::SamplerAddressMode)builder.address_mode).c_str());
	VkSamplerAddressMode new_address = builder.address_mode;
	if (ImGui::Button("switch addressing"))
		new_address = (VkSamplerAddressMode)((new_address + 1) % 5);
	if (new_mode != builder.filtering_mode || new_address != builder.address_mode)
	{
		vkDestroySampler(RenderServer::getDevice(), sampler, nullptr);
		builder.filtering_mode = new_mode;
		builder.address_mode = new_address;
		VkSamplerCreateInfo create_info{ };
		create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		create_info.magFilter = builder.filtering_mode;
		create_info.minFilter = builder.filtering_mode;
		create_info.addressModeU = builder.address_mode;
		create_info.addressModeV = builder.address_mode;
		create_info.addressModeW = builder.address_mode;
		VkPhysicalDeviceProperties properties{ };
		vkGetPhysicalDeviceProperties(RenderServer::getPhysicalDevice(), &properties);
		create_info.anisotropyEnable = VK_TRUE;
		create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		create_info.unnormalizedCoordinates = VK_FALSE;
		create_info.compareEnable = VK_FALSE;
		create_info.compareOp = VK_COMPARE_OP_ALWAYS;
		create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		create_info.mipLodBias = 0.0f;
		create_info.minLod = 0.0f;
		create_info.maxLod = 0.0f;
		if (vkCreateSampler(RenderServer::getDevice(), &create_info, nullptr, &sampler) != VK_SUCCESS)
			DBG_FAULT("vkCreateSampler failed");
		return true;
	}
	return false;
}

void UniformBlock::drawImGuiDebug(const map<string, uint32_t>& texture_name_to_binding)
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
			ImGui::LabelText("name", "%s", binding_to_texture_name[pair.first].c_str());
			auto result = texturePicker(pair.second.first, "texture");
			if (result != pair.second.first)
				setTexture(pair.first, result);
			if (pair.second.second->drawImGuiDebug())
			{
				setSampler(pair.first, nullptr);
				setSampler(pair.first, pair.second.second);
			}
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
			ImGui::LabelText("block name", "%s", block.name.c_str());
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
				ImGui::Text("%s", var.name.c_str());
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

void Engine::_drawImGuiDebug(float delta_time)
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
		Engine::getScene()->render_graph->drawImGuiDebug();
		ImGui::End();
	}

	ImGui::Begin("performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::LabelText("delta time", "%fms", last_frame_stats.delta_time * 1000.0f);
	ImGui::LabelText("smoothed FPS", "%f", smoothed_fps);
	if (ImGui::CollapsingHeader("time details", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::LabelText("render imgui", "%fms", last_frame_stats.imgui_time * 1000.0f);
		ImGui::LabelText("build buffers", "%fms", last_frame_stats.build_time * 1000.0f);
		ImGui::LabelText("record commands", "%fms", last_frame_stats.record_time * 1000.0f);
		ImGui::LabelText("render time", "%fms", last_frame_stats.render_time * 1000.0f);
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

void Engine::debugCamera(float delta_time)
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

void Engine::debugClearSelection(WeakRef<Object> object, WeakRef<Material> material)
{
	selected_object = object;
	selected_material = material;
	if (Engine::getScene())
		selected_camera = Engine::getScene()->getCamera(0).get();
	else
		selected_camera = nullptr;
}

WeakRef<Object> Engine::getDebugSelection()
{
	return selected_object;
}

void RenderGraph::drawImGuiDebug()
{
	if (ImGui::CollapsingHeader("render graph", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::InputInt("show step", &output_step, 1, 1);
		ImGui::SliderInt("show attachment", &output_image, 0, 4);

		static int pass_details_index = 0;
		ImGui::BeginTable("passes", 7, ImGuiTableFlags_Borders);
		ImGui::TableSetupColumn("type");
		ImGui::TableSetupColumn("slot");
		ImGui::TableSetupColumn("material");
		ImGui::TableSetupColumn("inputs");
		ImGui::TableSetupColumn("outputs");
		ImGui::TableSetupColumn("extent");
		ImGui::TableSetupColumn("info");
		ImGui::TableHeadersRow();
		int current_pass = 0;
		for (const auto& pass : execution_steps)
		{
			ImGui::PushID(current_pass);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text(pass.is_camera ? "camera" : "post-process");
			ImGui::TableSetColumnIndex(1);
			if (pass.is_camera)
				ImGui::Text("%i", pass.camera_slot);
			ImGui::TableSetColumnIndex(2);
			if (!pass.is_camera)
				ImGui::Text("%s", PTR(pass.material.get()).c_str());
			ImGui::TableSetColumnIndex(3);
			if (!pass.is_camera)
				ImGui::Text("%i", pass.texture_bindings.size());
			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%i", pass.render_pass->getClearValues().size());
			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%i x %i", pass.render_pass->getExtent().width, pass.render_pass->getExtent().height);
			ImGui::TableSetColumnIndex(6);
			if (ImGui::SmallButton(">"))
				pass_details_index = current_pass;
			++current_pass;
			ImGui::PopID();
		}
		ImGui::EndTable();
		if (pass_details_index < execution_steps.size())
		{
			if (ImGui::CollapsingHeader(("pass " + to_string(pass_details_index) + " details").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				const auto& pass = execution_steps[pass_details_index];
				if (!pass.is_camera)
				{
					ImGui::Text("input texture bindings:");
					ImGui::BeginTable("bindings", 5, ImGuiTableFlags_Borders);
					ImGui::TableSetupColumn("binding");
					ImGui::TableSetupColumn("pass");
					ImGui::TableSetupColumn("attachment");
					ImGui::TableSetupColumn("filtering");
					ImGui::TableSetupColumn("addressing");
					ImGui::TableHeadersRow();
					for (const auto& pair : pass.texture_bindings)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("%i", pair.first);
						ImGui::TableSetColumnIndex(1);
						ImGui::Text("%i", pair.second.step_index);
						ImGui::TableSetColumnIndex(2);
						ImGui::Text("%i", pair.second.output_index);
						ImGui::TableSetColumnIndex(3);
						ImGui::Text("%s", vk::to_string((vk::Filter)(pair.second.filter_mode)));
						ImGui::TableSetColumnIndex(4);
						ImGui::Text("%s", vk::to_string((vk::SamplerAddressMode)(pair.second.address_mode)));
					}
					ImGui::EndTable();
					ImGui::Separator();
				}
				if (pass.resolution_scale > 0)
					ImGui::LabelText("resolution scale", "%.2f", pass.resolution_scale);
				else
					ImGui::LabelText("custom resolution", "%i x %i", pass.custom_extent.width, pass.custom_extent.height);
				ImGui::Separator();
				ImGui::Text("output attachments:");
				ImGui::LabelText("colour", "always present");
				ImGui::LabelText("extras", "%i", pass.render_pass->getOutputConfig().additional_attachments);
				ImGui::LabelText("depth", pass.render_pass->getOutputConfig().has_depth_attachment ? "present" : "disabled");
			}
		}
	}
}

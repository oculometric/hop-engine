#include <imgui.h>
#include <imgui_internal.h>
#include <map>
#include <string>
#include <vulkan/vulkan.hpp>

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
#include "package.h"
#if !defined(STANDALONE)
#include "main.h"
#endif
#include "text_block.h"

using namespace HopEngine;
using namespace std;

static WeakRef<Object> selected_object;
static WeakRef<Material> selected_material;
static Camera* selected_camera;

static Ref<Texture> texturePicker(const Ref<Texture>& current, const char* str)
{
	auto options = Engine::getAllRefs<Texture>();
	options.emplace_back(nullptr);
	int selected = 0;
	while (static_cast<size_t>(selected) < (options.size() - 1) && options[selected] != current)
		++selected;
	string options_str;
	for (auto opt : options)
	{
		options_str += opt->getOrigin();
		options_str.resize(options_str.size() + 1);
	}
	ImGui::Combo(str, &selected, options_str.c_str());
	return options[selected].strong();
}

static Ref<Mesh> meshPicker(const Ref<Mesh>& current, const char* str)
{
	auto options = Engine::getAllRefs<Mesh>();
	options.emplace_back(nullptr);
	int selected = 0;
	while (static_cast<size_t>(selected) < (options.size() - 1) && options[selected] != current)
		++selected;
	string options_str;
	for (auto opt : options)
	{
		options_str += opt->getOrigin();
		options_str.resize(options_str.size() + 1);
	}
	ImGui::Combo(str, &selected, options_str.c_str());
	return options[selected].strong();
}

static Ref<Material> materialPicker(const Ref<Material>& current, const char* str)
{
	auto options = Engine::getAllRefs<Material>();
	options.emplace_back(nullptr);
	int selected = 0;
	while (static_cast<size_t>(selected) < (options.size() - 1) && options[selected] != current)
		++selected;
	string options_str;
	for (auto opt : options)
	{
		options_str += opt->getOrigin();
		options_str.resize(options_str.size() + 1);
	}
	ImGui::Combo(str, &selected, options_str.c_str());
	return options[selected].strong();
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
		ImGui::DragFloat3("position", reinterpret_cast<float*>(&vec), 0.02f);
		transform.setLocalPosition(vec);
		vec = transform.getLocalEuler();
		ImGui::DragFloat3("euler", reinterpret_cast<float*>(&vec), 0.5f);
		transform.setLocalEuler(vec);
		vec = transform.getLocalScale();
		ImGui::DragFloat3("scale", reinterpret_cast<float*>(&vec), 0.05f);
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
		ImGui::ColorEdit3("clear colour", reinterpret_cast<float*>(&clear_colour));
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
			ImGui::LabelText("vertices", "%zu", mesh->getVertexCount());
			ImGui::LabelText("triangles", "%zu", mesh->getIndexCount() / 3);
			ImGui::LabelText("vertex size", "%llu", sizeof(Vertex));
			ImGui::LabelText("vertex attributes", "%zu", Mesh::getAttributeDescriptions().size());
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
					ImGui::Checkbox(("##xx" + ::to_string(i)).c_str(), &v);
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

void TextBlock::drawImGuiDebug()
{
	Object::drawImGuiDebug();
	if (ImGui::CollapsingHeader("text block params", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static char tmp[513] = { };
		memcpy(tmp, text.data(), std::min(512ull, text.size()));
		tmp[std::min(512ull, text.size())] = '\0';
		ImGui::InputText("text", tmp, 513);
		text = tmp;
		ImGui::ColorEdit3("tint", reinterpret_cast<float*>(&tint));
		
		updateGeometry();
	}
}

void Light::drawImGuiDebug()
{
	Object::drawImGuiDebug();
	if (ImGui::CollapsingHeader("light params", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const char* types = "POINT\0SPOT\0DIRECTIONAL";
		ImGui::Combo("type", reinterpret_cast<int*>(&type), types);
		ImGui::ColorEdit3("colour", reinterpret_cast<float*>(&colour));
		if (type == SPOT)
		{
			spot_angle *= glm::pi<float>() / 180.0f;
			ImGui::SliderAngle("spot angle", &spot_angle, 1.0f, 89.0f);
			spot_angle /= glm::pi<float>() / 180.0f;
		}
	}
}

static void drawImGuiSceneTreeItem(multimap<Object*, WeakRef<Object>>& parent_map, WeakRef<Object> parent)
{
	auto [range_start, range_end] = parent_map.equal_range(parent.get());
	while (range_start != range_end)
	{
		WeakRef<Object> child = range_start->second;
		
		if (ImGui::TreeNode((child->name + " - " + PTR(child.get())).c_str()))
		{
			if (ImGui::Button("select"))
				selected_object = child;
			drawImGuiSceneTreeItem(parent_map, child);
			ImGui::TreePop();
		}
		++range_start;
	}
}

static void collectDescendents(multimap<Object*, WeakRef<Object>>& parent_map, Object* parent, vector<Object*>& descendents)
{
	auto [range_start, range_end] = parent_map.equal_range(parent);
	while (range_start != range_end)
	{
		descendents.push_back(range_start->second.get());
		collectDescendents(parent_map, range_start->second.get(), descendents);
		++range_start;
	}
}

void Scene::drawImGuiDebug()
{
	ImGui::LabelText("scene", "%s", getOrigin().c_str());
	ImGui::ColorEdit3("ambient light", reinterpret_cast<float*>(&(ambient_colour)));
	skybox = texturePicker(skybox, "skybox");
	ImGui::LabelText("total objects", "%zu", objects.size());
	multimap<Object*, WeakRef<Object>> parent_map;
	for (auto object : objects)
		parent_map.insert({ object->getParent().get(), object});
	parent_map.insert({ (nullptr), root });

	if (ImGui::CollapsingHeader("object heirarchy", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		drawImGuiSceneTreeItem(parent_map, WeakRef<Object>(nullptr));
	ImGui::Text("selected: %s", selected_object ? (selected_object->name + " - " + PTR(selected_object.get())).c_str() : "none");
	const bool disabled = !selected_object;
	if (disabled)
		ImGui::BeginDisabled();
	if (selected_object != root && ImGui::Button("remove from tree"))
	{
		vector<Object*> objects_to_remove;
		objects_to_remove.push_back(selected_object.get());
		collectDescendents(parent_map, selected_object.get(), objects_to_remove);
		for (const Object* obj : objects_to_remove)
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
		for (const auto& object : objects)
		{
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
		ImGui::LabelText("culling", "%s", to_string(config.culling_mode).c_str());
		ImGui::LabelText("polygon", "%s", to_string(config.polygon_mode).c_str());
		ImGui::LabelText("depth write", "%s", config.depth_write_enable ? "true" : "false");
		ImGui::LabelText("depth test", "%s", config.depth_test_enable ? "true" : "false");
		ImGui::LabelText("depth operation", "%s", to_string(config.depth_compare_op).c_str());
	}
	uniforms->drawImGuiDebug(texture_name_to_binding);
	// TODO: reload shader button
	ImGui::Button("reload shader");
}

bool Sampler::drawImGuiDebug()
{
	ImGui::PushID(this);
	static std::string filter_names[2] = 
	{
		"NEAREST",
		"LINEAR"
	};
	static std::string address_names[3] = 
	{
		"REPEAT",
		"MIRRORED",
		"CLAMP TO EDGE"
	};
	ImGui::LabelText("filter", "%s", filter_names[builder.filtering_mode].c_str());
	ImGui::LabelText("address", "%s", address_names[builder.address_mode].c_str());
	ImGui::PopID();
	return false;
}

void UniformBlock::drawImGuiDebug(const map<string, uint32_t>& texture_name_to_binding)
{
	map<uint32_t, string> binding_to_texture_name;
	for (const auto& pair : texture_name_to_binding)
		binding_to_texture_name[pair.second] = pair.first;

	if (ImGui::CollapsingHeader("texture bindings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const auto& [tex_id, tex_bind] : textures_in_use)
		{
			ImGui::PushID(static_cast<int>(tex_id));
			ImGui::LabelText("binding", "%u", tex_id);
			ImGui::LabelText("name", "%s", binding_to_texture_name[tex_id].c_str());
			auto result = texturePicker(tex_bind.texture, "texture");
			if (result != tex_bind.texture)
				setTexture(tex_id, result);
			if (tex_bind.sampler->drawImGuiDebug())
			{
				setSampler(tex_id, nullptr);
				setSampler(tex_id, tex_bind.sampler);
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
			ImGui::LabelText("binding", "%u", block.binding);
			ImGui::LabelText("block name", "%s", block.name.c_str());
			ImGui::LabelText("block size", "%llu", block.buffer_size);
			ImGui::BeginTable("uniforms", 3, ImGuiTableFlags_Borders);
			ImGui::TableSetupColumn("name");
			ImGui::TableSetupColumn("size");
			ImGui::TableSetupColumn("offset");
			ImGui::TableHeadersRow();
			for (const auto& [va_name, var_size, var_off] : block.variables)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", va_name.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%zu", var_size);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%zu", var_off);
			}
			ImGui::EndTable();
		}
		// TODO: variables be modifiable
	}
}

void Engine::debugCamera(float delta_time)
{
	if (!selected_camera)
		return;

	glm::vec2 mouse_delta = { 0, 0 };
	mouse_delta += glm::vec2{ Input::getGamepadAxis(Input::GAMEPAD_RX), Input::getGamepadAxis(Input::GAMEPAD_RY) } * delta_time * 160.0f;
	static bool mouse_down = false;
	if (Input::isMouseDown(Input::MOUSE_RIGHT))
	{
		mouse_delta += Input::getMouseDelta() * 0.25f;
		if (!mouse_down)
			Input::setCursorVisible(false);
		mouse_down = true;
	}
	else if (mouse_down)
	{
		Input::setCursorVisible(true);
		mouse_down = false;		
	}
	
	selected_camera->transform.rotate({ 0, 0, -mouse_delta.x });
	selected_camera->transform.rotateLocal({ -mouse_delta.y, 0, 0 });

	const glm::mat4 camera_matrix = selected_camera->transform.getMatrix();
	glm::vec3 local_move_vector = glm::vec3{
		                              Input::getAxis('A', 'D') + Input::getGamepadAxis(Input::GAMEPAD_LX),
		                              Input::getAxis('Q', 'E') + Input::getGamepadAxis(Input::GAMEPAD_BUTTONS),
		                              Input::getAxis('W', 'S') + Input::getGamepadAxis(Input::GAMEPAD_LY)
	                              } * delta_time * 1.5f;
	if (Input::isKeyDown(Input::KEY_LEFT_SHIFT) || Input::isGamepadButtonDown(Input::GAMEPAD_B))
		local_move_vector *= 3.0f;
	selected_camera->transform.translateLocal(local_move_vector);
}

void Engine::debugSelect(const WeakRef<Object>& object)
{
	selected_object = object;
}

void Engine::debugClearSelection(const WeakRef<Object>& object, const WeakRef<Material>& material, WeakRef<Camera> camera)
{
	selected_object = object;
	selected_material = material;
	selected_camera = camera.get();
}

WeakRef<Object> Engine::getDebugSelection()
{
	return selected_object;
}

void Engine::_drawImGuiDebug(float delta_time) const
{
	static bool show_imgui = true;
	static unsigned int align_windows = 3;
	
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("file"))
	{
#if !defined(STANDALONE)
		if (ImGui::BeginMenu("open scene"))
		{
			if (ImGui::MenuItem("bunnygirl"))
			{
				const auto scn = getAshaScene();
				debugClearSelection();
				Engine::setup(scn.init_func, scn.update_func, scn.imgui_func);
			}
			if (ImGui::MenuItem("museum"))
			{
				const auto scn = getMuseumScene();
				debugClearSelection();
				Engine::setup(scn.init_func, scn.update_func, scn.imgui_func);
			}
			ImGui::EndMenu();
		}
#endif
		if (ImGui::MenuItem("quit"))
			stop();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("view"))
	{
		ImGui::Checkbox("show/hide ImGui windows", &show_imgui);
		if (ImGui::MenuItem("arrange ImGui windows"))
			align_windows = 1;
		if (ImGui::MenuItem("toggle wireframe"))
			Engine::setForceWireframe(!Engine::isWireframeMode());
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();	
	
	if (!show_imgui)
		return;
	
	float last_window_height = 0;
	if (Engine::getScene())
	{
		if (align_windows)
			ImGui::SetNextWindowPos({ 10, 30 });
		ImGui::Begin("scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		Engine::getScene()->drawImGuiDebug();
		if (ImGui::CollapsingHeader("render graph", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			Engine::getScene()->render_graph->drawImGuiDebug();
		last_window_height = ImGui::GetWindowHeight();
		ImGui::End();
	}
	
	if (selected_object)
	{
		if (align_windows)
			ImGui::SetNextWindowPos({ 10, last_window_height + 40 });
		ImGui::Begin("object", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		selected_object->drawImGuiDebug();
		if (ImGui::Button("close"))
			selected_object = nullptr;
		ImGui::End();
	}
	
	float rightmost_window_width = 0;
	{
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
			ImGui::LabelText("draw calls", "%zu", last_frame_stats.draw_calls);
			ImGui::LabelText("pipeline rebinds", "%zu", last_frame_stats.pipeline_rebinds);
			ImGui::LabelText("triangles", "%zu", last_frame_stats.triangles);
			ImGui::LabelText("vertices", "%zu", last_frame_stats.vertices);
			ImGui::LabelText("render passes", "%zu", last_frame_stats.passes);
			if (ImGui::CollapsingHeader("pass durations"))
			{
				int i = 1;
				for (const float dur : last_frame_stats.pass_times)
					ImGui::LabelText(("pass " + ::to_string(i++)).c_str(), "%fms", dur * 1000.0f);
			}
			ImGui::LabelText("camera rendering", "%zu", last_frame_stats.cameras);
			ImGui::LabelText("lights rendering", "%zu", last_frame_stats.lights);
		}
		ImGui::Spacing();
		ImGui::PlotLines("##xx", delta_time_history, 512, history_offset, "delta time", 0.0001f, 0.2f, ImVec2{0, 160}, 4);
		const auto size = ImGui::GetWindowSize();
		if (align_windows)
			ImGui::SetWindowPos({ ImGui::GetIO().DisplaySize.x - size.x - 10.0f, 30 });
		last_window_height = size.y;
		rightmost_window_width = max(rightmost_window_width, size.x);
		ImGui::End();
	}
	
	{
		ImGui::Begin("resources", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::LabelText("total references", "%zu", allocated_refs.size());
		ImGui::Text("the categories below only show\nresources loaded with Engine::loadXXX(),\nnot internal resources");
		if (ImGui::CollapsingHeader(("materials (" + ::to_string(loaded_materials.size()) + " loaded)").c_str()))
		{
			for (const auto& [res_name, res] : loaded_materials)
			{
				ImGui::LabelText(("(" + ::to_string(res.getCount()) + " users)").c_str(), "%s", res_name.c_str());
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", res_name.c_str());
			}
		}
		if (ImGui::CollapsingHeader(("meshes (" + ::to_string(loaded_meshes.size()) + " loaded)").c_str()))
		{
			for (const auto& [res_name, ref] : loaded_meshes)
			{
				ImGui::LabelText(("(" + ::to_string(ref.getCount()) + " users)").c_str(), "%s", res_name.c_str());
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", res_name.c_str());
			}
		}
		if (ImGui::CollapsingHeader(("textures (" + ::to_string(loaded_textures.size()) + " loaded)").c_str()))
		{
			for (const auto& [res_name, ref] : loaded_textures)
			{
				ImGui::LabelText(("(" + ::to_string(ref.getCount()) + " users)").c_str(), "%s", res_name.c_str());
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", res_name.c_str());
			}
		}
		if (ImGui::CollapsingHeader(("shaders (" + ::to_string(getEngine()->loaded_shaders.size()) + " loaded)").c_str()))
		{
			for (const auto& [res_name, ref] : getEngine()->loaded_shaders)
			{
				ImGui::LabelText(("(" + ::to_string(ref.getCount()) + " users)").c_str(), "%s", res_name.c_str());
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", res_name.c_str());
			}
		}
		if (ImGui::Button("prune loaded resources"))
			ImGui::OpenPopup("prune_are_you_sure");
		static size_t pruned_count = 0;
		if (ImGui::BeginPopup("prune_are_you_sure"))
		{
			ImGui::Text("are you sure you want to unload all unused resources?");
			if (ImGui::Button("continue"))
			{
				pruned_count = Engine::pruneUnusedResources();
				ImGui::CloseCurrentPopup();
				ImGui::OpenPopup("pruned_done");
			}
			if (ImGui::Button("cancel"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopup("pruned_done"))
		{
			ImGui::Text("pruned %zu resources", pruned_count);
			ImGui::EndPopup();
		}
	
		if (ImGui::Button("load resource"))
			ImGui::OpenPopup("load_resource");
		if (ImGui::BeginPopup("load_resource"))
		{
			const auto entries = Package::listLoadedEntries();
			ImGui::BeginTable("resources_table", 1);
			for (const auto& entry : entries)
			{
				string extension;
				const size_t extension_start = entry.find_last_of('.');
				if (extension_start != string::npos)
					extension = entry.substr(extension_start + 1);
				int type = -1;
				string type_string;
				if (extension == "obj")
				{ type = 0; type_string = "mesh"; }
				else if (extension == "png")
				{ type = 1; type_string = "texture"; }
				else if (extension == "hmat")
				{ type = 2; type_string = "material"; }
				else if (extension == "vert" || extension == "frag")
				{ type = 3; type_string = "shader"; }
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", entry.c_str());
				if (ImGui::IsItemHovered() && type != -1)
					ImGui::SetTooltip("loads as %s", type_string.c_str());
				if (ImGui::IsItemClicked() && type != -1)
				{
					switch (type)
					{
					case 0: Engine::loadMesh("res://" + entry); break;
					case 1: Engine::loadTexture("res://" + entry); break;
					case 2: Engine::loadMaterial("res://" + entry); break;
					case 3: Engine::loadShader("res://" + entry.substr(0, extension_start)); break;
					default: break;
					}
					ImGui::CloseCurrentPopup();				
				}
			}
			ImGui::EndTable();
			ImGui::EndPopup();
		}
		const auto size = ImGui::GetWindowSize();
		if (align_windows)
			ImGui::SetWindowPos({ ImGui::GetIO().DisplaySize.x - size.x - 10.0f, last_window_height + 40 });
		rightmost_window_width = max(rightmost_window_width, size.x);
		ImGui::End();
	}

	if (selected_material)
	{
		ImGui::Begin("material", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		selected_material->drawImGuiDebug();
		if (ImGui::Button("close"))
			selected_material = nullptr;
		const auto size = ImGui::GetWindowSize();
		if (align_windows)
			ImGui::SetWindowPos({ ImGui::GetIO().DisplaySize.x - size.x - rightmost_window_width - 20.0f, 30 });
		ImGui::End();
	}
	
	if (align_windows)
		--align_windows;
}

void RenderGraph::drawImGuiDebug()
{
	ImGui::LabelText("render graph", "%s", getOrigin().c_str());
	ImGui::InputInt("show step", &output_step, 1, 1);
	ImGui::SliderInt("show attachment", &output_image, 0, 5);

	ImGui::BeginTable("passes", 4, ImGuiTableFlags_Borders);
	ImGui::TableSetupColumn("name");
	ImGui::TableSetupColumn("type");
	ImGui::TableSetupColumn("extent");
	ImGui::TableSetupColumn("enabled");
	ImGui::TableHeadersRow();
	int current_pass = 0;
	for (auto& pass : execution_steps)
	{
		ImGui::PushID(current_pass);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("%s", pass.name.c_str());
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%s", pass.is_camera ? "camera" : "post-process");		
		ImGui::TableSetColumnIndex(2);
		ImGui::Text("%u x %u", pass.render_pass->getExtent().x, pass.render_pass->getExtent().y);
		ImGui::TableSetColumnIndex(3);
		bool b = !pass.skipped;
		if (ImGui::Checkbox("", &b))
			setSkipStep(current_pass, !b);
		++current_pass;
		ImGui::PopID();
	}
	const int hovered = ImGui::TableGetHoveredRow();
	ImGui::EndTable();
	if (ImGui::IsItemHovered() && hovered > 0)
	{
		ImGui::BeginTooltip();
		const int pass_details_index = hovered - 1;
		const auto& pass = execution_steps[pass_details_index];
		if (!pass.is_camera)
		{
			ImGui::LabelText("shader", "%s", pass.material->getShader()->getOrigin().c_str());
			ImGui::Text("input texture bindings:");
			ImGui::BeginTable("bindings", 5, ImGuiTableFlags_Borders);
			ImGui::TableSetupColumn("binding");
			ImGui::TableSetupColumn("pass");
			ImGui::TableSetupColumn("attachment");
			ImGui::TableSetupColumn("filtering");
			ImGui::TableSetupColumn("addressing");
			ImGui::TableHeadersRow();
			for (const auto& [tex_id, bind] : pass.texture_bindings)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%u", tex_id);
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%zu", bind.step_index);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%zu", bind.output_index);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%s", to_string(bind.filter_mode).c_str());
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%s", to_string(bind.address_mode).c_str());
			}
			ImGui::EndTable();
			ImGui::Separator();
		}
		else
			ImGui::LabelText("camera slot", "%zu", pass.camera_slot);
		if (pass.resolution_scale > 0)
			ImGui::LabelText("resolution scale", "%.2f", pass.resolution_scale);
		else
			ImGui::LabelText("custom resolution", "%u x %u", pass.custom_extent.x, pass.custom_extent.y);
		ImGui::Separator();
		ImGui::Text("output attachments:");
		ImGui::LabelText("colour", "always present");
		ImGui::LabelText("extras", "%zu", pass.render_pass->getOutputConfig().additional_attachments);
		ImGui::LabelText("depth", pass.render_pass->getOutputConfig().has_depth_attachment ? "present" : "disabled");
		ImGui::EndTooltip();
	}
	ImGui::Text("hover over a render step for more info");
}

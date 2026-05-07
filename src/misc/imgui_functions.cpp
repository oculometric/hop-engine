#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <map>
#include <string>

#include "engine.h"
#include "basic_components.h"
#include "scene.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"
#include "input.h"
#include "render_graph.h"
#include "render_server.h"
#include "package.h"
#include "node_view.h"

using namespace HopEngine;
using namespace std;

static WeakRef<Object> selected_object;
static WeakRef<Material> selected_material;

static WeakRef<Texture> texturePicker(const WeakRef<Texture>& current, const char* str)
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

static WeakRef<Mesh> meshPicker(const WeakRef<Mesh>& current, const char* str)
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

static WeakRef<Material> materialPicker(const WeakRef<Material>& current, const char* str)
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
	for (const auto& comp : components)
	{
		ImGui::PushID(comp.get());
		comp->drawImGuiDebug();
		ImGui::PopID();
	}
}

void Component::drawImGuiDebug()
{
	if (ImGui::CollapsingHeader("component", ImGuiTreeNodeFlags_DefaultOpen))
	{ }
}

void CameraComponent::drawImGuiDebug()
{
	if (ImGui::CollapsingHeader("camera component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("near clip", &near_clip, 0.1f, 0.001f, far_clip, "%.3f", ImGuiSliderFlags_Logarithmic);
		ImGui::DragFloat("far clip", &far_clip, 1.0f, near_clip, 1000.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
		ImGui::DragFloat("fov", &fov, 1.0f, 1.0f, 179.0f);
		ImGui::ColorEdit3("clear colour", reinterpret_cast<float*>(&clear_colour));
	}
}

void StaticMeshComponent::drawImGuiDebug()
{
	if (ImGui::CollapsingHeader("static mesh component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		mesh = meshPicker(mesh, "mesh data").strong();
		if (mesh)
		{
			ImGui::LabelText("vertices", "%zu", mesh->getVertexCount());
			ImGui::LabelText("triangles", "%zu", mesh->getIndexCount() / 3);
			ImGui::LabelText("vertex size", "%llu", static_cast<unsigned long long>(sizeof(Mesh::Vertex)));
		}
		material = materialPicker(material, "material").strong();
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

void TextComponent::drawImGuiDebug()
{
	if (ImGui::CollapsingHeader("text block component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static char tmp[513] = { };
		memcpy(tmp, text.data(), std::min(static_cast<size_t>(512), static_cast<size_t>(text.size())));
		tmp[std::min(static_cast<size_t>(512), static_cast<size_t>(text.size()))] = '\0';
		ImGui::InputText("text", tmp, 513);
		text = tmp;
		ImGui::ColorEdit3("tint", reinterpret_cast<float*>(&tint));
		
		updateGeometry();
	}
}

void LightComponent::drawImGuiDebug()
{
	if (ImGui::CollapsingHeader("light component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const char* types = "POINT\0SPOT\0DIRECTIONAL\0";
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

static void drawImGuiSceneTreeItem(WeakRef<Object> parent)
{
	for (size_t i = 0; i < parent->getChildCount(); ++i)
	{
		WeakRef<Object> child = parent->getChild(i);
		
		if (ImGui::TreeNode((child->name + " - " + PTR(child.get())).c_str()))
		{
			if (ImGui::Button("select"))
				selected_object = child;
			drawImGuiSceneTreeItem(child);
			ImGui::TreePop();
		}
	}
}

void Scene::drawImGuiDebug()
{
	ImGui::LabelText("scene", "%s", getOrigin().c_str());
	ImGui::ColorEdit3("ambient light", reinterpret_cast<float*>(&(ambient_colour)));
	//sky = new Sky(texturePicker(sky ? sky->get, "skybox"));
	ImGui::LabelText("total objects", "%zu", objects.size());

	if (ImGui::CollapsingHeader("object heirarchy", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		drawImGuiSceneTreeItem(root);
	ImGui::Text("selected: %s", selected_object ? (selected_object->name + " - " + PTR(selected_object.get())).c_str() : "none");
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
			auto result = texturePicker(std::get<0>(tex_bind), "texture");
			if (result != std::get<0>(tex_bind))
				setTexture(tex_id, result.strong());
			ImGui::PopID();
		}
	}
	if (ImGui::CollapsingHeader("uniform variables", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const auto& block : layout.bindings)
		{
			if (block.type != Shader::UNIFORM)
				continue;
			ImGui::LabelText("binding", "%u", block.binding);
			ImGui::LabelText("block name", "%s", block.name.c_str());
			ImGui::LabelText("block size", "%llu", static_cast<unsigned long long>(block.buffer_size));
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

void Engine::debugCamera(const WeakRef<Object>& selected_camera)
{
	if (!selected_camera)
		return;

	glm::vec2 mouse_delta = { 0, 0 };
	mouse_delta += glm::vec2{ Input::getGamepadAxis(Input::GAMEPAD_RX), Input::getGamepadAxis(Input::GAMEPAD_RY) } * Engine::getDeltaTime() * 160.0f;
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
	
	selected_camera->getTransform().rotate({ 0, 0, -mouse_delta.x });
	selected_camera->getTransform().rotateLocal({ -mouse_delta.y, 0, 0 });

	const glm::mat4 camera_matrix = selected_camera->getTransform().getMatrix();
	glm::vec3 local_move_vector = glm::vec3{
		                              Input::getAxis('A', 'D') + Input::getGamepadAxis(Input::GAMEPAD_LX),
		                              Input::getAxis('Q', 'E') + Input::getGamepadAxis(Input::GAMEPAD_BUMPERS),
		                              Input::getAxis('W', 'S') + Input::getGamepadAxis(Input::GAMEPAD_LY)
	                              } * Engine::getDeltaTime() * 1.5f;
	if (Input::isKeyDown(Input::KEY_LEFT_SHIFT) || Input::isGamepadButtonDown(Input::GAMEPAD_B))
		local_move_vector *= 3.0f;
	selected_camera->getTransform().translateLocal(local_move_vector);
}

void Engine::debugSelect(const WeakRef<Object>& object)
{
	selected_object = object;
}

WeakRef<Object> Engine::getDebugSelection()
{
	return selected_object;
}

void Engine::_drawImGuiDebug() const
{
	static bool show_imgui = true;
	static unsigned int align_windows = 3;
	
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("file"))
	{
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
		if (ImGui::MenuItem("toggle V-sync"))
			RenderServer::setVsyncEnabled(!RenderServer::getVsyncEnabled());
		if (ImGui::MenuItem("toggle fullscreen"))
			RenderServer::setFullscreenEnabled(!RenderServer::getFullscreenEnabled());
        if (ImGui::MenuItem("toggle overlay logs"))
            RenderServer::setOverlayLogs(!RenderServer::getOverlayLogs());
		ImGui::EndMenu();
	}

	string signature_text = format("hop-engine @ {:>5.1f}fps, {:>6.2f}ms", smoothed_fps, smoothed_delta_time * 1000.0f).c_str();
	ImGui::TextAligned(1.0f, -FLT_MIN, signature_text.c_str());
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
		ImGui::LabelText("delta time", "%fms", smoothed_delta_time * 1000.0f);
		ImGui::LabelText("FPS", "%f", smoothed_fps);
		if (ImGui::CollapsingHeader("time details", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::LabelText("record commands", "%fms", last_frame_stats.record_time * 1000.0f);
			ImGui::LabelText("render time", "%fms", last_frame_stats.render_time * 1000.0f);
			ImGui::LabelText("update scene", "%fms", last_frame_stats.update_time * 1000.0f);
		}
		if (ImGui::CollapsingHeader("scene stats", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::LabelText("draw calls", "%zu", last_frame_stats.draw_calls);
			ImGui::LabelText("pipeline rebinds", "%zu", last_frame_stats.pipeline_rebinds);
			ImGui::LabelText("triangles", "%zu", last_frame_stats.triangles);
			ImGui::LabelText("render passes", "%zu", last_frame_stats.passes);
			if (ImGui::CollapsingHeader("pass durations"))
			{
				int i = 1;
				for (const float dur : last_frame_stats.pass_times)
					ImGui::LabelText(("pass " + ::to_string(i++)).c_str(), "%fms", dur * 1000.0f);
			}
			ImGui::LabelText("camera rendering", "%zu", last_frame_stats.cameras);
		}
		ImGui::Spacing();
		ImGui::PlotLines("##xx", delta_time_history, 200, history_offset, "delta time", 0.01f, 100.0f, ImVec2{200, 80}, 4);
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
		ImGui::Text("%u x %u", pass.framebuffer->getExtent().x, pass.framebuffer->getExtent().y);
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
				ImGui::Text("%s", tex_id.c_str());
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
		ImGui::LabelText("extras", "%zu", pass.framebuffer->getConfig().additional_attachments);
		ImGui::LabelText("depth", pass.framebuffer->getConfig().has_depth_attachment ? "present" : "disabled");
		ImGui::EndTooltip();
	}
	ImGui::Text("hover over a render step for more info");
}

void NodeView::drawImGuiDebug()
{
    ImGui::Begin("style controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::CollapsingHeader("header", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::InputInt("header_align", &style->header_align, 1, 1);
            ImGui::Checkbox("header_at_top", &style->header_at_top);
            ImGui::Checkbox("header_fill", &style->header_fill);
            ImGui::InputInt("after_header_spacing", &style->after_header_spacing, 1, 1);
            ImGui::Spacing();
        }

        if (ImGui::CollapsingHeader("text", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat2("text_offset", (float*)&style->text_offset, -10.0f, 10.0f);
            ImGui::ColorEdit3("text_colour", (float*)&style->text_colour);
            ImGui::SliderFloat("text_spacing", &style->text_spacing, -2.0f, 4.0f);
            ImGui::Spacing();
        }

        if (ImGui::CollapsingHeader("outline", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Combo("outline_style", (int*)&style->outline_style, "HIDDEN\0PRESET_COLOUR\0NODE_COLOUR\0MODULATE_NODE_COLOUR\0");
            ImGui::ColorEdit3("outline_colour", (float*)&style->outline_colour);
            ImGui::ColorEdit3("outline_colour_highlight", (float*)&style->outline_colour_highlight);
            ImGui::SliderFloat("outline_colour_mult", &style->outline_colour_mult, 0.0f, 1.0f);
            ImGui::Spacing();
        }

        if (ImGui::CollapsingHeader("fill", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("fill_modulate_colour", &style->fill_modulate_colour);
            ImGui::ColorEdit3("fill_colour", (float*)&style->fill_colour);
            ImGui::SliderFloat("fill_colour_mult", &style->fill_colour_mult, 0.0f, 1.0f);
            ImGui::Spacing();
        }

        if (ImGui::CollapsingHeader("background", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit3("background_colour", (float*)&style->background_colour);
            ImGui::Checkbox("show_grid", &style->show_grid);
            ImGui::InputInt("grid_scale", &style->grid_scale, 1);
            ImGui::ColorEdit3("grid_colour", (float*)&style->grid_colour);
            ImGui::SliderFloat("grid_dots_modulate", &style->grid_dots_modulate, 0.0f, 10.0f);
            ImGui::Spacing();
        }

        if (ImGui::CollapsingHeader("elements", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::InputFloat("pin_offset", &style->pin_offset, 1.0f, 1.0f);
            ImGui::Checkbox("reverse_element_order", &style->reverse_element_order);
            ImGui::Checkbox("center_text_elements", &style->center_text_elements);
            ImGui::InputInt("after_elements_spacing", &style->after_elements_spacing, 1, 1);
            ImGui::Spacing();
        }

        ImGui::Checkbox("shadows", &style->shadows);
        ImGui::SliderFloat2("shadow_offset", (float*)&style->shadow_offset, -12, 12);
        ImGui::ColorEdit3("shadow_colour", (float*)&style->shadow_colour);

        ImGui::Spacing();
        if (ImGui::Button("update style"))
            setStyle(style);

        ImGui::End();
    }
}

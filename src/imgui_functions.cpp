#include <imgui.h>
#include <map>

#include "engine.h"
#include "object.h"
#include "scene.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"
#include "input.h"

using namespace HopEngine;
using namespace std;

static WeakRef<Object> selected_object;
static WeakRef<Material> selected_material;
static Camera* selected_camera;

void drawImGuiDebug()
{
	if (selected_object)
	{
		ImGui::Begin("selected object", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		selected_object->drawImGuiDebug();
		if (ImGui::Button("close"))
			selected_object = nullptr;
		ImGui::End();
	}

	if (selected_material)
	{
		ImGui::Begin("selected material", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		//selected_material->drawImGuiDebug();
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

void debugClearSelection()
{
	selected_object = nullptr;
	selected_material = nullptr;
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
	ImGui::LabelText("draws", "%d", getDrawCommands().size());
	multimap<Object*, WeakRef<Object>> parent_map;
	for (auto object : objects)
		parent_map.insert({ object->getParent().get(), object});
	parent_map.insert({ (nullptr), root });

	drawImGuiSceneTreeItem(parent_map, WeakRef<Object>(nullptr));
}
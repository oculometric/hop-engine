#include <imgui.h>
#include <map>

#include "engine.h"
#include "object.h"
#include "scene.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"

using namespace HopEngine;
using namespace std;

Ref<Texture> texturePicker(Ref<Texture> current, const char* str)
{
	auto options = Engine::getAllRefs<Texture>();
	int selected = 0;
	while (selected < options.size() && options[selected] != current)
		++selected;
	string options_str;
	for (auto opt : options)
	{
		string origin = opt->getOrigin();
		if (origin.empty())
			options_str += PTR(opt.get());
		else
			options_str += origin;
		options_str.resize(options_str.size() + 1);
	}
	ImGui::Combo(str, &selected, options_str.c_str());
	return options[selected];
}

Ref<Mesh> meshPicker(Ref<Mesh> current, const char* str)
{
	auto options = Engine::getAllRefs<Mesh>();
	int selected = 0;
	while (selected < options.size() && options[selected] != current)
		++selected;
	string options_str;
	for (auto opt : options)
	{
		string origin = opt->getOrigin();
		if (origin.empty())
			options_str += PTR(opt.get());
		else
			options_str += origin;
		options_str.resize(options_str.size() + 1);
	}
	ImGui::Combo(str, &selected, options_str.c_str());
	return options[selected];
}

Ref<Material> materialPicker(Ref<Material> current, const char* str)
{
	auto options = Engine::getAllRefs<Material>();
	int selected = 0;
	while (selected < options.size() && options[selected] != current)
		++selected;
	string options_str;
	for (auto opt : options)
	{
		string origin;
		if (origin.empty())
			options_str += PTR(opt.get());
		else
			options_str += origin;
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
	}
}

void StaticMesh::drawImGuiDebug()
{
	Object::drawImGuiDebug();
	if (ImGui::CollapsingHeader("mesh params", ImGuiTreeNodeFlags_DefaultOpen))
	{
		mesh = meshPicker(mesh, "mesh data");
		ImGui::LabelText("vertices", "%i", mesh->getVertexCount());
		ImGui::LabelText("triangles", "%i", mesh->getIndexCount() / 3);
		ImGui::LabelText("vertex size", "%i", sizeof(Vertex));
		ImGui::LabelText("vertex attributes", "%i", mesh->getAttributeDescriptions().size());

		material = materialPicker(material, "material");
		ImGui::Button("edit material");
		// TODO: jump to editing this material!
		// TODO: jump to viewing object when clicked in heirarchy (track)
	}
}

void drawImGuiSceneTreeItem(multimap<Object*, Object*>& parent_map, Object* parent)
{
	auto range = parent_map.equal_range(parent);
	while (range.first != range.second)
	{
		Object* child = range.first->second;
		if (ImGui::TreeNode((child->name + " - " + PTR(child)).c_str()))
		{
			drawImGuiSceneTreeItem(parent_map, child);
			ImGui::TreePop();
		}
		++range.first;
	}
}

void Scene::drawImGuiDebug()
{
	ImGui::Begin("scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::ColorEdit3("ambient light", (float*)&(ambient_colour));
	int current_skybox_index = 0;
	const char* skybox_items = "tex 1\0tex 2\0tex 3";
	skybox = texturePicker(skybox, "skybox");
	ImGui::LabelText("total objects", "%d", objects.size());
	ImGui::LabelText("draws", "%d", getDrawCommands().size());
	multimap<Object*, Object*> parent_map;
	for (auto object : objects)
		parent_map.insert({ object->getParent().get(), object.get() });
	parent_map.insert({ nullptr, root.get() });

	drawImGuiSceneTreeItem(parent_map, nullptr);

	ImGui::End();
}
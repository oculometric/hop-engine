#include "shader.h"

#include <map>
#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.hpp>
#include <spirv_reflect/spirv_reflect.h>
#include <filesystem>

#include "command_buffer.h"
#include "render_server.h"
#include "package.h"

using namespace HopEngine;
using namespace std;

Shader::Shader(const string& base_path)
{
	origin = base_path;
	vector<uint32_t> vert_blob;
	vector<uint32_t> frag_blob;
	if (!compileShaders(base_path, vert_blob, frag_blob))
	{
		DBG_ERROR(base_path + " shader compilation failed");
		if (!compileShaders("res://engine/shaders/default_shader.glsl", vert_blob, frag_blob))
			DBG_FAULT("failed to load default shader!");
	}

	vert_module = createShaderModule(vert_blob);
	frag_module = createShaderModule(frag_blob);

	const auto vert_bindings = getReflectedBindings(vert_blob);
	const auto frag_bindings = getReflectedBindings(frag_blob);

	bindings = mergeBindings(vert_bindings, frag_bindings);

	vector<VkDescriptorSetLayoutBinding> layout_bindings;
	for (const DescriptorBinding& binding : bindings)
	{
		VkDescriptorSetLayoutBinding layout_binding{ };
		layout_binding.binding = binding.binding;
		layout_binding.descriptorType = (binding.type == UNIFORM) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layout_binding.descriptorCount = 1;
		layout_binding.pImmutableSamplers = nullptr;
		layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layout_bindings.push_back(layout_binding);
	}

	VkDescriptorSetLayoutCreateInfo set_layout_create_info{ };
	set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_layout_create_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
	set_layout_create_info.pBindings = layout_bindings.data();

	if (vkCreateDescriptorSetLayout(RenderServer::getDevice(), &set_layout_create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
		DBG_FAULT("vkCreateDescriptorSetLayout failed");

	VkPipelineLayoutCreateInfo layout_create_info{ };
	layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_create_info.setLayoutCount = 3;
	const VkDescriptorSetLayout layouts[3] =
	{
		RenderServer::getSceneDescriptorSetLayout(),
		RenderServer::getObjectDescriptorSetLayout(),
		descriptor_set_layout
	};
	layout_create_info.pSetLayouts = layouts;

	if (vkCreatePipelineLayout(RenderServer::getDevice(), &layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
		DBG_FAULT("vkCreatePipelineLayout failed");

	DBG_VERBOSE("created shader from " + base_path);
}

Shader::~Shader()
{
	DBG_VERBOSE("destroyed shader '" + getOrigin() + '\'');

    RenderServer::waitIdle();
	vkDestroyPipelineLayout(RenderServer::getDevice(), pipeline_layout, nullptr);
	vkDestroyDescriptorSetLayout(RenderServer::getDevice(), descriptor_set_layout, nullptr);

	vkDestroyShaderModule(RenderServer::getDevice(), vert_module, nullptr);
	vkDestroyShaderModule(RenderServer::getDevice(), frag_module, nullptr);
}

ShaderLayout Shader::getShaderLayout() const
{
	return { descriptor_set_layout, bindings };
}

void Shader::bind(Ref<DrawCommandBuffer> command_buffer)
{
	command_buffer->bindPipelineLayoutInternal(pipeline_layout);
}

vector<VkPipelineShaderStageCreateInfo> Shader::getShaderStageCreateInfos() const
{
	VkPipelineShaderStageCreateInfo vert_stage_create_info{ };
	vert_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_stage_create_info.module = vert_module;
	vert_stage_create_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_stage_create_info{ };
	frag_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_stage_create_info.module = frag_module;
	frag_stage_create_info.pName = "main";

	return { vert_stage_create_info, frag_stage_create_info };
}

bool Shader::reloadShader()
{
	RenderServer::waitIdle();

	// string proper_path = origin;
	// if (!precompiled)
	// {
	// 	proper_path = Package::getTempPath() + "temp_shader_compiled";
	// 	if (!compileShaders(origin, proper_path))
	// 	{
	// 		DBG_ERROR(origin + " shader compilation failed");
	// 		return false;
	// 	}
	// }
	//
	// auto vert_blob = Package::tryLoadFile(proper_path + "_vert.spv");
	// auto frag_blob = Package::tryLoadFile(proper_path + "_frag.spv");
	//
	// vkDestroyShaderModule(RenderServer::getDevice(), vert_module, nullptr);
	// vert_module = createShaderModule(vert_blob);
	// vkDestroyShaderModule(RenderServer::getDevice(), frag_module, nullptr);
	// frag_module = createShaderModule(frag_blob);
	//
	// auto vert_bindings = getReflectedBindings(vert_blob);
	// auto frag_bindings = getReflectedBindings(frag_blob);
	//
	// bindings = mergeBindings(vert_bindings, frag_bindings);
	//
	// vector<VkDescriptorSetLayoutBinding> layout_bindings;
	// for (const DescriptorBinding& binding : bindings)
	// {
	// 	VkDescriptorSetLayoutBinding layout_binding{ };
	// 	layout_binding.binding = binding.binding;
	// 	layout_binding.descriptorType = (binding.type == UNIFORM) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// 	layout_binding.descriptorCount = 1;
	// 	layout_binding.pImmutableSamplers = nullptr;
	// 	layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	// 	layout_bindings.push_back(layout_binding);
	// }
	//
	// VkDescriptorSetLayoutCreateInfo set_layout_create_info{ };
	// set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	// set_layout_create_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
	// set_layout_create_info.pBindings = layout_bindings.data();
	//
	// if (vkCreateDescriptorSetLayout(RenderServer::getDevice(), &set_layout_create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
	// 	DBG_FAULT("vkCreateDescriptorSetLayout failed");
	//
	// VkPipelineLayoutCreateInfo layout_create_info{ };
	// layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	// layout_create_info.setLayoutCount = 3;
	// VkDescriptorSetLayout layouts[3] =
	// {
	// 	RenderServer::getSceneDescriptorSetLayout(),
	// 	RenderServer::getObjectDescriptorSetLayout(),
	// 	descriptor_set_layout
	// };
	// layout_create_info.pSetLayouts = layouts;
	//
	// if (vkCreatePipelineLayout(RenderServer::getDevice(), &layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
	// 	DBG_FAULT("vkCreatePipelineLayout failed");
	//
	// DBG_VERBOSE("recompiled shader from " + origin);
	return true;
}

vector<DescriptorBinding> Shader::mergeBindings(const vector<DescriptorBinding>& list_a, const vector<DescriptorBinding>& list_b)
{
	multimap<uint32_t, DescriptorBinding> bindings;

	for (const auto& item : list_a)
		bindings.insert({ item.binding, item });
	for (const auto& item : list_b)
		bindings.insert({ item.binding, item });

	if (bindings.empty())
		return { };
	if (bindings.size() == 1)
		return { bindings.begin()->second };

	vector<DescriptorBinding> resolved_bindings;

	auto binding_it = bindings.begin();
	while (binding_it != bindings.end())
	{
		DescriptorBinding last_binding = binding_it->second;
		resolved_bindings.push_back(last_binding);
		++binding_it;
		if (binding_it == bindings.end())
			return resolved_bindings;
		if (binding_it->first == last_binding.binding)
		{
			// uh oh! duplicate bindings! that's not good...
			if (binding_it->second.type == last_binding.type && binding_it->second.buffer_size == last_binding.buffer_size)
				++binding_it;
			else
			{
				DBG_ERROR("incompatible duplicate shader uniform/texture bindings found");
				++binding_it;
			}
		}
	}

	return resolved_bindings;
}

vector<DescriptorBinding> Shader::getReflectedBindings(const vector<uint32_t>& blob)
{
	SpvReflectShaderModule reflected_module;
	SpvReflectResult result = spvReflectCreateShaderModule(blob.size() * 4, blob.data(), &reflected_module);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		DBG_WARNING("unable to construct reflection module");
		return { };
	}
	const SpvReflectDescriptorSet* vert_material_set = spvReflectGetDescriptorSet(&reflected_module, 2, &result);
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		DBG_VERBOSE("unable to reflect descriptor set 2");
		spvReflectDestroyShaderModule(&reflected_module);
		return { };
	}

	vector<DescriptorBinding> bindings;
	for (size_t i = 0; i < vert_material_set->binding_count; ++i)
	{
		SpvReflectDescriptorBinding* binding = vert_material_set->bindings[i];
		if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			DescriptorBinding db;
			db.type = UNIFORM;
			db.binding = binding->binding;
			db.buffer_size = binding->block.padded_size;
			db.name = binding->name;
			for (size_t j = 0; j < binding->block.member_count; ++j)
			{
				SpvReflectBlockVariable member = binding->block.members[j];
				db.variables.push_back({ member.name, member.size, member.offset });
			}
			bindings.push_back(db);
		}
		else if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			DescriptorBinding db;
			db.type = TEXTURE;
			db.binding = binding->binding;
			db.name = binding->name;
			bindings.push_back(db);
		}
	}

	spvReflectDestroyShaderModule(&reflected_module);
	return bindings;
}

VkShaderModule Shader::createShaderModule(const vector<uint32_t>& blob)
{
	VkShaderModuleCreateInfo create_info{ };
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = blob.size() * 4;
	create_info.pCode = blob.data();

	VkShaderModule shader_module;
	if (vkCreateShaderModule(RenderServer::getDevice(), &create_info, nullptr, &shader_module) != VK_SUCCESS)
		DBG_FAULT("vkCreateShaderModule failed");

	return shader_module;
}

void Shader::fixIncludes(string& source_code_text, const string& path_prefix, const bool res_relative)
{
	static const string include_search = "#include \"";
	size_t offset = source_code_text.find(include_search, 0);
	while (offset != string::npos)
	{
		const size_t start = offset + include_search.size();
		const size_t end = source_code_text.find('\"', start);
		string include_path = source_code_text.substr(start, end - start);
		if (include_path.find(' ') != string::npos)
		{
			DBG_ERROR("malformed include found!");
			return;
		}
		source_code_text.erase(offset, (end - offset) + 1);
		string real_path;
		if (include_path.starts_with("res://"))
		{
			real_path = include_path;
		}
		else
		{
			string target_path = path_prefix + include_path;
			filesystem::path fixed_path = target_path;
			auto lex = fixed_path.lexically_normal();
			real_path = lex.string();
			for (char& value : real_path) 
				if (value == '\\')
					value = '/';
			if (res_relative)
				real_path = "res://" + real_path;
		}
		auto include_data = Package::tryLoadFile(real_path);
		if (include_data.empty())
		{
			DBG_WARNING("included file " + real_path + " did not exist, or contained no data!");
		}
		string include_string(include_data.size(), ' ');
		memcpy(include_string.data(), include_data.data(), include_data.size());
		source_code_text.insert(source_code_text.begin() + static_cast<long long>(offset), include_data.begin(), include_data.end());

		offset = source_code_text.find(include_search, offset);
	}
}

static const string vertex_function_sig = "void vertex()";
static const string fragment_function_sig = "void fragment()";

void Shader::preprocess(const string& source_code, string& vertex_shader_code, string& fragment_shader_code, const string& path)
{
	string common_code = source_code;
	
	// remove comments
	size_t comment_pos = common_code.find("//");
	while (comment_pos != string::npos)
	{
		size_t comment_end = common_code.find('\n', comment_pos);
		common_code.erase(comment_pos, (comment_end - comment_pos) + 1);
		comment_pos = common_code.find("//", comment_pos);
	}
	
	comment_pos = common_code.find("/*");
	while (comment_pos != string::npos)
	{
		size_t comment_end = common_code.find("*/", comment_pos);
		common_code.erase(comment_pos, (comment_end - comment_pos) + 2);
		comment_pos = common_code.find("/*", comment_pos);
	}
	
	// top-of-file version and descriptor set preprocessing
	static const string version_str = "#version 450\n\n";
	static const string descriptor_set_0_str = R"V0G0N(struct Light
{
	vec4 position;
	vec4 direction;
	vec4 colour;
	float spot_angle;
	int type;
	bool enabled;
	float padding;
};

layout(set = 0, binding = 0) uniform SceneUniforms
{
	mat4 world_to_view;
	mat4 view_to_clip;
	mat4 clip_to_view;
	mat4 view_to_world;
	ivec2 viewport_size;
	vec3 eye_position;
	float time;
	vec2 near_far;
	Light lights[8];
	vec4 ambient_light;
} scene;

)V0G0N";
	
	static const string descriptor_set_1_str = R"V0G0N(layout(set = 1, binding = 0) uniform ObjectUniforms
{
	mat4 model_to_world;
	int id;
} object;

)V0G0N";
	
	common_code.insert(0, version_str);
	common_code.insert(version_str.size(), descriptor_set_0_str);
	static const string omit_object_uniform_pragma = "#pragma OMIT_OBJECT_UNIFORMS";	
	size_t omit_object_uniform_pragma_pos = common_code.find(omit_object_uniform_pragma);
	if (omit_object_uniform_pragma_pos != string::npos)
		common_code.erase(omit_object_uniform_pragma_pos, omit_object_uniform_pragma.size());
	else
		common_code.insert(version_str.size() + descriptor_set_0_str.size(), descriptor_set_1_str);
	
	// check if vertex and fragment funcs are present and have the correct signature
	size_t vert_func_pos = common_code.find(vertex_function_sig);
	if (vert_func_pos == string::npos)
	{
		DBG_ERROR("error preprocessing " + path + ": vertex shader function is missing or has the wrong signature.");
		return;
	}
	size_t frag_func_pos = common_code.find(fragment_function_sig);
	if (frag_func_pos == string::npos)
	{
		DBG_ERROR("error preprocessing " + path + ": fragment shader function is missing or has the wrong signature.");
		return;
	}
	
	static const string vertex_fragment_varyings = R"VOGON(struct Frag
{
	vec4 position;
	vec4 colour;
	vec4 normal;
	vec4 tangent;
	vec2 uv;
};

)VOGON";
	common_code.insert(vert_func_pos, vertex_fragment_varyings);
	
	// TODO: format/layout uniforms
	
	vertex_shader_code = preprocessVertex(common_code, path);
	fragment_shader_code = preprocessFragment(common_code, path);
}

string Shader::preprocessVertex(const string& common_code, const string& path)
{
	string result_code = common_code;
	size_t vert_func_pos = result_code.find(vertex_function_sig);
	
	// vertex inputs
	static const string vertex_input_default = R"VOGON(layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_colour;
layout(location = 2) in vec4 in_normal;
layout(location = 3) in vec4 in_tangent;
layout(location = 4) in vec2 in_uv;)VOGON";
	static const string vertex_input_pragma = "#pragma DEFAULT_VERTEX";
	size_t vertex_input_pragma_pos = result_code.find(vertex_input_pragma);
	if (vertex_input_pragma_pos > vert_func_pos && vertex_input_pragma_pos != string::npos)
	{
		DBG_ERROR("error preprocessing " + path + ": vertex input pragma must be placed before vertex function.");
		return "";
	}
	if (vertex_input_pragma_pos != string::npos)
	{
		result_code.erase(vertex_input_pragma_pos, vertex_input_pragma.size());
		result_code.insert(vertex_input_pragma_pos, vertex_input_default);
		vert_func_pos = result_code.find(vertex_function_sig);
	}
	
	// vertex outputs
	static const string vertex_outputs = "layout(location = 0) out Frag frag;\n";
	result_code.insert(vert_func_pos, vertex_outputs);
	
	// TODO: custom varyings support
	
	// default/canvas vertex transform
	static const string default_vertex_transform = R"VOGON(frag.position = object.model_to_world * vec4(in_position.xyz, 1);
	frag.colour = in_colour;
	frag.normal = vec4(normalize((object.model_to_world * vec4(in_normal.xyz, 0)).xyz), 0);
	frag.tangent = vec4(normalize((object.model_to_world * vec4(in_tangent.xyz, 0)).xyz), 0);
	frag.uv = in_uv;
	gl_Position = scene.view_to_clip * scene.world_to_view * frag.position;)VOGON";
	static const string canvas_vertex_transform = R"VOGON(frag.position = vec4(in_position.xyz, 1);
	frag.uv = in_uv;
	gl_Position = vec4(in_position.xyz, 1);)VOGON";
	static const string default_transform_pragma = "#pragma DEFAULT_TRANSFORM";
	static const string canvas_transform_pragma = "#pragma CANVAS_TRANSFORM";
	size_t default_transform_pragma_pos = result_code.find(default_transform_pragma);
	size_t canvas_transform_pragma_pos = result_code.find(canvas_transform_pragma);
	
	if (default_transform_pragma_pos != string::npos)
	{
		if (canvas_transform_pragma_pos != string::npos)
			DBG_WARNING("error preprocessing " + path + ": only DEFAULT_TRANSFORM or CANVAS_TRANSFORM should be used in a shader, not both at once.");
		result_code.erase(default_transform_pragma_pos, default_transform_pragma.size());
		result_code.insert(default_transform_pragma_pos, default_vertex_transform);
	}
	else if (canvas_transform_pragma_pos != string::npos)
	{
		result_code.erase(canvas_transform_pragma_pos, canvas_transform_pragma.size());
		result_code.insert(canvas_transform_pragma_pos, canvas_vertex_transform);
	}
	
	removeFunction(result_code, fragment_function_sig);
	destroyAllPragmas(result_code);
	
	return result_code;
}

string Shader::preprocessFragment(const string& common_code, const string& path)
{
	string result_code = common_code;
	size_t frag_func_pos = result_code.find(fragment_function_sig);
	
	// fragment inputs
	static const string fragment_inputs = "layout(location = 0) in Frag frag;\n";
	result_code.insert(frag_func_pos, fragment_inputs);
	
	// default/canvas fragment outputs
	static const string default_attachments = R"VOGON(layout(location = 0) out vec4 out_colour;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_params;
layout(location = 3) out vec4 out_custom;)VOGON";
	static const string canvas_attachments = R"VOGON(layout(location = 0) out vec4 out_colour;)VOGON";
	static const string default_attachment_pragma = "#pragma DEFAULT_ATTACHMENTS";
	static const string canvas_attachment_pragma = "#pragma CANVAS_ATTACHMENTS";
	size_t default_attachment_pragma_pos = result_code.find(default_attachment_pragma);
	size_t canvas_attachment_pragma_pos = result_code.find(canvas_attachment_pragma);
	
	if (default_attachment_pragma_pos != string::npos)
	{
		if (canvas_attachment_pragma_pos != string::npos)
			DBG_WARNING("error preprocessing " + path + ": only DEFAULT_ATTACHMENTS or CANVAS_ATTACHMENTS should be used in a shader, not both at once.");
		result_code.erase(default_attachment_pragma_pos, default_attachment_pragma.size());
		result_code.insert(default_attachment_pragma_pos, default_attachments);
	}
	else if (canvas_attachment_pragma_pos != string::npos)
	{
		result_code.erase(canvas_attachment_pragma_pos, canvas_attachment_pragma.size());
		result_code.insert(canvas_attachment_pragma_pos, canvas_attachments);
	}
	
	removeFunction(result_code, vertex_function_sig);
	destroyAllPragmas(result_code);
	
	return result_code;
}

void Shader::removeFunction(string& code, const string& signature)
{
	size_t function_start = code.find(signature);
	size_t function_end = function_start + signature.size();
	size_t steps_deep = 0;
	while (function_end < code.size())
	{
		if (code[function_end] == '{')
			++steps_deep;
		else if (code[function_end] == '}')
		{
			--steps_deep;
			if (steps_deep == 0)
				break;
		}
		++function_end;
	}
	code.erase(function_start, (function_end - function_start) + 1);
}

void Shader::destroyAllPragmas(std::string& code)
{
	size_t pragma_pos = code.find("#pragma");
	while (pragma_pos != string::npos)
	{
		size_t end_pos = code.find('\n', pragma_pos);
		code.erase(pragma_pos, (end_pos - pragma_pos) + 1);
		pragma_pos = code.find("#pragma", pragma_pos);
	}
}

bool Shader::compileShaders(const string& path, vector<uint32_t>& vert_blob, vector<uint32_t>& frag_blob)
{
	auto shader_data = Package::tryLoadFile(path);
	
	if (shader_data.empty())
	{
		DBG_ERROR("shader " + path + " not found");
		return false;
	}
	
	filesystem::path current_file_path;
	bool is_res_relative = false;
	if (path.starts_with("res://"))
	{
		current_file_path = path.substr(6);
		is_res_relative = true;		
	}
	else
		current_file_path = path;
	const string current_file_location = current_file_path.remove_filename().string();
	
	string shader_text; shader_text.resize(shader_data.size());
	memcpy(shader_text.data(), shader_data.data(), shader_data.size());
	
	Shader::fixIncludes(shader_text, current_file_location, is_res_relative);
	string vertex_text;
	string fragment_text;
	Shader::preprocess(shader_text, vertex_text, fragment_text, path);
	if (vertex_text.empty() || fragment_text.empty())
		return false;
	
	const shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.AddMacroDefinition("vertex", "main");
	auto result = compiler.CompileGlslToSpv(vertex_text.data(), vertex_text.size(), shaderc_glsl_vertex_shader, path.c_str(), options);
	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		DBG_ERROR("error compiling vertex shader " + path + ": " + result.GetErrorMessage());
		DBG_INFO("see the full shader code below: " + vertex_text);
		return false;
	}
	vert_blob = { result.cbegin(), result.cend() };
	
	shaderc::CompileOptions options2;
	options2.AddMacroDefinition("fragment", "main");
	result = compiler.CompileGlslToSpv(fragment_text.data(), fragment_text.size(), shaderc_glsl_fragment_shader, path.c_str(), options2);
	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		DBG_ERROR("error compiling fragment shader " + path + ": " + result.GetErrorMessage());
		DBG_INFO("see the full shader code below: " + fragment_text);
		return false;
	}
	frag_blob = { result.cbegin(), result.cend() };
	
	return true;
}

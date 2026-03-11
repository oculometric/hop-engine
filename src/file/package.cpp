#include "package.h"

#include <filesystem>
#include <fstream>
#include <cstring>

using namespace HopEngine;
using namespace std;

static Package* instance = nullptr;

void Package::init()
{
	DBG_INFO("initialising package manager");
	if (instance == nullptr)
		instance = new Package();
}

void Package::destroy()
{
	DBG_INFO("destroying package manager");
	if (instance != nullptr)
	{
		delete instance;
		instance = nullptr;
	}
}

DataBlock Package::load(const string& path)
{
    string real_path;
	if (isResPath(path, real_path))
	{
		if (!instance)
		Package::init();

		DBG_VERBOSE("loading '" + real_path + "'");
		const auto redirector = instance->alias_table.find(real_path);
		map<string, vector<uint8_t>>::iterator it;
		if (redirector == instance->alias_table.end())
			it = instance->database.find(real_path);
		else
			it = instance->database.find(redirector->second);

		if (it != instance->database.end())
			return it->second;

		if (redirector == instance->alias_table.end())
			DBG_WARNING("found no data associated with '" + real_path + "'");
		else
			DBG_WARNING("found no data associated with '" + real_path + "' (redirected to '" + redirector->second + "')");

		return { };
	}
	else
		return loadFromDisk(path);
}

DataBlock Package::loadFromDisk(const string& path)
{
	DBG_VERBOSE("loading '" + path + "' from file");
	// load file data
	ifstream file(path, ios::ate | ios::binary);
	if (!file.is_open())
	{
		DBG_WARNING("failed to load '" + path + "'; file not found");
		return { };
	}

	vector<uint8_t> content(file.tellg());
	file.seekg(ios::beg);
	file.read(reinterpret_cast<char*>(content.data()), static_cast<streamsize>(content.size()));
	file.close();

	return content;
}

bool Package::store(const string& path, const DataBlock& data)
{
	string real_path;
	if (isResPath(path, real_path))
	{
		DBG_VERBOSE("storing '" + identifier + "'; " + to_string(data.size()) + " bytes");
		instance->database[real_path] = data;
		return true;
	}
	else
		return storeToDisk(path, data);
}

bool Package::storeToDisk(const string& path, const DataBlock& data)
{
    DBG_VERBOSE("storing '" + path + "' to file; " + to_string(data.size()) + " bytes");
	ofstream file(path, ios::binary);
	if (!file.is_open())
	{
		DBG_ERROR("failed to store '" + path + "'; file not accessible");
		return false;
	}
	file.write(reinterpret_cast<const char*>(data.data()), static_cast<streamsize>(data.size()));
	file.close();
	true;
}

bool Package::exportPackage(const string& path, bool compressed)
{
	DBG_VERBOSE("storing package: " + path);
	DataBlock data = exportPackage(compressed);
    return storeToDisk(path, data);
}

bool Package::importPackage(const string& path)
{
	DBG_VERBOSE("loading package: " + path);
    DataBlock data = loadFromDisk(path);
	if (data.empty())
		return false;
	return importPackage(data);
}

constexpr uint32_t SIGNATURE = 0xCA55E77E;

struct PackageHeader
{
	uint32_t signature_version;
	uint32_t file_size;
	uint32_t alias_entries;
	uint32_t package_entries;
};

struct AliasEntry
{
	uint32_t a_string_length;
	uint32_t b_string_length;
};

struct DataBlockEntry
{
	uint32_t name_size;
	uint32_t data_size;
};

DataBlock Package::exportPackage(bool compressed)
{
	DBG_VERBOSE("exporting version 4 package");
	PackageHeader header;
	header.signature_version = SIGNATURE + 4;
	header.package_entries = static_cast<uint32_t>(instance->database.size());
	header.alias_entries = static_cast<uint32_t>(instance->alias_table.size());
	header.file_size = sizeof(PackageHeader);
	for (const auto& alias : instance->alias_table)
		header.file_size += sizeof(AliasEntry) + alias.first.size() + alias.second.size();
	for (const auto& block : instance->database)
		header.file_size += sizeof(DataBlockEntry) + block.first.size() + block.second.size();

	DataBlock data; data.resize(header.file_size);
	uint8_t* write_point = data.data();

	memcpy(write_point, &header, sizeof(PackageHeader));
	write_point += sizeof(PackageHeader);

	for (const auto& alias : instance->alias_table)
	{
		AliasEntry entry = { static_cast<uint32_t>(alias.first.size()), static_cast<uint32_t>(alias.second.size()) };
		memcpy(write_point, &entry, sizeof(AliasEntry));
		write_point += sizeof(AliasEntry);
		memcpy(write_point, alias.first.data(), alias.first.size());
		write_point += alias.first.size();
		memcpy(write_point, alias.second.data(), alias.second.size());
		write_point += alias.second.size();
	}

	for (const auto& block : instance->database)
	{
		DataBlockEntry entry = { static_cast<uint32_t>(block.first.size()), static_cast<uint32_t>(block.second.size()) };
		memcpy(write_point, &entry, sizeof(DataBlockEntry));
		write_point += sizeof(DataBlockEntry);
		memcpy(write_point, block.first.data(), block.first.size());
		write_point += block.first.size();
		memcpy(write_point, block.second.data(), block.second.size());
		write_point += block.second.size();
	}

	DBG_INFO("stored " + to_string(header.package_entries) + " items to package");

	if (compressed)
	{
		DBG_VERBOSE("storing compressed (version 2) package");
		string command;
#if defined(_WIN32)
		command = "zip.exe -j ";
#else
		command = "zip -rj ";
#endif

		filesystem::create_directories(Package::getTempPath());
		string temp_hop_address = Package::getTempPath() + "hop_package_tmp" + PTR(instance);
		if (!storeToDisk(temp_hop_address, data))
		{
			DBG_ERROR("failed to store package to temp: " + temp_hop_address);
			return data;
		}
		string temp_zip_address = temp_hop_address + ".zip";
		command = command + temp_zip_address + ' ' + temp_hop_address;
		string output;

		int result = exec(command, output);
		if (result != 0)
		{
			DBG_ERROR("error compressing package: " + output);
			filesystem::remove(temp_hop_address);
			return data;
		}
		filesystem::remove(temp_hop_address);

		ifstream file(temp_zip_address, ios::ate | ios::binary);
		if (!file.is_open())
		{
			DBG_ERROR("failed to generate compressed package; unable to open zip file");
			filesystem::remove(temp_zip_address);
			return data;
		}

		size_t size = static_cast<size_t>(file.tellg());
		PackageHeader header2;
		header2.signature_version = SIGNATURE + 2;
		header2.package_entries = 0;
		header2.alias_entries = 0;
		header2.file_size = sizeof(PackageHeader) + static_cast<uint32_t>(size);

		data.resize(header2.file_size);
		memcpy(data.data(), &header2, sizeof(PackageHeader));

		file.seekg(0);
		file.read(reinterpret_cast<char*>(data.data() + sizeof(PackageHeader)), static_cast<streamsize>(size));
		file.close();
		filesystem::remove(temp_zip_address);

		DBG_INFO("generated compressed package");
	}

	return data;
}

bool Package::importPackage(const DataBlock& data)
{
	if (data.size() < sizeof(PackageHeader))
	{
		DBG_ERROR("failed to load package; corrupted file");
		return false;
	}

	PackageHeader header = *reinterpret_cast<const PackageHeader*>(data.data());
	if (header.file_size != static_cast<uint32_t>(data.size()))
	{
		DBG_ERROR("failed to load package; invalid file size");
		return false;
	}
	if (header.signature_version == SIGNATURE + 2)
	{
		DBG_VERBOSE("loading compressed package");

		filesystem::create_directories(Package::getTempPath());
		string temp_zip_address = Package::getTempPath() + "hop_package_tmp" + PTR(instance) + ".zip";
		DataBlock trimmed(data.begin() + sizeof(PackageHeader), data.end());
		if (!storeToDisk(temp_zip_address, trimmed))
		{
			DBG_ERROR("failed to load package; error during decompression");
			return false;
		}
		trimmed.clear();

		string command;
#if defined(_WIN32)
		command = "tar.exe -x -f ";
#else
		command = "unzip ";
#endif
		string unpack_dir = Package::getTempPath() + "hop";
		filesystem::create_directory(unpack_dir);
		command = command + temp_zip_address + 
#if defined(_WIN32)
				" -C "
#else
				" -d "
#endif
				+ unpack_dir;
		string output;

		int result = exec(command, output);
		filesystem::remove(temp_zip_address);
		if (result != 0)
		{
			DBG_ERROR("error decompressing package; " + output);
			return false;
		}

		auto it = filesystem::directory_iterator(unpack_dir);
		if (!it->exists())
		{
			DBG_ERROR("error decompressing package; no package file found");
			filesystem::remove(unpack_dir);
			return false;
		}

		DataBlock decompressed = loadFromDisk(it->path().string());
		filesystem::remove_all(unpack_dir);

		if (!importPackage(decompressed))
		{
			DBG_ERROR("failed to load package; error during decompression");
			return false;
		}
		return true;
	}
	if (header.signature_version != SIGNATURE + 4)
	{
		DBG_ERROR("failed to load package; invalid signature/version");
		return false;
	}

	const uint8_t* data_end = data.data() + data.size();
	const uint8_t* read_point = data.data() + sizeof(PackageHeader);

	vector<pair<string, string>> aliases;
	for (uint32_t i = 0; i < header.alias_entries; ++i)
	{
		if (read_point + sizeof(AliasEntry) > data_end)
		{
			DBG_ERROR("error reading package; truncated file");
			return false;
		}
		AliasEntry alias = *reinterpret_cast<const AliasEntry*>(read_point);
		read_point += sizeof(AliasEntry);
		if (read_point + alias.a_string_length + alias.b_string_length > data_end)
		{
			DBG_ERROR("error reading package; truncated file");
			return false;
		}
		string string_a(alias.a_string_length, ' ');
		memcpy(string_a.data(), read_point, string_a.size());
		read_point += string_a.size();
		string string_b(alias.b_string_length, ' ');
		memcpy(string_b.data(), read_point, string_b.size());
		read_point += string_b.size();

		aliases.emplace_back(string_a, string_b);
	}

	vector<pair<string, DataBlock>> blocks;
	for (uint32_t i = 0; i < header.package_entries; ++i)
	{
		if (read_point + sizeof(DataBlockEntry) > data_end)
		{
			DBG_ERROR("error reading package; truncated file");
			return false;
		}
		DataBlockEntry block = *reinterpret_cast<const DataBlockEntry*>(read_point);
		read_point += sizeof(DataBlockEntry);
		if (read_point + block.name_size + block.data_size > data_end)
		{
			DBG_ERROR("error reading package; truncated file");
			return false;
		}
		string name(block.name_size, ' ');
		memcpy(name.data(), read_point, name.size());
		read_point += name.size();
		DataBlock data_block(block.data_size, ' ');
		memcpy(data_block.data(), read_point, data_block.size());
		read_point += data_block.size();

		blocks.emplace_back(name, data_block);
	}

	instance->alias_table.insert(aliases.begin(), aliases.end());
	instance->database.insert(blocks.begin(), blocks.end());
	DBG_INFO("loaded " + to_string(header.package_entries) + " items from package");

    return true;
}

vector<string> Package::listLoadedEntries()
{
	vector<string> names;
	names.reserve(instance->database.size());
	for (const auto& [identifier, _] : instance->database)
		names.push_back(identifier);
	return names;
}

void Package::setAlias(const std::string& a, const std::string& b)
{ instance->alias_table[a] = b; }

void Package::clearAlias(const std::string& a)
{ instance->alias_table.erase(a); }

bool Package::isResPath(const string& path, string& trimmed)
{
	const static string res_prefix = "res://";
	if (path.starts_with(res_prefix))
	{
		trimmed = path.substr(res_prefix.size());
		return true;
	}
    return false;
}

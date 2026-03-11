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


constexpr uint32_t SIGNATURE = 0xCA55E77E;

struct PackageHeader
{
	uint32_t signature;
	uint32_t version;
	uint64_t file_size;
	uint32_t package_entries;
	uint32_t alias_entries;
};

struct PackageEntry
{
	size_t data_header_offset;
	size_t data_total_size;
};

struct AliasEntry
{
	size_t a_string_length;
	size_t b_string_length;
};

struct PackageDataHeader
{
	size_t name_size;
	size_t data_size;
};

bool Package::loadPackageFromMemory(vector<uint8_t>& content, const string& load_path)
{
	size_t size = content.size();

	if (size < sizeof(PackageHeader))
	{
		DBG_ERROR("failed to load package: " + load_path + "; corrupted file");
		return false;
	}

	PackageHeader header = *reinterpret_cast<PackageHeader*>(content.data());
	if (header.signature != SIGNATURE)
	{
		DBG_ERROR("failed to load package: " + load_path + "; invalid signature");
		return false;
	}
	if (header.file_size != size)
	{
		DBG_ERROR("failed to load package: " + load_path + "; invalid file size");
		return false;
	}
	if (header.version == 2)
	{
		content = loadCompressedPackage(content);
		if (content.empty())
		{
			DBG_ERROR("failed to load package: " + load_path + "; error during decompression");
			return false;
		}
	}
	else if (header.version != 3)
	{
		DBG_ERROR("failed to load package: " + load_path + "; invalid version");
		return false;
	}

	header = *reinterpret_cast<PackageHeader*>(content.data());

	vector<PackageEntry> entries(header.package_entries);
	memcpy(entries.data(), content.data() + sizeof(PackageHeader), sizeof(PackageEntry) * entries.size());

	for (const auto& [data_header_offset, data_total_size] : entries)
	{
		PackageDataHeader data_header = *reinterpret_cast<PackageDataHeader*>(content.data() + data_header_offset);
		if (data_header.data_size + data_header.name_size + sizeof(PackageDataHeader) != data_total_size)
		{
			DBG_ERROR("error loading package: " + load_path + "; invalid data entry size");
			return false;
		}
		string name(data_header.name_size, ' ');
		memcpy((char*)(name.data()), (content.data() + data_header_offset + sizeof(PackageDataHeader)), name.size());
		vector<uint8_t> data(data_header.data_size);
		memcpy(data.data(), (content.data() + data_header_offset + sizeof(PackageDataHeader) + name.size()), data.size());
		instance->database[name] = data;
	}
	
	size_t offset = sizeof(PackageHeader) + (sizeof(PackageEntry) * entries.size());
	for (size_t i = 0; i < header.alias_entries; ++i)
	{
		AliasEntry alias_header = *reinterpret_cast<AliasEntry*>(content.data() + offset);
		string a_string(alias_header.a_string_length, ' ');
		string b_string(alias_header.b_string_length, ' ');
		memcpy(a_string.data(), content.data() + offset + sizeof(AliasEntry), alias_header.a_string_length);
		memcpy(b_string.data(), content.data() + offset + sizeof(AliasEntry) + alias_header.a_string_length, alias_header.b_string_length);
		instance->alias_table[a_string] = b_string;
		
		offset += sizeof(AliasEntry) + alias_header.a_string_length + alias_header.b_string_length;
	}

	DBG_INFO("loaded " + to_string(header.package_entries) + " items from package: " + load_path);
	return true;
}

bool Package::loadPackage(const string& load_path)
{
	if (!instance)
		Package::init();

	DBG_VERBOSE("loading package: " + load_path);
	ifstream file(load_path, ios::ate | ios::binary);
	if (!file.is_open())
	{
		DBG_ERROR("failed to load package: " + load_path + "; file not accessible");
		return false;
	}

	const size_t size = file.tellg();
	vector<uint8_t> content(size);
	file.seekg(ios::beg);
	file.read(reinterpret_cast<char*>(content.data()), static_cast<streamsize>(size));
	file.close();

	return loadPackageFromMemory(content, load_path);
}

vector<string> Package::listLoadedEntries()
{
	vector<string> names;
	names.reserve(instance->database.size());
	for (const auto& [identifier, _] : instance->database)
		names.push_back(identifier);
	return names;
}

vector<uint8_t> Package::loadData(const string& identifier)
{
	if (!instance)
		Package::init();

	DBG_VERBOSE("loading '" + identifier + "'");
	const auto redirector = instance->alias_table.find(identifier);
	map<string, vector<uint8_t>>::iterator it;
	if (redirector == instance->alias_table.end())
		it = instance->database.find(identifier);
	else
		it = instance->database.find(redirector->second);
	if (it != instance->database.end())
		return it->second;
	if (redirector == instance->alias_table.end())
		DBG_WARNING("found no data associated with '" + identifier + "'");
	else
		DBG_WARNING("found no data associated with '" + identifier + "' (redirected to '" + redirector->second + "')");
	return { };
}

vector<uint8_t> Package::tryLoadFile(const string& path_or_identifier)
{
	if (!instance)
		Package::init();

	const static string res_prefix = "res://";
	if (path_or_identifier.starts_with(res_prefix))
	{
		// load package resource
		return Package::loadData(path_or_identifier.substr(res_prefix.size()));
	}
	else
	{
		DBG_VERBOSE("loading '" + path_or_identifier + "' from file");
		// load file data
		ifstream file(path_or_identifier, ios::ate | ios::binary);
		if (!file.is_open())
		{
			DBG_WARNING("failed to load '" + path_or_identifier + "'; file not accessible");
			return { };
		}

		const size_t size = file.tellg();
		vector<uint8_t> content(size);
		file.seekg(0);
		file.read(reinterpret_cast<char*>(content.data()), static_cast<streamsize>(size));
		file.close();

		return content;
	}
}

bool Package::storePackage(const string& store_path)
{
	if (!instance)
		Package::init();

	DBG_VERBOSE("storing package: " + store_path);
	ofstream file(store_path, ios::binary);
	if (!file.is_open())
	{
		DBG_ERROR("failed to store package: " + store_path + "; file not accessible");
		return false;
	}

	PackageHeader header;
	header.signature = SIGNATURE;
	header.package_entries = static_cast<uint32_t>(instance->database.size());
	header.alias_entries = static_cast<uint32_t>(instance->alias_table.size());
	header.version = 3;
	
	vector<vector<uint8_t>> data_blocks;
	size_t offset = sizeof(PackageHeader);
	for (const auto& [a, b] : instance->alias_table)
	{
		const AliasEntry entry = { a.size(), b.size() };
		vector<uint8_t> data_block(sizeof(AliasEntry) + entry.a_string_length + entry.b_string_length);
		memcpy(data_block.data(), &entry, sizeof(AliasEntry));
		memcpy(data_block.data() + sizeof(AliasEntry), a.data(), entry.a_string_length);
		memcpy(data_block.data() + sizeof(AliasEntry) + entry.a_string_length, b.data(), entry.b_string_length);
		data_blocks.push_back(data_block);
		offset += data_block.size();
	}
	
	vector<PackageEntry> entries;
	offset += (instance->database.size() * sizeof(PackageEntry));
	for (auto [identifier, object_data] : instance->database)
	{
		PackageDataHeader data_header;
		data_header.name_size = identifier.size();
		data_header.data_size = object_data.size();
		vector<uint8_t> data_block(sizeof(PackageDataHeader) + data_header.name_size + data_header.data_size);
		memcpy(data_block.data(), &data_header, sizeof(PackageDataHeader));
		memcpy(data_block.data() + sizeof(PackageDataHeader), identifier.data(), data_header.name_size);
		memcpy(data_block.data() + sizeof(PackageDataHeader) + data_header.name_size, object_data.data(), data_header.data_size);
		data_blocks.push_back(data_block);

		PackageEntry entry;
		entry.data_total_size = data_block.size();
		entry.data_header_offset = offset;
		entries.push_back(entry);

		offset += entry.data_total_size;
	}

	header.file_size = offset;
	file.write(reinterpret_cast<char*>(&header), sizeof(PackageHeader));
	file.write(reinterpret_cast<char*>(entries.data()), static_cast<streamsize>(entries.size() * sizeof(PackageEntry)));
	for (const vector<uint8_t>& data_block : data_blocks)
		file.write(reinterpret_cast<const char*>(data_block.data()), static_cast<streamsize>(data_block.size()));
	file.close();

	DBG_INFO("stored " + to_string(header.package_entries) + " items to package: " + store_path);
	return true;
}

bool Package::storeCompressedPackage(const string& store_path)
{
	if (!instance)
		Package::init();

	DBG_VERBOSE("storing compressed package: " + store_path);
	if (!storePackage(store_path))
	{
		DBG_ERROR("failed to generate version 1 package: " + store_path);
		return false;
	}

	string command = "zip -rj ";
#if defined(_WIN32)
	command = "zip.exe -j ";
#endif

	string temp_address = Package::getTempPath() + "hop_package_tmp" + PTR(instance) + ".zip";
	filesystem::create_directories(Package::getTempPath());
	command = command + temp_address + ' ' + store_path;
	string output;

	int result = exec(command, output);
	if (result != 0)
	{
		DBG_ERROR("error compressing package: " + store_path + "; " + output);
		return false;
	}

	ifstream file(temp_address, ios::ate | ios::binary);
	if (!file.is_open())
	{
		filesystem::remove(temp_address);
		DBG_ERROR("failed to generate compressed package: " + store_path + "; unable to open zip file");
		return false;
	}

	size_t size = (size_t)file.tellg();
	vector<uint8_t> content(size);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(content.data()), static_cast<streamsize>(size));
	file.close();
	filesystem::remove(temp_address);

	PackageHeader header;
	header.signature = SIGNATURE;
	header.package_entries = 0;
	header.alias_entries = 0;
	header.file_size = sizeof(PackageHeader) + size;
	header.version = 2;

	ofstream outfile(store_path, ios::binary);
	if (!outfile.is_open())
	{
		DBG_ERROR("failed to generate compressed package: " + store_path + "; file not accessible");
		return false;
	}
	outfile.write(reinterpret_cast<char*>(&header), sizeof(PackageHeader));
	outfile.write(reinterpret_cast<char*>(content.data()), static_cast<streamsize>(size));
	outfile.close();

	DBG_INFO("stored compressed package: " + store_path);
	return true;
}

void Package::storeData(const string& identifier, const vector<uint8_t>& data)
{
	if (!instance)
		Package::init();

	DBG_VERBOSE("storing '" + identifier + "'; " + to_string(data.size()) + " bytes");
	instance->database[identifier] = data;
}

void Package::tryWriteFile(const string& path, const vector<uint8_t>& data)
{
	if (!instance)
		Package::init();

	DBG_VERBOSE("storing '" + path + "' to file; " + to_string(data.size()) + " bytes");
	ofstream file(path, ios::binary);
	if (!file.is_open())
	{
		DBG_ERROR("failed to store '" + path + "'; file not accessible");
		return;
	}
	file.write(reinterpret_cast<const char*>(data.data()), static_cast<streamsize>(data.size()));
	file.close();
}

void Package::setAlias(const std::string& a, const std::string& b)
{ instance->alias_table[a] = b; }

void Package::clearAlias(const std::string& a)
{ instance->alias_table.erase(a); }

Package::~Package()
{
}

vector<uint8_t> Package::loadCompressedPackage(const vector<uint8_t>& data)
{
	if (!instance)
		Package::init();

	if (data.size() < sizeof(PackageHeader))
	{
		DBG_ERROR("failed to load compressed package; invalid size");
		return { };
	}

	PackageHeader header = *reinterpret_cast<PackageHeader*>(const_cast<uint8_t*>(data.data()));
	DBG_VERBOSE("loading compressed package");

	if (header.signature != SIGNATURE)
	{
		DBG_ERROR("failed to load compressed package; invalid signature");
		return { };
	}
	if (header.file_size != data.size())
	{
		DBG_ERROR("failed to load compressed package; invalid file size");
		return { };
	}
	if (header.version != 2)
	{
		DBG_ERROR("failed to load compressed package; invalid version");
		return { };
	}

	string temp_address = Package::getTempPath() + "hop_package_tmp.zip";
	filesystem::create_directories(Package::getTempPath());
	ofstream file(temp_address, ios::binary);
	if (!file.is_open())
	{
		DBG_ERROR("error decompressing package; file not accessible");
		return { };
	}
	file.write(reinterpret_cast<const char*>(data.data()) + sizeof(PackageHeader), static_cast<streamsize>(header.file_size - sizeof(PackageHeader)));
	file.close();

	string command = "unzip ";
#if defined(_WIN32)
	command = "tar.exe -x -f ";
#endif
	
	string unpack_dir = Package::getTempPath() + "hop";
	filesystem::create_directory(unpack_dir);
	command = command + temp_address + 
#if defined(_WIN32)
	          " -C "
#else
	          " -d "
#endif
	          + unpack_dir;
	string output;

	int result = exec(command, output);
	filesystem::remove(temp_address);
	if (result != 0)
	{
		DBG_ERROR("error decompressing package; " + output);
		return { };
	}

	auto it = filesystem::directory_iterator(unpack_dir);
	if (!it->exists())
	{
		DBG_ERROR("error decompressing package; no package file found");
		filesystem::remove(unpack_dir);
		return { };
	}

	ifstream infile(it->path().string(), ios::ate | ios::binary);
	if (!infile.is_open())
	{
		DBG_ERROR("error decompressing package; file not accessible");
		filesystem::remove(unpack_dir);
		return { };
	}
	size_t size = infile.tellg();
	vector<uint8_t> content(size);
	infile.seekg(0);
	infile.read(reinterpret_cast<char*>(content.data()), static_cast<streamsize>(size));
	infile.close();
	filesystem::remove_all(unpack_dir);

	header = *reinterpret_cast<PackageHeader*>(content.data());
	if (header.signature != SIGNATURE)
	{
		DBG_ERROR("failed to load package; invalid signature");
		return { };
	}
	if (header.file_size != content.size())
	{
		DBG_ERROR("failed to load package; invalid file size");
		return { };
	}
	if (header.version != 3)
	{
		DBG_ERROR("failed to load package; invalid version");
		return { };
	}

	DBG_VERBOSE("unpacked compressed package");
	return content;
}

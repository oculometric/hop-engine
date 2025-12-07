#include "package.h"

#include <filesystem>
#include <fstream>

using namespace HopEngine;
using namespace std;

static Package* application_package = nullptr;

Package::Package() { }

Package::~Package()
{
	database.clear();
}

void Package::init()
{
	if (application_package == nullptr)
		application_package = new Package();
}

constexpr uint32_t SIGNATURE = 0xCA55E77E;

struct PackageHeader
{
	uint32_t signature;
	uint32_t version;
	size_t file_size;
	size_t package_entries;
};

struct PackageEntry
{
	size_t data_header_offset;
	size_t data_total_size;
};

struct PackageDataHeader
{
	size_t name_size;
	size_t data_size;
};

// package file structure
// PackageHeader
// data table: array of PackageEntry
//
// PackageDataHeader
// name
// data
// ...

bool Package::loadPackage(string load_path)
{
	ifstream file(load_path, ios::ate | ios::binary);
	if (!file.is_open())
		return false;

	size_t size = (size_t)file.tellg();
	vector<uint8_t> content(size);
	file.seekg(0);
	file.read((char*)(content.data()), size);
	file.close();

	if (size < sizeof(PackageHeader))
		return false;

	PackageHeader header = *((PackageHeader*)content.data());
	if (header.signature != SIGNATURE)
		return false;
	if (header.file_size != size)
		return false;
	if (header.version == 2)
		content = loadCompressedPackage(content);
	else if (header.version != 1)
		return false;

	header = *((PackageHeader*)content.data());

	vector<PackageEntry> entries(header.package_entries);
	memcpy(entries.data(), content.data() + sizeof(PackageHeader), sizeof(PackageEntry) * entries.size());

	for (PackageEntry& entry : entries)
	{
		PackageDataHeader data_header = *((PackageDataHeader*)(content.data() + entry.data_header_offset));
		if (data_header.data_size + data_header.name_size + sizeof(PackageDataHeader) != entry.data_total_size)
			return false;
		string name(data_header.name_size, ' ');
		memcpy((char*)(name.data()), (content.data() + entry.data_header_offset + sizeof(PackageDataHeader)), name.size());
		vector<uint8_t> data(data_header.data_size);
		memcpy(data.data(), (content.data() + entry.data_header_offset + sizeof(PackageDataHeader) + name.size()), data.size());
		application_package->database[name] = data;
	}

	return true;
}

bool Package::storePackage(string store_path)
{
	ofstream file(store_path, ios::binary);
	if (!file.is_open())
		return false;

	PackageHeader header;
	header.signature = SIGNATURE;
	header.package_entries = application_package->database.size();
	header.version = 1;

	vector<PackageEntry> entries;
	vector<vector<uint8_t>> data_blocks;
	size_t offset = sizeof(PackageHeader) + (application_package->database.size() * sizeof(PackageEntry));
	for (auto pair : application_package->database)
	{
		PackageDataHeader data_header;
		data_header.name_size = pair.first.size();
		data_header.data_size = pair.second.size();
		vector<uint8_t> data_block(sizeof(PackageDataHeader) + data_header.name_size + data_header.data_size);
		memcpy(data_block.data(), &data_header, sizeof(PackageDataHeader));
		memcpy(data_block.data() + sizeof(PackageDataHeader), pair.first.data(), data_header.name_size);
		memcpy(data_block.data() + sizeof(PackageDataHeader) + data_header.name_size, pair.second.data(), data_header.data_size);
		data_blocks.push_back(data_block);

		PackageEntry entry;
		entry.data_total_size = data_block.size();
		entry.data_header_offset = offset;
		entries.push_back(entry);

		offset += entry.data_total_size;
	}

	header.file_size = offset;
	file.write((char*)(&header), sizeof(PackageHeader));
	file.write((char*)entries.data(), entries.size() * sizeof(PackageEntry));
	for (const vector<uint8_t>& data_block : data_blocks)
		file.write((char*)(data_block.data()), data_block.size());
	file.close();

	return true;
}

bool Package::storeCompressedPackage(string store_path)
{
	if (!storePackage(store_path))
		return false;

	string command = "tar";
#if defined(_WIN32)
	command = "tar.exe";
#endif

	string temp_address = Package::getTempPath() + "hop_package_tmp.zip";
	filesystem::create_directories(Package::getTempPath());
	command = command + " -a -c -f " + temp_address + ' ' + store_path;
	string output;

	int result = exec(command, output);
	if (result != 0)
		return false;

	ifstream file(temp_address, ios::ate | ios::binary);
	if (!file.is_open())
	{
		filesystem::remove(temp_address);
		return false;
	}

	size_t size = (size_t)file.tellg();
	vector<uint8_t> content(size);
	file.seekg(0);
	file.read((char*)(content.data()), size);
	file.close();
	filesystem::remove(temp_address);

	PackageHeader header;
	header.signature = SIGNATURE;
	header.package_entries = 0;
	header.file_size = sizeof(PackageHeader) + size;
	header.version = 2;

	ofstream outfile(store_path, ios::binary);
	if (!outfile.is_open())
		return false;
	outfile.write((char*)(&header), sizeof(PackageHeader));
	outfile.write((char*)(content.data()), content.size());
	outfile.close();

	return true;
}

vector<uint8_t> Package::loadCompressedPackage(vector<uint8_t> data)
{
	PackageHeader header = *((PackageHeader*)data.data());

	if (header.signature != SIGNATURE)
		return { };
	if (header.file_size != data.size())
		return { };
	if (header.version != 2)
		return { };

	string temp_address = Package::getTempPath() + "hop_package_tmp.zip";
	filesystem::create_directories(Package::getTempPath());
	ofstream file(temp_address, ios::binary);
	if (!file.is_open())
		return { };
	file.write((char*)(data.data()) + sizeof(PackageHeader), header.file_size - sizeof(PackageHeader));
	file.close();

	string command = "tar";
#if defined(_WIN32)
	command = "tar.exe";
#endif

	string unpack_dir = Package::getTempPath() + "hop";
	filesystem::create_directory(unpack_dir);
	command = command + " -x -f " + temp_address + " -C " + unpack_dir;
	string output;

	int result = exec(command, output);
	filesystem::remove(temp_address);
	if (result != 0)
		return { };

	auto it = filesystem::directory_iterator(unpack_dir);
	if (!it->exists())
	{
		filesystem::remove(unpack_dir);
		return { };
	}

	ifstream infile(it->path().string(), ios::ate | ios::binary);
	if (!infile.is_open())
	{
		filesystem::remove(unpack_dir);
		return { };
	}
	size_t size = (size_t)infile.tellg();
	vector<uint8_t> content(size);
	infile.seekg(0);
	infile.read((char*)(content.data()), size);
	infile.close();
	filesystem::remove_all(unpack_dir);

	header = *((PackageHeader*)content.data());
	if (header.signature != SIGNATURE)
		return { };
	if (header.file_size != content.size())
		return { };
	if (header.version != 1)
		return { };

	return content;
}

vector<uint8_t> Package::loadData(string identifier)
{
	auto it = application_package->database.find(identifier);
	if (it != application_package->database.end())
		return it->second;
	return { };
}

void Package::storeData(string identifier, vector<uint8_t> data)
{
	application_package->database[identifier] = data;
}

vector<uint8_t> Package::tryLoadFile(string path_or_identifier)
{
	static string res_prefix = "res://";
	if (path_or_identifier.substr(0, res_prefix.size()) == res_prefix)
	{
		// load package resource
		return Package::loadData(path_or_identifier.substr(res_prefix.size()));
	}
	else
	{
		// load file data
		ifstream file(path_or_identifier, ios::ate | ios::binary);
		if (!file.is_open())
			return { };

		size_t size = (size_t)file.tellg();
		vector<uint8_t> content(size);
		file.seekg(0);
		file.read((char*)(content.data()), size);
		file.close();

		return content;
	}
}

void Package::tryWriteFile(string path, vector<uint8_t> data)
{
	ofstream file(path, ios::binary);
	if (!file.is_open())
		return;
	file.write((char*)(data.data()), data.size());
	file.close();
}

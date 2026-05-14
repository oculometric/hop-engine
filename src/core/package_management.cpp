#include "package.h"

#include <cstring>
#include <filesystem>
#include <fstream>

using namespace HopEngine;

static Package* instance = nullptr;

void Package::init()
{
    DBG_INFO("initialising package manager");
    if (instance == nullptr) instance = new Package();
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

DataBlock Package::load(const std::string& path)
{
    std::string real_path;
    if (isResPath(path, real_path))
    {
        if (!instance) Package::init();

        DBG_VERBOSE("loading '" + path + "'");
        auto [start, end] = instance->all_entries.equal_range(real_path);
        if (start == end)
        {
            DBG_ERROR("failed to load '" + path + "'");
            return {};
        }

        auto [package, entry] = (--end)->second;

        if (package == "__ANONYMOUS__") return instance->loose_entries[entry].data;
        else
        {
            auto package_it = instance->tracked_packages.find(package);
            auto entry_it   = package_it->second.second.begin() + entry;
            if (!entry_it->is_loaded && !entry_it->is_loading) queueLoad(package_it, entry_it);
            while (!entry_it->is_loaded) { _sleep(1000); }
            DataBlock result = entry_it->data;
            if (entry_it->unload_after_read)
            {
                entry_it->is_loaded         = false;
                entry_it->unload_after_read = false;
                entry_it->data.clear();
            }
            return result;
        }
    }
    else
    {
        DataBlock data;
        if (Package::readFile(path, data)) return data;
        return {};
    }
}

void Package::preload(const std::string& identifier)
{
    std::string real_path;
    if (isResPath(identifier, real_path))
    {
        if (!instance) Package::init();

        DBG_VERBOSE("preloading '" + identifier + "'");
        auto [start, end] = instance->all_entries.equal_range(real_path);
        if (start == end)
        {
            DBG_ERROR("failed to preload '" + identifier + "'");
            return;
        }

        auto [package, entry] = (--end)->second;

        if (package == "__ANONYMOUS__") return;
        else
        {
            auto package_it = instance->tracked_packages.find(package);
            auto entry_it   = package_it->second.second.begin() + entry;
            if (!entry_it->is_loaded && !entry_it->is_loading)
            {
                entry_it->unload_after_read = true;
                queueLoad(package_it, entry_it);
            }
        }
    }
    else
        DBG_WARNING("preload cannot be used for disk files.");
}

bool Package::store(const std::string& path, const DataBlock& data)
{
    std::string real_path;
    if (isResPath(path, real_path))
    {
        if (!instance) Package::init();

        DBG_VERBOSE("storing '" + path + "' (" + std::to_string(data.size()) + " bytes)");
        instance->loose_entries.emplace_back("__ANONYMOUS__", real_path, "", 0, 0, 0, true, false, 0, 0,
            false, data);
        instance->all_entries.emplace(real_path, "__ANONYMOUS__", instance->loose_entries.size() - 1);
        return true;
    }
    else
        return Package::writeFile(path, data);
}

bool Package::store(const std::string& identifier, const DataBlock& data, const std::string& author,
    uint16_t creation_year, uint8_t creation_month, uint8_t creation_day)
{
    std::string real_path;
    if (isResPath(identifier, real_path))
    {
        if (!instance) Package::init();

        DBG_VERBOSE("storing '" + identifier + "' (" + std::to_string(data.size()) + " bytes)");
        instance->loose_entries.emplace_back("__ANONYMOUS__", real_path, author, creation_year,
            creation_month, creation_day, true, false, 0, 0, false, data);
        instance->all_entries.emplace(real_path, "__ANONYMOUS__", instance->loose_entries.size() - 1);
        return true;
    }
    else
        DBG_WARNING("advanced store cannot be used for disk files.");
}



// bool Package::exportPackage(const std::string& path, bool compressed)
// {
//     DBG_VERBOSE("storing package: " + path);
//     DataBlock data = exportPackage(compressed);
//     return storeToDisk(path, data);
// }

// bool Package::importPackage(const std::string& path)
// {
//     DBG_VERBOSE("loading package: " + path);
//     DataBlock data = loadFromDisk(path);
//     if (data.empty()) return false;
//     return importPackage(data);
// }

// constexpr uint32_t SIGNATURE = 0xCA55E77E;

// struct PackageHeader
// {
//     uint32_t signature_version;
//     uint32_t file_size;
//     uint32_t alias_entries;
//     uint32_t package_entries;
// };

// struct AliasEntry
// {
//     uint32_t a_string_length;
//     uint32_t b_string_length;
// };

// struct DataBlockEntry
// {
//     uint32_t name_size;
//     uint32_t data_size;
// };

// DataBlock Package::exportPackage(bool compressed)
// {
//     DBG_VERBOSE("exporting version 4 package");
//     PackageHeader header;
//     header.signature_version = SIGNATURE + 4;
//     header.package_entries   = static_cast<uint32_t>(instance->database.size());
//     header.alias_entries     = static_cast<uint32_t>(instance->alias_table.size());
//     header.file_size         = sizeof(PackageHeader);
//     for (const auto& alias : instance->alias_table)
//         header.file_size +=
//             static_cast<uint32_t>(sizeof(AliasEntry) + alias.first.size() + alias.second.size());
//     for (const auto& block : instance->database)
//         header.file_size +=
//             static_cast<uint32_t>(sizeof(DataBlockEntry) + block.first.size() + block.second.size());

//     DataBlock data;
//     data.resize(header.file_size);
//     uint8_t* write_point = data.data();

//     memcpy(write_point, &header, sizeof(PackageHeader));
//     write_point += sizeof(PackageHeader);

//     for (const auto& alias : instance->alias_table)
//     {
//         AliasEntry entry = { static_cast<uint32_t>(alias.first.size()),
//             static_cast<uint32_t>(alias.second.size()) };
//         memcpy(write_point, &entry, sizeof(AliasEntry));
//         write_point += sizeof(AliasEntry);
//         memcpy(write_point, alias.first.data(), alias.first.size());
//         write_point += alias.first.size();
//         memcpy(write_point, alias.second.data(), alias.second.size());
//         write_point += alias.second.size();
//     }

//     for (const auto& block : instance->database)
//     {
//         DataBlockEntry entry = { static_cast<uint32_t>(block.first.size()),
//             static_cast<uint32_t>(block.second.size()) };
//         memcpy(write_point, &entry, sizeof(DataBlockEntry));
//         write_point += sizeof(DataBlockEntry);
//         memcpy(write_point, block.first.data(), block.first.size());
//         write_point += block.first.size();
//         memcpy(write_point, block.second.data(), block.second.size());
//         write_point += block.second.size();
//     }

//     DBG_INFO("stored " + std::to_string(header.package_entries) + " items to package");

//     if (compressed)
//     {
//         DBG_VERBOSE("storing compressed (version 2) package");
//         std::string command;
// #if defined(_WIN32)
//         command = "zip.exe -j ";
// #else
//         command = "zip -rj ";
// #endif

//         std::filesystem::create_directories(Package::getTempPath());
//         std::string temp_hop_address = Package::getTempPath() + "hop_package_tmp" + PTR(instance);
//         if (!storeToDisk(temp_hop_address, data))
//         {
//             DBG_ERROR("failed to store package to temp: " + temp_hop_address);
//             return data;
//         }
//         std::string temp_zip_address = temp_hop_address + ".zip";
//         command                      = command + temp_zip_address + ' ' + temp_hop_address;
//         std::string output;

//         int result = exec(command, output);
//         if (result != 0)
//         {
//             DBG_ERROR("error compressing package: " + output);
//             std::filesystem::remove(temp_hop_address);
//             return data;
//         }
//         std::filesystem::remove(temp_hop_address);

//         std::ifstream file(temp_zip_address, std::ios::ate | std::ios::binary);
//         if (!file.is_open())
//         {
//             DBG_ERROR("failed to generate compressed package; unable to open zip file");
//             std::filesystem::remove(temp_zip_address);
//             return data;
//         }

//         size_t size = static_cast<size_t>(file.tellg());
//         PackageHeader header2;
//         header2.signature_version = SIGNATURE + 2;
//         header2.package_entries   = 0;
//         header2.alias_entries     = 0;
//         header2.file_size         = sizeof(PackageHeader) + static_cast<uint32_t>(size);

//         data.resize(header2.file_size);
//         memcpy(data.data(), &header2, sizeof(PackageHeader));

//         file.seekg(0);
//         file.read(reinterpret_cast<char*>(data.data() + sizeof(PackageHeader)),
//             static_cast<std::streamsize>(size));
//         file.close();
//         std::filesystem::remove(temp_zip_address);

//         DBG_INFO("generated compressed package");
//     }

//     return data;
// }

// bool Package::importPackage(const DataBlock& data)
// {
//     if (data.size() < sizeof(PackageHeader))
//     {
//         DBG_ERROR("failed to load package; corrupted file");
//         return false;
//     }

//     PackageHeader header = *reinterpret_cast<const PackageHeader*>(data.data());
//     if (header.file_size != static_cast<uint32_t>(data.size()))
//     {
//         DBG_ERROR("failed to load package; invalid file size");
//         return false;
//     }
//     if (header.signature_version == SIGNATURE + 2)
//     {
//         DBG_VERBOSE("loading compressed package");

//         std::filesystem::create_directories(Package::getTempPath());
//         std::string temp_zip_address = Package::getTempPath() + "hop_package_tmp" + PTR(instance) + ".zip";
//         DataBlock trimmed(data.begin() + sizeof(PackageHeader), data.end());
//         if (!storeToDisk(temp_zip_address, trimmed))
//         {
//             DBG_ERROR("failed to load package; error during decompression");
//             return false;
//         }
//         trimmed.clear();

//         std::string command;
// #if defined(_WIN32)
//         command = "tar.exe -x -f ";
// #else
//         command = "unzip ";
// #endif
//         std::string unpack_dir = Package::getTempPath() + "hop";
//         std::filesystem::create_directory(unpack_dir);
//         command = command + temp_zip_address +
// #if defined(_WIN32)
//                   " -C "
// #else
//                   " -d "
// #endif
//                   + unpack_dir;
//         std::string output;

//         int result = exec(command, output);
//         std::filesystem::remove(temp_zip_address);
//         if (result != 0)
//         {
//             DBG_ERROR("error decompressing package; " + output);
//             return false;
//         }

//         auto it = std::filesystem::directory_iterator(unpack_dir);
//         if (!it->exists())
//         {
//             DBG_ERROR("error decompressing package; no package file found");
//             std::filesystem::remove(unpack_dir);
//             return false;
//         }

//         DataBlock decompressed = loadFromDisk(it->path().string());
//         std::filesystem::remove_all(unpack_dir);

//         if (!importPackage(decompressed))
//         {
//             DBG_ERROR("failed to load package; error during decompression");
//             return false;
//         }
//         return true;
//     }
//     if (header.signature_version != SIGNATURE + 4)
//     {
//         DBG_ERROR("failed to load package; invalid signature/version");
//         return false;
//     }

//     const uint8_t* data_end   = data.data() + data.size();
//     const uint8_t* read_point = data.data() + sizeof(PackageHeader);

//     std::vector<std::pair<std::string, std::string>> aliases;
//     for (uint32_t i = 0; i < header.alias_entries; ++i)
//     {
//         if (read_point + sizeof(AliasEntry) > data_end)
//         {
//             DBG_ERROR("error reading package; truncated file");
//             return false;
//         }
//         AliasEntry alias = *reinterpret_cast<const AliasEntry*>(read_point);
//         read_point += sizeof(AliasEntry);
//         if (read_point + alias.a_string_length + alias.b_string_length > data_end)
//         {
//             DBG_ERROR("error reading package; truncated file");
//             return false;
//         }
//         std::string string_a(alias.a_string_length, ' ');
//         memcpy(string_a.data(), read_point, string_a.size());
//         read_point += string_a.size();
//         std::string string_b(alias.b_string_length, ' ');
//         memcpy(string_b.data(), read_point, string_b.size());
//         read_point += string_b.size();

//         aliases.emplace_back(string_a, string_b);
//     }

//     std::vector<std::pair<std::string, DataBlock>> blocks;
//     for (uint32_t i = 0; i < header.package_entries; ++i)
//     {
//         if (read_point + sizeof(DataBlockEntry) > data_end)
//         {
//             DBG_ERROR("error reading package; truncated file");
//             return false;
//         }
//         DataBlockEntry block = *reinterpret_cast<const DataBlockEntry*>(read_point);
//         read_point += sizeof(DataBlockEntry);
//         if (read_point + block.name_size + block.data_size > data_end)
//         {
//             DBG_ERROR("error reading package; truncated file");
//             return false;
//         }
//         std::string name(block.name_size, ' ');
//         memcpy(name.data(), read_point, name.size());
//         read_point += name.size();
//         DataBlock data_block(block.data_size, ' ');
//         memcpy(data_block.data(), read_point, data_block.size());
//         read_point += data_block.size();

//         blocks.emplace_back(name, data_block);
//     }

//     instance->alias_table.insert(aliases.begin(), aliases.end());
//     instance->database.insert(blocks.begin(), blocks.end());
//     DBG_INFO("loaded " + std::to_string(header.package_entries) + " items from package");

//     return true;
// }

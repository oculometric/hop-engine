#include "package.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

using namespace HopEngine;

std::string Package::getTempPath() { return std::filesystem::temp_directory_path().string(); }

bool Package::isResPath(const std::string& path, std::string& trimmed)
{
    const static std::string res_prefix = "res://";
    if (path.starts_with(res_prefix))
    {
        trimmed = path.substr(res_prefix.size());
        return true;
    }
    return false;
}

bool Package::readFile(const std::string& path, DataBlock& result, size_t amount)
{
    DBG_VERBOSE("loading '" + path + "' from disk");
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        DBG_ERROR("failed to load '" + path + "'; file not found");
        return false;
    }

    result.clear();
    result.resize(std::min(static_cast<size_t>(file.tellg()), amount));
    file.seekg(std::ios::beg);
    file.read(reinterpret_cast<char*>(result.data()), static_cast<std::streamsize>(result.size()));
    file.close();

    return true;
}

bool Package::writeFile(const std::string& path, const DataBlock& data)
{
    DBG_VERBOSE("storing '" + path + "' to disk (" + std::to_string(data.size()) + " bytes)");
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        DBG_ERROR("failed to store '" + path + "'; file not accessible");
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    file.close();

    return true;
}

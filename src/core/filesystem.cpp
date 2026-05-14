#include "package.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <io.h>

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
    int file;
    _sopen_s(&file, path.c_str(), _O_BINARY | _O_RDONLY, _SH_DENYWR, _S_IREAD);
    if (file == -1)
    {
        DBG_ERROR("failed to load '" + path + "'; file not found");
        return false;
    }

    result.clear();
    result.resize(std::min(static_cast<size_t>(_filelengthi64(file)), amount));
    _read(file, result.data(), static_cast<unsigned int>(result.size()));
    _close(file);

    return true;
}

bool Package::writeFile(const std::string& path, const DataBlock& data)
{
    DBG_VERBOSE("storing '" + path + "' to disk (" + std::to_string(data.size()) + " bytes)");
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    int file;
    _sopen_s(&file, path.c_str(), _O_CREAT | _O_BINARY | _O_TRUNC | _O_WRONLY, _SH_DENYRD, _S_IWRITE);
    if (file == -1)
    {
        DBG_ERROR("failed to store '" + path + "'; file not accessible");
        return false;
    }

    _write(file, data.data(), static_cast<unsigned int>(data.size()));
    _close(file);

    return true;
}

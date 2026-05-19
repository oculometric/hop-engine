#include "package.h"

#include <algorithm>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#if defined(_WIN32)
#include <io.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

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
#if defined(_WIN32)
    _sopen_s(&file, path.c_str(), _O_BINARY | _O_RDONLY, _SH_DENYWR, _S_IREAD);
#else
    file = open(path.c_str(), O_RDONLY);
#endif
    if (file == -1)
    {
        DBG_ERROR("failed to load '" + path + "'; file not found");
        return false;
    }

    result.clear();
#if defined(_WIN32)
    size_t file_size = static_cast<size_t>(_filelengthi64(file));
#else
    struct stat file_stat;
    fstat(file, &file_stat);
    size_t file_size = static_cast<size_t>(file_stat.st_size);
#endif
    result.resize(std::min(file_size, amount));
#if defined(_WIN32)
    _read(file, result.data(), static_cast<unsigned int>(result.size()));
    _close(file);
#else
    read(file, result.data(), result.size());
    close(file);
#endif

    return true;
}

bool Package::writeFile(const std::string& path, const DataBlock& data)
{
    DBG_VERBOSE("storing '" + path + "' to disk (" + std::to_string(data.size()) + " bytes)");
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    int file;
#if defined(_WIN32)
    _sopen_s(&file, path.c_str(), _O_CREAT | _O_BINARY | _O_TRUNC | _O_WRONLY, _SH_DENYRD, _S_IWRITE);
#else
    file = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
    if (file == -1)
    {
        DBG_ERROR("failed to store '" + path + "'; file not accessible");
        return false;
    }

#if defined(_WIN32)
    _write(file, data.data(), static_cast<unsigned int>(data.size()));
    _close(file);
#else
    write(file, data.data(), data.size());
    close(file);
#endif

    return true;
}

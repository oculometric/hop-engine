#include "package.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

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

Package* Package::getInstance()
{
    if (!instance) Package::init();
    return instance;
}

DataBlock Package::load(const std::string& path)
{
    std::string real_path;
    if (isResPath(path, real_path))
    {
        DBG_VERBOSE("loading '" + path + "'");
        auto [start, end] = getInstance()->all_entries.equal_range(real_path);
        if (start == end)
        {
            DBG_ERROR("failed to load '" + path + "'");
            return {};
        }

        auto [package, entry] = (--end)->second;

        if (package == "__ANONYMOUS__") return getInstance()->loose_entries[entry].data;
        else
        {
            auto package_it = getInstance()->tracked_packages.find(package);
            auto entry_it   = package_it->second.second.begin() + entry;
            if (!entry_it->is_loaded && !entry_it->is_loading) queueLoad(package_it, entry_it);
            while (!entry_it->is_loaded) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
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
        DBG_VERBOSE("preloading '" + identifier + "'");
        auto [start, end] = getInstance()->all_entries.equal_range(real_path);
        if (start == end)
        {
            DBG_ERROR("failed to preload '" + identifier + "'");
            return;
        }

        auto [package, entry] = (--end)->second;

        if (package == "__ANONYMOUS__") return;
        else
        {
            auto package_it = getInstance()->tracked_packages.find(package);
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
        DBG_VERBOSE("storing '" + path + "' (" + std::to_string(data.size()) + " bytes)");
        getInstance()->loose_entries.emplace_back("__ANONYMOUS__", real_path, "", 0, 0, 0, true, false, 0,
            0, false, data);
        getInstance()->all_entries.insert({
            real_path, EntryPointer{ "__ANONYMOUS__", getInstance()->loose_entries.size() - 1 }
        });
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
        DBG_VERBOSE("storing '" + identifier + "' (" + std::to_string(data.size()) + " bytes)");
        getInstance()->loose_entries.emplace_back("__ANONYMOUS__", real_path, author, creation_year,
            creation_month, creation_day, true, false, 0, 0, false, data);
        getInstance()->all_entries.insert({
            real_path, EntryPointer{ "__ANONYMOUS__", getInstance()->loose_entries.size() - 1 }
        });
        return true;
    }
    else
        DBG_WARNING("advanced store cannot be used for disk files.");
    return false;
}

Package::Package()
{
    instance          = this;
    background_thread = new std::thread(Package::packageBackgroundMain);
}

Package::~Package()
{
    background_thread_exit = true;
    background_thread->join();
}

void Package::queueLoad(PackageMap::iterator package_it, EntryList::iterator entry_it)
{
    const std::lock_guard lock(getInstance()->database_mutex);
    if (entry_it->is_loaded || entry_it->is_loading) return;
    entry_it->is_loading = true;
    entry_it->data.resize(entry_it->data_size);
    getInstance()->load_queue.emplace(package_it->second.first, entry_it->data_offset, entry_it->data_size,
        package_it->first, std::distance(std::begin(package_it->second.second), entry_it),
        entry_it->data.data());
}

void Package::packageBackgroundMain()
{
    while (!getInstance()->background_thread_exit)
    {
        if (getInstance()->load_queue.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        LoadCommand command = getInstance()->load_queue.front();
        getInstance()->load_queue.pop();

        command.file->seekg(static_cast<std::streampos>(command.offset));
        command.file->read(reinterpret_cast<char*>(command.data_pointer), command.size);

        auto& entry      = getInstance()->tracked_packages[command.package_name].second[command.entry];
        entry.is_loaded  = true;
        entry.is_loading = false;
    }
}

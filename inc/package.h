/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <thread>
#include <mutex>

namespace HopEngine
{

/**
 * @brief singleton class providing various functionality for loading files, loading packaged data
 * files, and loading data blocks from those packages.
 */
class Package final
{
    friend class InitMachine;
public:
    struct Selector
    {
        bool allow_resources_from_packages  = false;
        bool allow_loose_resources          = true;
        std::string package_selection_regex = ".*";
        std::string entry_selection_regex   = ".*";
    };

private:
    struct Entry
    {
        std::string owning_package;
        std::string identifier;
        std::string author;
        uint16_t creation_date_year;
        uint8_t creation_date_months;
        uint8_t creation_date_days;
        bool is_loaded;
        bool is_loading;
        uint32_t data_size;
        uint32_t data_offset;
        bool unload_after_read;
        DataBlock data;
    };

    typedef std::pair<std::string, size_t> EntryPointer;
    typedef std::vector<Entry> EntryList;
    typedef std::map<std::string, std::pair<std::ifstream, EntryList>> PackageMap;

    struct LoadCommand
    {
        std::ifstream* file;
        uint32_t offset;
        uint32_t size;
        std::string package_name;
        size_t entry;
        void* data_pointer;
    };

private:
    // maps entry names to a package which contains them and the index of the entry in the entry table
    std::multimap<std::string, EntryPointer> all_entries;
    // maps package names to their associated file handles and entry tables
    PackageMap tracked_packages;
    EntryList loose_entries;
    std::queue<LoadCommand> load_queue;
    bool background_thread_exit = false;
    std::thread* background_thread = nullptr;
    std::mutex database_mutex;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(Package);

    // fetches the data from a file (path) or an entry loaded from a package (identifier)
    static DataBlock load(const std::string& path_or_identifier);
    // marks an entry from a tracked package to be loaded asynchronously in the background
    static void preload(const std::string& identifier);
    // stores data to a file (path) or an entry to the loose package (identifier)
    static bool store(const std::string& path_or_identifier, const DataBlock& data);
    // stores a data block to the loose package with extra information
    static bool store(const std::string& identifier, const DataBlock& data, const std::string& author,
        uint16_t creation_year, uint8_t creation_month, uint8_t creation_day);

    // encode currently loaded entries into a new package based on a selector
    static DataBlock encodePackage(const std::string& author, const Selector& selector,
        uint16_t creation_year, uint8_t creation_month, uint8_t creation_day);

    // pulls in a package from memory, loading everything immediately
    static bool importPackage(const DataBlock& data);
    // frees a package (and all entries) loaded using importpackage
    static bool releasePackage(const DataBlock& data);
    // starts tracking a package file by loading just its index
    static bool importDeferredPackage(const std::string& path);
    // stops tracking a package file
    static bool releaseDeferredPackage(const std::string& path);

    /**
     * @brief gives a platform-specific temporary path, mostly used internally for unzipping files.
     * @returns path to the appropriate temporary directory for the current platform.
     */
    static std::string getTempPath();

private:
    Package();
    ~Package();

    static void init();
    static void destroy();

    static Package* getInstance();

    /**
     * @brief checks if the specified path has the `res://` prefix.
     * @param path path or identifier to check.
     * @param trimmed location to output the identifier without the `res://` prefix, if the prefix was
     * found.
     * @returns `true` if `path` was res-relative (i.e. it was prefixed with `res://`), otherwise
     * `false`.
     */
    static bool isResPath(const std::string& path, std::string& trimmed);

    static bool readFile(const std::string& path, DataBlock& result, size_t amount = SIZE_MAX);
    static bool writeFile(const std::string& path, const DataBlock& data);

    static void queueLoad(PackageMap::iterator package_it, EntryList::iterator entry_it);
    static void packageBackgroundMain();
};

} // namespace HopEngine

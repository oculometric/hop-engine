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
#include <fstream>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

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
    /**
     * @brief describes a pattern for selecting package entries from the database.
     */
    struct Selector
    {
        bool allow_resources_from_packages  = false;
        bool allow_loose_resources          = true;
        std::string package_selection_regex = ".*";
        std::string entry_selection_regex   = ".*";
    };

private:
    /**
     * @brief in-memory description of a package entry. the entry may or may not actually be loaded.
     */
    struct Entry
    {
        std::string owning_package;   // file path of the package this entry belongs to
        std::string identifier;       // string name of the entry
        std::string author;           // name of the author of the entry
        uint16_t creation_date_year;  // numerical year of authoring
        uint8_t creation_date_months; // numerical month of authoring
        uint8_t creation_date_days;   // numerical day (of the month) of authoring
        bool is_loaded;               // if `true` the entry's data is currently stored in `data`
        bool is_loading;              // if `true` the entry is already queued for load
        uint32_t data_size;           // size of the data block in the source
        uint32_t data_offset;         // offset of the data block start in the source
        bool unload_after_read;       // if `true` the entry will be unloaded again after being read
        DataBlock data;               // possibly unpopulated, data actually contained in the entry
    };

    typedef std::pair<std::string, size_t> EntryPointer;
    typedef std::vector<Entry> EntryList;
    typedef std::map<std::string, std::pair<std::ifstream*, EntryList>> PackageMap;

    /**
     * @brief internal struct for tracking package entries queued to be loaded.
     */
    struct LoadCommand
    {
        std::ifstream* file;      // file handle to read from
        uint32_t offset;          // offset of the start of the data in the file
        uint32_t size;            // size of the data to be read
        std::string package_name; // name of the package being loaded for
        size_t entry;             // index of the entry being loaded within the package
        void* data_pointer;       // pointer to where to put the data when it's loaded
    };

private:
    // maps entry names to a package which contains them and the index of the entry in the entry table
    std::multimap<std::string, EntryPointer> all_entries;
    PackageMap tracked_packages; // maps package names to their associated file handles and entry tables
    EntryList loose_entries;     // default package for 'loose' entries not part of a tracked package file
    std::queue<LoadCommand> load_queue;       // queued load instructions for resources
    bool background_thread_exit    = false;   // exit condition for background loader thread
    std::thread* background_thread = nullptr; // background loader thread
    // mutex for synchronising against modifications to `tracked_packages` or `loose_entries`
    std::mutex database_mutex;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(Package);

    /**
     * @brief fetches the data from a file (path) or an entry available from a currently loaded package
     * (identifier). identifiers are specified by prefixing the path with `res://`, in which case the
     * package manager will search the currently loaded packages for a data block whose identifier matches.
     * if the entry is found in a package loaded with `importPackage`, the data is returned immediately. if
     * the entry is found within a package loaded with `importDeferredPackage`, and `preload` has not been
     * called for the entry, then you may have to wait for the resource to be loaded from disk.
     * @param path_or_identifier either the filesystem path to the file to be loaded, or the identifier of a
     * packaged resource prefixed with `res://`.
     * @returns byte array containing all the data from the target file/data block; empty if the file
     * could not be found and/or read, or if no data block was found in the currently loaded packages
     * with a matching identifier.
     */
    static DataBlock load(const std::string& path_or_identifier);
    /**
     * @brief marks an entry from a currently loaded package to be loaded asynchronously in the background.
     * once loaded, the entry will remain in memory until `load` is called for it, when it will be released.
     * should be used when preparing the next scene, to load resources you know will be needed soon.
     * @param identifier the identifier of a packaged resource prefixed with `res://`.
     */
    static void preload(const std::string& identifier);
    /**
     * @brief stores data to a file (path) or a package entry (identifier). identifiers are specified by
     * prefixing the path with `res://`, in which case the package manager will add the entry to the
     * anonymous (loose) package.
     * @param path_or_identifier either the filesystem path to the file to be written, or the identifier for
     * the package entry prefixed with `res://`.
     * @param data byte array to be written.
     * @returns `true` if the write was successful, or `false` if there was an error.
     */
    static bool store(const std::string& path_or_identifier, const DataBlock& data);
    /**
     * @brief stores data to a package entry with some extra metadata. the package manager will add the
     * entry to the anonymous (loose) package.
     * @param path_or_identifier the identifier for the package entry prefixed with `res://`.
     * @param data byte array to be written.
     * @param author name of the file author, if appropriate.
     * @param creation_year numerical year in which the file was authored.
     * @param creation_month numerical month in which the file was authored.
     * @param creation_day numerical day (of the month) on which the file was authored.
     * @returns `true` if the write was successful, or `false` if there was an error.
     */
    static bool store(const std::string& identifier, const DataBlock& data, const std::string& author,
        uint16_t creation_year, uint8_t creation_month, uint8_t creation_day);

    // encode currently loaded entries into a new package based on a selector
    /**
     * @brief stores a selection of currently loaded packages into a new package file.
     * @param author name of the package author, if appropriate. probably put your copyright message here.
     * @param selector information about which entries should be included in the new package. entries loaded
     * by `importDeferredPackage` are loaded during this function call.
     * @param creation_year numerical year in which the package was authored.
     * @param creation_month numerical month in which the package was authored.
     * @param creation_day numerical day (of the month) on which the package was authored.
     * @returns byte array representing encoded package file.
     */
    static DataBlock encodePackage(const std::string& author, const Selector& selector,
        uint16_t creation_year, uint8_t creation_month, uint8_t creation_day);

    /**
     * @brief loads a package file from data stored in memory. all entries will be loaded immediately and
     * stored into the anonymous (loose) package.
     * @param data binary byte data representing the package file.
     * @returns `false` if an error occurred during decoding, otherwise `true`.
     */
    static bool importPackage(const DataBlock& data);
    /**
     * @brief loads the index only from a package file from data stored on disk. all entries will be added
     * to the database in a package with a name matching `path`, but their data will only be loaded from the
     * original file on disk when actually needed.
     * @param path path to the target package file.
     * @returns `false` if an error occurred during decoding, otherwise `true`.
     */
    static bool importDeferredPackage(const std::string& path);
    /**
     * @brief unloads all entries loaded in a package with a name matching `path`, and closes the associated
     * file handle.
     * @param path path to the target package file.
     * @returns `false` if there was no matching package loaded, otherwise `true`.
     */
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

    /**
     * @brief loads the contents of a file from disk using direct IO.
     * @param path path to the file to load.
     * @param result byte array where the output will be delivered.
     * @param amount maximum number of bytes to read.
     * @returns `true` if the file was read successfully, otherwise `false`.
     */
    static bool readFile(const std::string& path, DataBlock& result, size_t amount = SIZE_MAX);
    /**
     * @brief writes data to a file to disk using direct IO.
     * @param path path to the file to write. intermediate directories are created automatically.
     * @param data byte array to be stored.
     * @returns `true` if the file was written successfully, otherwise `false`.
     */
    static bool writeFile(const std::string& path, const DataBlock& data);

    /**
     * @brief queues a package entry to be loaded asynchronously, unless it is already loaded or currently
     * loading.
     * @param package_it iterator into the tracked package map for the package to which the entry belongs.
     * @param entry_it iterator into the entry list within the specified tracked package.
     */
    static void queueLoad(PackageMap::iterator package_it, EntryList::iterator entry_it);
    /**
     * @brief mainloop function for the background async loading thread.
     */
    static void packageBackgroundMain();
};

} // namespace HopEngine

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

namespace HopEngine
{

/**
 * @brief singleton class providing various functionality for loading files, loading packaged data
 * files, and loading data blocks from those packages.
 */
class Package final
{
private:
    std::map<std::string, DataBlock> database; // map of all currently loaded data blocks
    std::map<std::string, std::string> alias_table;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(Package);

    static void init();
    static void destroy();

    /**
     * @brief loads a block of data from the specified path. may be a res-relative package path or an
     * external filesystem path. if `path` begins with `"res://"`, the package manager will search the
     * currently loaded packages for a data block whose identifier matches the rest of `path`,
     * otherwise, `loadFromDisk` is invoked and the package manager will attempt to read the entire
     * contents of the file at `path` from disk.
     * @param path identifier/path of the target file/data block.
     * @returns byte array containing all the data from the target file/data block; empty if the file
     * could not be found and/or read, or if no data block was found in the currently loaded packages
     * with a matching identifier.
     */
    static DataBlock load(const std::string& path);
    /**
     * @brief loads the entire contents of the specified file path on disk.
     * @param path path of the target file.
     * @returns byte array containing all the data from the target file; empty if the file could not be
     * found and/or read.
     */
    static DataBlock loadFromDisk(const std::string& path);
    /**
     * @brief stores a block of data to the specified path. may be a res-relative package path or an
     * external filesystem path. if `path` begins with `"res://"`, the package manager will insert the
     * data block into the active package registry, overwriting existing data if a data block with the
     * same identifier already exists, otherwise `storeToDisk` is invoked and the package manager will
     * attempt to write the data to the file at `path` on disk (creating or overwriting as needed).
     * @param path identifier/path of the target file/data block.
     * @param data byte array to be written to the file/data block registry.
     * @returns `true` if the operation was successful, otherwise `false`.
     */
    static bool store(const std::string& path, const DataBlock& data);
    /**
     * @brief stores the bytes in a byte array to the specified file path on disk. creates the file if
     * necessarry, and will overwrite existing data if the file already exists. does not create
     * intermediate directories to the specified path.
     * @param path of the target file.
     * @param byte array to be written to the file.
     * @returns `true` if the data was written successfully, or `false` if the file could not be
     * created/modified or one or more intermediate directories were missing.
     */
    static bool storeToDisk(const std::string& path, const DataBlock& data);

    /**
     * @brief reads a hop-engine package file as a byte array and loads all the data blocks contained
     * inside it into the data block registry. automatically detects if the package is compressed, and
     * decompresses it if needed.
     * @param data byte array containing the package file.
     * @returns `true` if the package was loaded successfully, or `false` if there was an error during
     * package extraction (may be due to file corruption, incomplete data, or inability to decompress).
     */
    static bool importPackage(const DataBlock& data);
    /**
     * @brief reads a hop-engine package file from disk and loads all the data blocks contained inside
     * it into the data block registry via `importPackage`.
     * @param path path to the target package file. the default extension for hop-engine package files
     * is `*.hop`.
     * @returns `true` if the package was loaded successfully, or `false` if there was an error during
     * file reading or package extraction (see `loadFromDisk` and `importPackage`).
     */
    static bool importPackage(const std::string& path);
    /**
     * @brief collects all currently loaded data blocks and writes them out to a package file in the
     * form of a byte array. optionally compresses the exported package with ZIP compression.
     * compression usually reduces size by about 20-50%, although it will increase load time and memory
     * usage when importing.
     * @param compressed if `true` enables ZIP compression for the package file.
     * @returns `true` if the package file was built successfully, or `false` if there was an error
     * during encoding (may be due to inability to compress).
     */
    static DataBlock exportPackage(bool compressed = false);
    /**
     * @brief collects all currently loaded data blocks and writes them out to a package file specified
     * by `path`. optionally compresses the exported package with ZIP compression. compression usually
     * reduces size by about 20-50%, although it will increase load time and memory usage when
     * importing.
     * @param path file path where the final package should be written. the default extension for
     * hop-engine package files is `*.hop`.
     * @param compressed if `true` enables ZIP compression for the package file.
     * @returns `true` if the package file was built and written successfully, or `false` if the file
     * could not be written for some reason (see `storeToDisk` and `exportPackage`).
     */
    static bool exportPackage(const std::string& path, bool compressed = false);

    /**
     * @brief lists the identifiers of all data blocks currently loaded in the registry.
     * @returns `vector` where each element is the identifier of a data block.
     */
    static std::vector<std::string> listLoadedEntries();
    /**
     * @brief inserts an alias between two idenfitiers for a data block in the registry. allows requests
     * for a data block named `a` to be instead directed to read from `b` instead. for example, when
     * `Package::load("res://identifier_a")` is executed, the result of
     * `Package::load("res://identifier_b")` will be returned instead.
     * @param a identifier to be redirected from.
     * @param b identifier to redirect to, when `a` is passed to functions.
     */
    static void setAlias(const std::string& a, const std::string& b);
    /**
     * @brief clears the alias from an identifier, meaning calls like
     * `Package::load("res://identifier_a")` will no longer be redirected to another identifier.
     * @param a identifier to clear redirections from.
     */
    static void clearAlias(const std::string& a);

    /**
     * @brief gives a platform-specific temporary path, mostly used internally for unzipping files.
     * @returns path to the appropriate temporary directory for the current platform.
     */
#if defined(_WIN32)
    static inline std::string getTempPath() { return "C:/tmp/"; }
#else
    static inline std::string getTempPath() { return "/tmp/"; }
#endif

private:
    Package()  = default;
    ~Package() = default;

    /**
     * @brief checks if the specified path has the `res://` prefix.
     * @param path path or identifier to check.
     * @param trimmed location to output the identifier without the `res://` prefix, if the prefix was
     * found.
     * @returns `true` if `path` was res-relative (i.e. it was prefixed with `res://`), otherwise
     * `false`.
     */
    static bool isResPath(const std::string& path, std::string& trimmed);
};

} // namespace HopEngine

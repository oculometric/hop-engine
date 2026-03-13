#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

#include "common.h"

namespace HopEngine
{

typedef std::vector<uint8_t> DataBlock;

class Package final
{
private:
	std::map<std::string, std::vector<uint8_t>> database;
	std::map<std::string, std::string> alias_table;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Package);

	static void init();
	static void destroy();

	static DataBlock load(const std::string& path);
	static DataBlock loadFromDisk(const std::string& path);
	static bool store(const std::string& path, const DataBlock& data);
	static bool storeToDisk(const std::string& path, const DataBlock& data);

	static bool exportPackage(const std::string& path, bool compressed = false);
	static bool importPackage(const std::string& path);
	static DataBlock exportPackage(bool compressed = false);
	static bool importPackage(const DataBlock& data);

	static std::vector<std::string> listLoadedEntries();
	static void setAlias(const std::string& a, const std::string& b);
	static void clearAlias(const std::string& a);

#if defined(_WIN32)
	static inline std::string getTempPath() { return "C:/tmp/"; }
#else
	static inline std::string getTempPath() { return "/tmp/"; }
#endif

private:
	Package() = default;
	~Package() = default;
	
	static bool isResPath(const std::string& path, std::string& trimmed);
};

}

#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

#include "common.h"

namespace HopEngine
{

class Package
{
private:
	std::map<std::string, std::vector<uint8_t>> database;
	std::map<std::string, std::string> alias_table;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Package);

	static void init();
	static void destroy();

#if defined(_WIN32)
	static inline std::string getTempPath() { return "C:/tmp/"; }
#else
	static inline std::string getTempPath() { return "/tmp/"; }
#endif
	static bool loadPackageFromMemory(std::vector<uint8_t>& content, const std::string& load_path);
	static bool loadPackage(const std::string& load_path);
	static std::vector<std::string> listLoadedEntries();
	static std::vector<uint8_t> loadData(const std::string& identifier);
	static std::vector<uint8_t> tryLoadFile(const std::string& path_or_identifier);
	static bool storePackage(const std::string& store_path);
	static bool storeCompressedPackage(const std::string& store_path);
	static void storeData(const std::string& identifier, const std::vector<uint8_t>& data);
	static void tryWriteFile(const std::string& path, const std::vector<uint8_t>& data);
	static void setAlias(const std::string& a, const std::string& b);
	static void clearAlias(const std::string& a);

private:
	Package() = default;
	~Package();
	
	static std::vector<uint8_t> loadCompressedPackage(const std::vector<uint8_t>& data);
};

}

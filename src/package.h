#pragma once

#include <string>
#include <vector>
#include <map>

#include "common.h"

namespace HopEngine
{

class Package
{
private:
	std::map<std::string, std::vector<uint8_t>> database;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Package);

	static void init();
	static Package* get();

	static bool loadPackage(std::string load_path);
	static bool storePackage(std::string store_path);
	static bool storeCompressedPackage(std::string store_path);
	static std::vector<uint8_t> loadData(std::string identifier);
	static void storeData(std::string identifier, std::vector<uint8_t> data);
	static std::vector<uint8_t> tryLoadFile(std::string path_or_identifier);
	static void tryWriteFile(std::string path, std::vector<uint8_t> data);
	static inline std::string getTempPath() { return "C:/tmp/"; }

private:
	Package();
	~Package();

	static std::vector<uint8_t> loadCompressedPackage(std::vector<uint8_t> data);
};

}

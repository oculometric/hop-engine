#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>

#include "../package.h"

using namespace std;

vector<uint8_t> readFile(string path)
{
	ifstream file(path, ios::ate | ios::binary);
	if (!file.is_open())
		return { };

	size_t size = (size_t)file.tellg();
	vector<uint8_t> content(size);
	file.seekg(0);
	file.read((char*)(content.data()), size);
	file.close();

	return content;
}

int main(const int nargs, const char** vargs)
{
	HopEngine::Debug::init(HopEngine::Debug::DEBUG_FAULT);
	if (nargs < 2)
	{
		cout << "usage: package-builder SOURCE_DIRECTORY [options] [OUTPUT_FILE]" << endl;
		cout << "options: -c (compress output)" << endl;
		cout << "if OUTPUT_FILE is not specified, 'resources.hop'";
		return -1;
	}

	bool compressed = false;
	string target_dir = vargs[1];
	string output_hop = "resources.hop";

	if (nargs == 3)
	{
		if (string(vargs[2]) == "-c")
			compressed = true;
		else
			output_hop = vargs[2];
	}

	if (nargs == 4)
	{
		if (string(vargs[2]) == "-c")
			compressed = true;
		else
		{
			cout << "invalid option '" << vargs[2] << '\'' << endl;
			return -1;
		}
		output_hop = vargs[3];
	}

	HopEngine::Package::init();
	size_t entries = 0;
	for (const auto& p : filesystem::recursive_directory_iterator(target_dir))
	{
		if (!filesystem::is_directory(p))
		{
			string path = p.path().string();
			string identifier = path.substr(target_dir.size() + 1);
			for (char& c : identifier)
				if (c == '\\')
					c = '/';
			HopEngine::Package::storeData(identifier, readFile(path));
			++entries;
		}
	}

	if (compressed)
		HopEngine::Package::storeCompressedPackage(output_hop);
	else
		HopEngine::Package::storePackage(output_hop);

	return 0;
}

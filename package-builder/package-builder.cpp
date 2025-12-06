#include <fstream>
#include <iostream>
#include <filesystem>

#include "../src/package.h"

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
	if (nargs < 2)
	{
		cout << "usage: package-builder SOURCE_DIRECTORY [OUTPUT_FILE]" << endl;
		return -1;
	}

	string target_dir = vargs[1];
	string output_hop = "resources.hop";

	if (nargs > 2)
		output_hop = vargs[2];

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
			cout << "storing '" << path << "' as '" << identifier << "'" << endl;
			HopEngine::Package::storeData(identifier, readFile(path));
			++entries;
		}
	}

	cout << "stored " << entries << " package entries." << endl;
	cout << "writing to '" << output_hop << "'...";
	HopEngine::Package::storePackage(output_hop);
	cout << "done." << endl;

	return 0;
}

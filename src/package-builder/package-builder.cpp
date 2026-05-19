#include "mesh.h"
#include "package.h"

#include <base64/base64.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

vector<string>::iterator findArg(string s, vector<string>& v)
{
    auto it = v.begin();
    while (*it != s && it != v.end()) ++it;
    return it;
}

HopEngine::Package* package = nullptr;
HopEngine::Debug* debug     = nullptr;

class HopEngine::InitMachine final
{
public:
    static void initialise()
    {
        bool success;
        debug = reinterpret_cast<HopEngine::Debug*>(malloc(sizeof(HopEngine::Debug)));
        new (debug) HopEngine::Debug({}, success);
        package = reinterpret_cast<HopEngine::Package*>(malloc(sizeof(HopEngine::Package)));
        new (package) HopEngine::Package({}, success);
    }

    static void destroy() {}
};

// TODO: improve this to be much more advanced, multiple files/folders to specify, set root, etc
int main(const int nargs, const char** vargs)
{
    HopEngine::InitMachine::initialise();

    if (nargs < 2)
    {
        cout << "usage: package-builder SOURCE_DIRECTORY [options] [OUTPUT_FILE]" << endl;
        cout << "options: -c (compress output)" << endl;
        cout << "         -p (add prefix to all identifiers)" << endl;
        cout << "         -b (encode files as binary versions if possible)" << endl;
        cout << "if OUTPUT_FILE is not specified, 'resources.hop'";
        return -1;
    }

    bool compressed   = false;
    bool binary       = false;
    string target_dir = vargs[1];
    string output_hop = "resources.hop";
    string path_prefix;

    vector<string> args;
    for (int i = 0; i < nargs; ++i) args.push_back(vargs[i]);

    auto it = findArg("-c", args);
    if (it != args.end()) compressed = true;
    it = findArg("-b", args);
    if (it != args.end()) binary = true;
    it = findArg("-p", args);
    if (it != args.end())
    {
        if (it + 1 == args.end()) cout << "-p option requires a string following it" << endl;
        path_prefix = *(it + 1);
    }
    if (args[args.size() - 1][0] != '-') output_hop = args[args.size() - 1];

    size_t entries = 0;
    for (const auto& p : filesystem::recursive_directory_iterator(target_dir))
    {
        if (!filesystem::is_directory(p))
        {
            string path       = p.path().string();
            string identifier = path.substr(target_dir.size() + 1);
            for (char& c : identifier)
                if (c == '\\') c = '/';
            if (binary && p.path().extension().string() == ".obj")
            {
                HopEngine::Package::store("res://" + path_prefix + identifier,
                    HopEngine::Mesh::convertToBinaryMesh(path));
            }
            else
                HopEngine::Package::store("res://" + path_prefix + identifier,
                    HopEngine::Package::load(path));
            ++entries;
        }
    }

    auto result = HopEngine::Package::encodePackage("cassette costen", {}, 2026, 5, 14);
    return !HopEngine::Package::store(output_hop, result);
}

HopEngine::Debug* HopEngine::Debug::getInstance() { return debug; }
HopEngine::Package* HopEngine::Package::getInstance() { return package; }

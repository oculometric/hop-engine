#include <format>
#include <fstream>
#include <iostream>

int main(const int nargs, const char** vargs)
{
    std::cout << "translating " << vargs[1] << " to CPP format in " << vargs[2] << ", named " << vargs[3] << std::endl;
    std::ifstream in_file;
    in_file.open(vargs[1], std::ios::binary);

    size_t size = 0;

    std::ofstream out_file;
    out_file.open(vargs[2]);

    out_file << "unsigned char " << vargs[3] << "[] = {";

    while (in_file.peek() != EOF)
    {
        char c;
        in_file.read(&c, 1);
        out_file << std::format("{:#04x},", (int)(unsigned char)c);
        ++size;
    }

    out_file << "};\n";

    out_file << std::format("unsigned long long {}_size = {}ull;", vargs[3], size);

    in_file.close();
    out_file.close();

    return 0;
}
#include "package.h"

using namespace HopEngine;

static constexpr uint32_t SIGNATURE = 0xB679AAB2;

struct PackageFileHeader
{
    uint32_t signature;
    uint32_t file_size;
    uint8_t version;
    uint8_t padding;
    uint16_t entry_table_count;
    uint32_t entry_table_offset;
    uint16_t creation_date_year;
    uint8_t creation_date_months;
    uint8_t creation_date_days;
    uint16_t author_str_size;
};

struct EntryFileHeader
{
    uint16_t identifier_str_size;
    uint16_t author_str_size;
    uint16_t creation_date_year;
    uint8_t creation_date_months;
    uint8_t creation_date_days;
    uint32_t data_size;
    uint32_t data_offset;
};

template<typename T> void insertData(DataBlock& result, T& data, size_t& offset)
{
    memcpy(result.data() + offset, &data, sizeof(T));
    offset += sizeof(T);
}

DataBlock Package::encodePackage(const std::string& author, const Selector& selector)
{
    PackageHeader package_header;
    package_header.signature = SIGNATURE;
    package_header.version   = 1;
    package_header.padding   = 0xFF;

    DataBlock result;

    // TODO: insert all the other data

    insertData(result);
}
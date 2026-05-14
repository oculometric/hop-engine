#include "package.h"

#include <fstream>
#include <regex>

using namespace HopEngine;

static constexpr uint32_t SIGNATURE    = 0xB679AAB2;
static constexpr size_t BYTE_ALIGNMENT = 4;

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

size_t align(size_t original) { return (original % BYTE_ALIGNMENT) ? original + BYTE_ALIGNMENT : original; }

DataBlock Package::encodePackage(const std::string& author, const Selector& selector,
    uint16_t creation_year, uint8_t creation_month, uint8_t creation_day)
{
    std::vector<std::pair<const Entry*, std::ifstream*>> entries_to_pack;

    std::regex entry_regex(selector.entry_selection_regex);
    std::regex package_regex(selector.package_selection_regex);

    if (selector.allow_loose_resources)
    {
        for (const auto& entry : getInstance()->loose_entries)
        {
            if (std::regex_match(entry.identifier, entry_regex))
                entries_to_pack.emplace_back(&entry, nullptr);
        }
    }
    if (selector.allow_resources_from_packages)
    {
        for (auto& package : getInstance()->tracked_packages)
        {
            if (!std::regex_match(package.first, package_regex)) continue;

            for (const auto& entry : package.second.second)
            {
                if (std::regex_match(entry.identifier, entry_regex))
                    entries_to_pack.emplace_back(&entry, &package.second.first);
            }
        }
    }

    PackageFileHeader package_header;
    package_header.signature            = SIGNATURE;
    package_header.version              = 1;
    package_header.padding              = 0xFF;
    package_header.entry_table_count    = static_cast<uint16_t>(entries_to_pack.size());
    package_header.entry_table_offset   = static_cast<uint32_t>(align(sizeof(PackageFileHeader)));
    package_header.creation_date_year   = creation_year;
    package_header.creation_date_months = creation_month;
    package_header.creation_date_days   = creation_day;
    package_header.file_size            = static_cast<uint32_t>(align(sizeof(PackageFileHeader)));

    // TODO: compress each data entry! not the whole package

    size_t entry_table_length = 0;
    for (const auto& [entry, file] : entries_to_pack)
    {
        entry_table_length += align(sizeof(EntryFileHeader));
        entry_table_length += align(entry->identifier.size());
        entry_table_length += align(entry->author.size());
        package_header.file_size += static_cast<uint32_t>(align(entry->is_loaded ? entry->data.size() : entry->data_size));
    }
    package_header.file_size += static_cast<uint32_t>(entry_table_length);

    DataBlock result;
    result.resize(package_header.file_size);
    memcpy(result.data(), &package_header, sizeof(PackageFileHeader));
    size_t offset      = package_header.entry_table_offset;
    size_t data_offset = offset + entry_table_length;
    for (auto& [entry, file] : entries_to_pack)
    {
        EntryFileHeader* entry_header      = reinterpret_cast<EntryFileHeader*>(result.data() + offset);
        entry_header->identifier_str_size  = static_cast<uint16_t>(align(entry->identifier.size()));
        entry_header->author_str_size      = static_cast<uint16_t>(align(entry->author.size()));
        entry_header->creation_date_year   = entry->creation_date_year;
        entry_header->creation_date_months = entry->creation_date_months;
        entry_header->creation_date_days   = entry->creation_date_days;
        entry_header->data_size   = static_cast<uint32_t>(align(entry->is_loaded ? entry->data.size() : entry->data_size));
        entry_header->data_offset = static_cast<uint32_t>(data_offset);
        offset += align(sizeof(EntryFileHeader));

        memcpy(result.data() + offset, entry->identifier.data(), entry->identifier.size());
        offset += entry_header->identifier_str_size;

        memcpy(result.data() + offset, entry->author.data(), entry->author.size());
        offset += entry_header->author_str_size;

        if (entry->is_loaded) memcpy(result.data() + data_offset, entry->data.data(), entry->data.size());
        else
        {
            file->seekg(static_cast<std::streampos>(entry->data_offset));
            file->readsome(reinterpret_cast<char*>(result.data() + data_offset), entry->data_size);
        }
        data_offset += entry_header->data_size;
    }

    return result;
}
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
    uint32_t checksum; // TODO: checksum!
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

size_t align(size_t original)
{ return (original % BYTE_ALIGNMENT) ? ((original / BYTE_ALIGNMENT) + 1) * BYTE_ALIGNMENT : original; }

DataBlock Package::encodePackage(const std::string& author, const Selector& selector,
    uint16_t creation_year, uint8_t creation_month, uint8_t creation_day)
{
    const std::lock_guard lock(getInstance()->database_mutex);

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
    package_header.signature         = SIGNATURE;
    package_header.version           = 1;
    package_header.padding           = 0xFF;
    package_header.entry_table_count = static_cast<uint16_t>(entries_to_pack.size());
    package_header.entry_table_offset =
        static_cast<uint32_t>(align(sizeof(PackageFileHeader)) + align(author.size()));
    package_header.creation_date_year   = creation_year;
    package_header.creation_date_months = creation_month;
    package_header.creation_date_days   = creation_day;
    package_header.file_size            = static_cast<uint32_t>(align(sizeof(PackageFileHeader)));
    package_header.author_str_size      = static_cast<uint16_t>(author.size());

    // TODO: compress each data entry! not the whole package

    size_t entry_table_length = 0;
    for (const auto& [entry, file] : entries_to_pack)
    {
        entry_table_length += align(sizeof(EntryFileHeader));
        entry_table_length += align(entry->identifier.size());
        entry_table_length += align(entry->author.size());
        package_header.file_size +=
            static_cast<uint32_t>(align(entry->is_loaded ? entry->data.size() : entry->data_size));
    }
    package_header.file_size += static_cast<uint32_t>(entry_table_length);
    package_header.file_size += static_cast<uint32_t>(align(package_header.author_str_size));

    DataBlock result;
    result.resize(package_header.file_size);
    memcpy(result.data(), &package_header, sizeof(PackageFileHeader));
    size_t offset = align(sizeof(PackageFileHeader));

    memcpy(result.data() + offset, author.data(), author.size());
    offset += align(package_header.author_str_size);

    size_t data_offset = offset + entry_table_length;
    for (auto& [entry, file] : entries_to_pack)
    {
        EntryFileHeader* entry_header      = reinterpret_cast<EntryFileHeader*>(result.data() + offset);
        entry_header->identifier_str_size  = static_cast<uint16_t>(entry->identifier.size());
        entry_header->author_str_size      = static_cast<uint16_t>(entry->author.size());
        entry_header->creation_date_year   = entry->creation_date_year;
        entry_header->creation_date_months = entry->creation_date_months;
        entry_header->creation_date_days   = entry->creation_date_days;
        entry_header->data_size =
            static_cast<uint32_t>(entry->is_loaded ? entry->data.size() : entry->data_size);
        entry_header->data_offset = static_cast<uint32_t>(data_offset);
        offset += align(sizeof(EntryFileHeader));

        memcpy(result.data() + offset, entry->identifier.data(), entry->identifier.size());
        offset += align(entry_header->identifier_str_size);

        memcpy(result.data() + offset, entry->author.data(), entry->author.size());
        offset += align(entry_header->author_str_size);

        if (entry->is_loaded) memcpy(result.data() + data_offset, entry->data.data(), entry->data.size());
        else
        {
            file->seekg(static_cast<std::streampos>(entry->data_offset));
            file->readsome(reinterpret_cast<char*>(result.data() + data_offset), entry->data_size);
        }
        data_offset += align(entry_header->data_size);
    }

    DBG_INFO("stored " + std::to_string(package_header.entry_table_count) + " entries (" +
             std::to_string(package_header.file_size) + " bytes) to in-memory package file.");

    return result;
}

bool populateIndex(const DataBlock& data, const PackageFileHeader*& package_header,
    std::vector<const EntryFileHeader*>& entry_headers)
{
    package_header = nullptr;
    entry_headers.clear();
    if (data.size() < sizeof(package_header))
    {
        DBG_ERROR("failed to decode package: data too short");
        return false;
    }
    package_header = reinterpret_cast<const PackageFileHeader*>(data.data());
    if (package_header->signature != SIGNATURE)
    {
        DBG_ERROR("failed to decode package: invalid signature");
        package_header = nullptr;
        return false;
    }
    if (package_header->file_size != static_cast<uint32_t>(data.size()))
    {
        DBG_ERROR("failed to decode package: invalid file size");
        package_header = nullptr;
        return false;
    }
    if (package_header->version != 1)
    {
        DBG_ERROR("failed to decode package: invalid version");
        package_header = nullptr;
        return false;
    }
    if (package_header->entry_table_offset +
            (package_header->entry_table_count * align(sizeof(EntryFileHeader))) >
        data.size())
    {
        DBG_ERROR("failed to decode package: data appears truncated");
        package_header = nullptr;
        return false;
    }
    size_t offset = package_header->entry_table_offset;
    for (size_t i = 0; i < package_header->entry_table_count; ++i)
    {
        if (offset + align(sizeof(EntryFileHeader)) > data.size())
        {
            DBG_ERROR("failed to decode package: data appears truncated");
            package_header = nullptr;
            entry_headers.clear();
            return false;
        }
        const EntryFileHeader* entry_header =
            reinterpret_cast<const EntryFileHeader*>(data.data() + offset);
        if (offset + align(sizeof(EntryFileHeader)) + align(entry_header->author_str_size) +
                align(entry_header->identifier_str_size) >
            data.size())
        {
            DBG_ERROR("failed to decode package: data appears truncated");
            package_header = nullptr;
            entry_headers.clear();
            return false;
        }
        if (entry_header->data_offset + align(entry_header->data_size) > data.size())
        {
            DBG_ERROR("failed to decode package: data appears truncated");
            package_header = nullptr;
            entry_headers.clear();
            return false;
        }
        offset += align(sizeof(EntryFileHeader)) + align(entry_header->author_str_size) + align(entry_header->identifier_str_size);
        entry_headers.push_back(entry_header);
    }
    return true;
}

bool Package::importPackage(const DataBlock& data)
{
    const PackageFileHeader* package_header;
    std::vector<const EntryFileHeader*> entry_headers;
    if (!populateIndex(data, package_header, entry_headers))
    {
        DBG_ERROR("unable to decode package");
        return false;
    }
    size_t offset = package_header->entry_table_offset;
    for (const EntryFileHeader* entry_header : entry_headers)
    {
        offset += align(sizeof(EntryFileHeader));
        std::string identifier(reinterpret_cast<const char*>(data.data() + offset),
            entry_header->identifier_str_size);
        offset += align(entry_header->identifier_str_size);
        std::string author(reinterpret_cast<const char*>(data.data() + offset),
            entry_header->author_str_size);
        offset += align(entry_header->author_str_size);
        Package::store("res://" + identifier,
            DataBlock(data.begin() + entry_header->data_offset,
                data.begin() + entry_header->data_offset + entry_header->data_size),
            author, entry_header->creation_date_year, entry_header->creation_date_months,
            entry_header->creation_date_days);
    }
    DBG_INFO("loaded " + std::to_string(package_header->entry_table_count) + " entries (" +
             std::to_string(package_header->file_size) + " bytes) from in-memory package file.");
    return true;
}

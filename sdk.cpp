/*
@file

    sdk.cpp

@purpose

    UE4 structs and functionality
*/

#include "sdk.h"
#include "offsets.h"

uint64_t b4b::TUObjectArray::get_object(const uint32_t id) const
{
    if (id >= num_elements) return 0ull;

    const auto chunk_index = id / 65536;

    if (chunk_index >= num_chunks) return 0ull;

    const auto chunk = interfaces::memorymanager->read<uint64_t>(objects + chunk_index);

    if (chunk == 0ull) return 0ull;

    const auto within_chunk_index = id % 65536 * 24; // 24 = sizeof(FUObjectItem) or chunksize

    return interfaces::memorymanager->read<uint64_t>(chunk + within_chunk_index);
}

std::string b4b::get_name(const uint64_t name_index)
{
    // get data location
    const auto data = interfaces::memorymanager->read<uint64_t>(offsets::gnames::pointer + (((name_index >> 16u) + 2u) * 8u)) + static_cast<uint16_t>(name_index) * 2ull;

    // size of the string
    const auto info = static_cast<uint32_t>(interfaces::memorymanager->read<uint16_t>(data));
    const auto string_length = info >> 6;

    // is the string wide?
    const bool wide = (info >> 0) & 1;

    if (!wide)
    {
        // prepare return string
        std::string ret;

        ret.resize(string_length, '\0');

        // ansi string
        if (!interfaces::memorymanager->read_raw(data + 2, &ret[0], string_length)) return "";

        return ret;
    }

    // wide string
    const auto buf = std::make_unique<wchar_t[]>(string_length + 1ull);

    if (!interfaces::memorymanager->read_raw(data + 2, buf.get(), string_length * sizeof(wchar_t))) return "";

    buf[string_length] = L'\0';

    // prepare return string
    std::string ret;

    ret.resize(string_length, '\0');

    // convert wide string to multibyte
    const auto copied = WideCharToMultiByte(CP_UTF8, 0, buf.get(), string_length, &ret[0], string_length, 0, 0);

    return copied == 0 ? "" : ret;
}

std::string b4b::UObject::get_name(void) const
{
    std::string name_string(name.get_name());

    if (name.comparison_index > 0)
    {
        name_string += '_' + std::to_string(name.comparison_index);
    }

    const auto pos = name_string.rfind('/');

    if (pos == std::string::npos) return name_string;

    return name_string.substr(pos + 1);
}

std::string b4b::FField::get_name(void) const
{
    std::string name_string(name.get_name());

    if (name.comparison_index > 0)
    {
        name_string += '_' + std::to_string(name.comparison_index);
    }

    const auto pos = name_string.rfind('/');

    if (pos == std::string::npos) return name_string;

    return name_string.substr(pos + 1);
}

std::string b4b::UObject::get_full_name(void) const
{
    std::string name;

    if (cls != 0ull)
    {
        std::string temp;

        for (auto p = outer; p != 0ull; p = interfaces::memorymanager->read<uint64_t>(p + offsetof(UObject, outer)))
        {
            temp = interfaces::memorymanager->read<UObject>(p).get_name() + '.' + temp;
        }

        name = interfaces::memorymanager->read<UObject>(cls).get_name();
        name += " ";
        name += temp;
        name += get_name();
    }

    return name;
}

bool b4b::UObject::is_a(const uint64_t cls, const uint64_t cmp)
{
    for (auto super = interfaces::memorymanager->read<uint64_t>(cls + offsetof(UObject, cls)); super != 0ull; super = interfaces::memorymanager->read<uint64_t>(super + offsetof(UStruct, super_struct)))
    {
        if (super == cmp) return true;
    }

    return false;
}

uint64_t b4b::UObject::find_class(const std::string& name)
{
    const auto object_array = interfaces::memorymanager->read<TUObjectArray>(offsets::gobjects::pointer);

    for (uint32_t i = 0; i < object_array.num_elements; i++)
    {
        try
        {
            const auto object_address = object_array.get_object(i);

            if (object_address == 0ull) continue;

            const auto uobject = interfaces::memorymanager->read<UStruct>(object_address);

            if (uobject.cls == 0ull) continue;

            const auto fullname = uobject.get_full_name();

            if (fullname == name) return object_address;
        }
        catch (...) {}
    }

    return 0ull;
}
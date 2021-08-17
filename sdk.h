/*
@file

    sdk.h

@purpose

    UE4 structs and functionality
*/

#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace b4b
{

    struct TUObjectArray final
    {

        uint64_t objects;
        uint64_t pre_allocated_objects;

        uint32_t max_elements;
        uint32_t num_elements;
        uint32_t max_chunks;
        uint32_t num_chunks;

        // returns the UObject* address of an object index
        uint64_t get_object(const uint32_t id) const;

    };

    // sdk.cpp
    extern std::string get_name(const uint64_t name_index);

    struct FNameEntry
    {

        int32_t index;
        int32_t comparison_index;

        std::string get_name(void) const
        {
            return b4b::get_name(static_cast<uint64_t>(index));
        }

    };

    struct UObject
    {

        uint64_t pad1;
        uint32_t pad2;
        int32_t index;
        uint64_t cls;

        FNameEntry name;

        uint64_t outer;
        uint64_t pad;

        std::string get_name(void) const;
        std::string get_full_name(void) const;

        static bool is_a(const uint64_t cls, const uint64_t cmp);
        static uint64_t find_class(const std::string& name);

    };

    struct UField : public UObject
    {

        uint64_t next;

    };

    struct UStruct : public UField
    {

        uint64_t pad0[2];
        uint64_t super_struct;
        uint64_t pad1;
        uint64_t children;
        uint64_t child_properties;
        int32_t properties_size;

    };

    struct FField
    {

        uint64_t pad[4];
        uint64_t next;

        FNameEntry name;

        std::string get_name(void) const;

    };

    struct FProperty : public FField
    {

        uint64_t pad1[3];
        uint32_t pad2;

        int32_t offset;

    };

    static_assert(offsetof(UObject, cls) == 0x10, "Class offset is misaligned");
    static_assert(offsetof(UObject, name.index) == 0x18, "Name index offset is misaligned");
    static_assert(offsetof(UObject, outer) == 0x20, "Outer offset is misaligned");
    static_assert(offsetof(UField, next) == 0x30, "Next offset is misaligned");
    static_assert(offsetof(UStruct, super_struct) == 0x48, "Super struct offset is misaligned");
    static_assert(offsetof(UStruct, children) == 0x58, "Children offset is misaligned");
    static_assert(offsetof(FField, next) == 0x20, "Next offset is misaligned");
    static_assert(offsetof(FField, name.index) == 0x28, "Name offset is misaligned");
    static_assert(offsetof(FProperty, offset) == 0x4C, "Name offset is misaligned");

}
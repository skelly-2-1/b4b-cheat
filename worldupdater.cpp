/*
@file

    worldupdater.cpp

@purpose

    Updating the address of the gameworld
*/

#include "../lib/strenc/strenc.h"
#include "../lib/fpsmanager.h"
#include "worldupdater.h"
#include "offsets.h"

namespace b4b
{

    namespace
    {

        template<class T>
        class TArray
        {

        private:

            T* Data;
            int32_t Count;
            int32_t Max;

        public:

            TArray()
            {
                Data = nullptr;
                Count = Max = 0;
            };

            size_t Num() const
            {
                return Count;
            };

            T& operator[](size_t i)
            {
                return Data[i];
            };

            const T& operator[](size_t i) const
            {
                return Data[i];
            };

            bool IsValidIndex(size_t i) const
            {
                return i < Num();
            }

            void* DataPointer(void) const
            {
                return reinterpret_cast<void*>(Data);
            }

        };

    }

}

DWORD WINAPI b4b::worldupdater_t::thread(LPVOID param)
{
    auto userdata = reinterpret_cast<worldupdater_thread_userdata_t*>(param);

    userdata->instance.run(userdata);
    userdata->closed = true;

    return 0ul;
}

void b4b::worldupdater_t::run(worldupdater_thread_userdata_t* userdata)
{
    world_t temp;

    auto update_temp_world = [&temp](void) -> bool
    {
        try
        {
            // world and game instance
            const auto world = interfaces::memorymanager->read<uint64_t>(offsets::uworld::pointer);

            if (world == 0ull) return false;

            // grab the level and it's entities
            const auto levels = interfaces::memorymanager->read<TArray<uint64_t>>(world + offsets::uworld::levels);

            if (levels.Num() == 0) return false;

            // grab the persistent level
            const auto persistent_level = interfaces::memorymanager->read<uint64_t>(world + offsets::uworld::persistent_level);

            if (persistent_level == 0) return false;

            temp.levels.resize(levels.Num());

            if (!interfaces::memorymanager->read_raw(reinterpret_cast<uint64_t>(levels.DataPointer()), temp.levels.data(), levels.Num() * sizeof(uint64_t))) return false;

            // filter out the persistent level from the levels array
            const auto it = std::find(temp.levels.begin(), temp.levels.end(), persistent_level);

            if (it == temp.levels.end()) return false;

            temp.levels.erase(it);
            temp.world = world;
            temp.persistent_level = persistent_level;
            temp.valid = true;

            return true;
        }
        catch (...) {}

        return false;
    };

    bool last_updated = false;

    for (fpsmanager_t fpsmanager(1u, 1.f); !userdata->close; fpsmanager.run())
    {
        userdata->framerate = fpsmanager.get_framerate();
        userdata->last_frametime = static_cast<float>(fpsmanager.get_last_frametime());

        const auto result = update_temp_world();

        if (result)
        {
            set_world(temp);

            last_updated = true;
        }
        else if (!result && last_updated)
        {
            temp.invalidate();

            set_world(temp);

            last_updated = false;
        }
    }
}
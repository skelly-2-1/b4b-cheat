/*
@file

	zombieupdater.cpp

@purpose

	Thread for updating all zombies
*/

#include "zombieupdater.h"
#include "offsets.h"
#include "../lib/fpsmanager.h"
#include "entityaddressupdater.h"
#include "sdk.h"

DWORD WINAPI b4b::zombieupdater_t::thread(LPVOID param)
{
    auto userdata = reinterpret_cast<zombieupdater_thread_userdata_t*>(param);

    userdata->instance.run(userdata);
    userdata->closed = true;

    return 0ul;
}

void b4b::zombieupdater_t::run(zombieupdater_thread_userdata_t* userdata)
{
    std::deque<zombie_entity_t> temp_zombies;

    auto reset = [&temp_zombies](void) -> void
    {
        set_zombies(temp_zombies);
    };

    auto run = [&temp_zombies](void) -> bool
    {
        for (const auto& zombie_address : entityaddressupdater->get_addresses(entity_type_e::ZOMBIE))
        {
            zombie_entity_t zombie(zombie_address.address);

            zombie.update();

            if (zombie.valid) temp_zombies.push_back(zombie);
        }

        return true;
    };

    for (fpsmanager_t fpsmanager(userdata->target_fps, 1.f); !userdata->close; fpsmanager.run())
    {
        userdata->framerate = fpsmanager.get_framerate();
        userdata->last_frametime = static_cast<float>(fpsmanager.get_last_frametime());

        temp_zombies.clear();

        try
        {
            if (!run())
            {
                reset();

                continue;
            }
        }
        catch (...)
        {
            reset();

            continue;
        }

        set_zombies(temp_zombies);
    }
}
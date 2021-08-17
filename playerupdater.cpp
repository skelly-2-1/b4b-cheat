/*
@file

    playerupdater.cpp

@purpose

    Thread for updating all players
*/

#include "playerupdater.h"
#include "offsets.h"
#include "../lib/fpsmanager.h"
#include "entityaddressupdater.h"
#include "sdk.h"

DWORD WINAPI b4b::playerupdater_t::thread(LPVOID param)
{
    auto userdata = reinterpret_cast<playerupdater_thread_userdata_t*>(param);

    userdata->instance.run(userdata);
    userdata->closed = true;

    return 0ul;
}

void b4b::playerupdater_t::run(playerupdater_thread_userdata_t* userdata)
{
    std::deque<player_entity_t> temp_players;

    player_entity_t temp_localplayer;

    auto reset = [&temp_players](void) -> void
    {
        player_entity_t trash;

        set_localplayer(trash);
        set_players(temp_players);
    };

    auto run = [&temp_players, &temp_localplayer/*, &bot*/](void) -> bool
    {
        const auto localplayer_address = entityaddressupdater->get_localplayer_address();

        if (localplayer_address == 0ull) return false;

        const auto localplayer_controller = interfaces::memorymanager->read<uint64_t>(localplayer_address + offsets::player::controller);

        if (localplayer_controller == 0ull) return false;

        const auto localplayer_pawn = interfaces::memorymanager->read<uint64_t>(localplayer_controller + offsets::playercontroller::acknowledged_pawn);

        if (localplayer_pawn == 0ull) return false;

        temp_localplayer.address = localplayer_pawn;
        temp_localplayer.update();

        if (!temp_localplayer.valid) return false;

        for (const auto& player_address : entityaddressupdater->get_addresses(entity_type_e::PLAYER))
        {
            player_entity_t player(player_address.address);

            player.update();

            if (player.valid) temp_players.push_back(player);
        }

        return true;
    };

    for (fpsmanager_t fpsmanager(userdata->target_fps, 1.f); !userdata->close; fpsmanager.run())
    {
        userdata->framerate = fpsmanager.get_framerate();
        userdata->last_frametime = static_cast<float>(fpsmanager.get_last_frametime());

        temp_players.clear();

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

        set_localplayer(temp_localplayer);
        set_players(temp_players);
    }
}
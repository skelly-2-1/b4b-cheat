/*
@file

    entityaddressupdater.cpp

@purpose

    Thread for updating all entity addresses
*/

#include "entityaddressupdater.h"
#include "worldupdater.h"
#include "offsets.h"
#include "../lib/fpsmanager.h"
#include "sdk.h"

DWORD WINAPI b4b::entityaddressupdater_t::thread(LPVOID param)
{
    auto userdata = reinterpret_cast<entityaddressupdater_thread_userdata_t*>(param);

    userdata->instance.run(userdata);
    userdata->closed = true;

    return 0ul;
}

void b4b::entityaddressupdater_t::run(entityaddressupdater_thread_userdata_t* userdata)
{
    // required classes
    uint64_t actor_class = 0ull, character = 0ull, gobi_character = 0ull, player_character = 0ull;
    {
        std::array<std::pair<uint64_t*, std::string>, 4ull> classes_to_grab{

            std::make_pair(&actor_class, sstrenc("Class Engine.Actor")),
            std::make_pair(&character, sstrenc("Class Engine.Character")),
            std::make_pair(&gobi_character, sstrenc("Class Gobi.GobiCharacter")),
            std::make_pair(&player_character, sstrenc("Class Gobi.PlayerCharacter"))

        };

        uint8_t grabbed_classes = 0;

        for (; grabbed_classes < classes_to_grab.size() && !userdata->close; std::this_thread::sleep_for(std::chrono::milliseconds(200)))
        {
            for (auto& entry : classes_to_grab)
            {
                if (*entry.first != 0ull) continue;

                auto found = UObject::find_class(entry.second);

                if (found == 0ull) continue;

                *entry.first = found;

                if (++grabbed_classes == classes_to_grab.size()) break;
            }
        }

        if (userdata->close)
        {
            userdata->closed = true;

            return;
        }
    }

    decltype(addresses) temp_addresses;

    uint64_t temp_localplayer_address = 0ull;

    std::vector<uint64_t> actor_addresses;

    auto find_addresses = [&temp_addresses, &temp_localplayer_address,&actor_addresses, &actor_class, &character, &gobi_character, &player_character](void) -> void
    {
        if (worldupdater == nullptr) return;

        const auto world = worldupdater->get_world();

        if (!world.valid) return;

        const auto game_instance = interfaces::memorymanager->read<uint64_t>(world.world + offsets::uworld::owning_game_instance);

        if (game_instance == 0ull) return;

        const auto localplayers = interfaces::memorymanager->read<uint64_t>(game_instance + offsets::gameinstance::local_players);

        if (localplayers == 0ull) return;

        temp_localplayer_address = interfaces::memorymanager->read<uint64_t>(localplayers);

        if (temp_localplayer_address == 0ull) return;

        const auto localplayer_controller = interfaces::memorymanager->read<uint64_t>(temp_localplayer_address + offsets::player::controller);

        if (localplayer_controller == 0ull) return;

        const auto localplayer_pawn = interfaces::memorymanager->read<uint64_t>(localplayer_controller + offsets::playercontroller::acknowledged_pawn);

        if (localplayer_pawn == 0ull) return;

        auto check_level = [&actor_addresses, localplayer_pawn, &actor_class, &character, &temp_addresses, &gobi_character, &player_character](const uint64_t& level_address)
        {
            struct actor_information_t final
            {

                uint64_t array;
                uint32_t count;

            };

            actor_information_t actor_information;

            if (!interfaces::memorymanager->read_raw(level_address + 0xA0, &actor_information, sizeof(actor_information))) return;
            if (actor_information.count == 0 || actor_information.array == 0) return;

            // read all the actors
            actor_addresses.resize(actor_information.count);

            if (!interfaces::memorymanager->read_raw(actor_information.array, actor_addresses.data(), actor_information.count * sizeof(uint64_t))) return;

            // loop all actors
            for (const auto& actor : actor_addresses)
            {
                if (actor == 0ull) continue;
                if (actor == localplayer_pawn) continue;

                const auto name_index = interfaces::memorymanager->read<int32_t>(actor + offsetof(UObject, name.index));

                if (name_index == 0) continue;

                // players
                if (UObject::is_a(actor, player_character))
                {
                    temp_addresses.at(static_cast<size_t>(entity_type_e::PLAYER)).emplace_back(entityaddress_t{ actor });
                }
                else if (UObject::is_a(actor, gobi_character)/* && interfaces::memorymanager->read<uint64_t>(actor + offsets::pawn::player_state == 0ull)*/) // not a player, has to be a zombie
                {
                    // NOTE: special infected seem to have a player state!
                    // not a player, has to be a zombie
                    // EDIT: not all of them???
                    temp_addresses.at(static_cast<size_t>(entity_type_e::ZOMBIE)).emplace_back(entityaddress_t{ actor });
                }
            }
        };

        check_level(world.persistent_level);

        for (const auto& level : world.levels) check_level(level);
    };

    for (fpsmanager_t fpsmanager(2u, 1.f); !userdata->close; fpsmanager.run())
    {
        userdata->framerate = fpsmanager.get_framerate();
        userdata->last_frametime = static_cast<float>(fpsmanager.get_last_frametime());

        // clear all temporary addresses
        for (auto& vec : temp_addresses) vec.clear();

        // clear the localplayer/hud addresses
        temp_localplayer_address = 0ull;

        // find all addresses and put them into a temporary vector
        try
        {
            find_addresses();
        }
        catch (...) {}

        // apply the temporary addresses
        for (uint32_t i = 0; i < static_cast<uint32_t>(entity_type_e::SIZE); i++)
        {
            auto& mutex = mutexes.at(i);

            std::lock_guard<std::shared_mutex> guard(mutex);

            addresses.at(i) = temp_addresses.at(i);
        }

        // apply the temporary localplayer/hud addresses
        localplayer_address = temp_localplayer_address;
    }
}
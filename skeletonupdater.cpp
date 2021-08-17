/*
@file

    skeletonupdater.cpp

@purpose

    Thread for updating skeletons (bones)
*/

#include <unordered_map>
#include "../lib/fpsmanager.h"
#include "../lib/timer.h"
#include "skeletonupdater.h"
#include "offsets.h"
#include "entityaddressupdater.h"
#include "config.h"
#include "entities.h"
#include "skeletonindicesupdater.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

DWORD WINAPI b4b::skeletonupdater_t::thread(LPVOID param)
{
    auto userdata = reinterpret_cast<skeletonupdater_thread_userdata_t*>(param);

    userdata->instance.run(userdata);
    userdata->closed = true;

    return 0ul;
}

void b4b::skeletonupdater_t::run(skeletonupdater_thread_userdata_t* userdata)
{
    constexpr uint32_t max_skeleton_expire_time = 50;

    std::unordered_map<uint64_t, std::pair<skeleton_entity_t, timer_t>> skeleton_cache;

    std::deque<player_entity_t> temp_players;
    std::deque<zombie_entity_t> temp_zombies;
    std::deque<skeleton_entity_t> temp_skeletons;
    player_entity_t localplayer;

    auto target_fps = userdata->target_fps;

    if (config.misc_bones_target_fps >= 0) target_fps = std::max(std::min(static_cast<uint32_t>(config.misc_bones_target_fps), userdata->target_fps), 30u);

    const auto loop_entities = [&userdata, &temp_skeletons, &skeleton_cache](const auto& entity_vector) -> void
    {
        for (const auto& entity : entity_vector)
        {
            try
            {
                if (!entity.valid) continue;

                skeleton_entity_t skeleton(entity.address, userdata->boneid_transform_entry_type);

                skeleton.update();

                if (!skeleton.valid) continue;

                temp_skeletons.push_back(skeleton);

                timer_t timer;
                timer.start();

                skeleton_cache[entity.address] = std::make_pair(std::move(skeleton), timer);
            }
            catch (...) {}
        }
    };

    for (fpsmanager_t fpsmanager(target_fps, 1.f); !userdata->close; fpsmanager.run())
    {
        userdata->framerate = fpsmanager.get_framerate();
        userdata->last_frametime = static_cast<float>(fpsmanager.get_last_frametime());

        // remove old entries from skeleton cache
        for (auto it = skeleton_cache.begin(); it != skeleton_cache.end();)
        {
            if (it->second.second.get_elapsed() > max_skeleton_expire_time)
            {
                it = skeleton_cache.erase(it);

                continue;
            }

            it++;
        }

        if (userdata->boneid_transform_entry_type == boneid_transform_entry_e::players)
        {
            temp_players = get_players();

            if (temp_players.empty())
            {
                temp_skeletons.clear();

                continue;
            }
        }
        else
        {
            temp_zombies = get_zombies();

            if (temp_zombies.empty())
            {
                temp_skeletons.clear();

                continue;
            }
        }

        boneid_transforms.at(static_cast<size_t>(userdata->boneid_transform_entry_type)) = skeletonindicesupdater->get_bone_index_cache(userdata->boneid_transform_entry_type);

        if (!boneid_transforms.at(static_cast<size_t>(userdata->boneid_transform_entry_type)).empty())
        {
            if (userdata->boneid_transform_entry_type == boneid_transform_entry_e::players)
            {
                loop_entities(temp_players);
            }
            else
            {
                loop_entities(temp_zombies);
            }

            if (!updated_boneid_transforms.at(static_cast<size_t>(userdata->boneid_transform_entry_type)).empty()) skeletonindicesupdater->update_timers(userdata->boneid_transform_entry_type, updated_boneid_transforms.at(static_cast<size_t>(userdata->boneid_transform_entry_type)));
        }

        // apply cached skeletons if needed
        for (const auto& cache : skeleton_cache)
        {
            const auto f = std::find_if(temp_skeletons.begin(), temp_skeletons.end(), [&cache](const skeleton_entity_t& skel) { return skel.address == cache.first; });

            if (f != temp_skeletons.end()) continue;

            temp_skeletons.push_back(cache.second.first);
        }

        if (userdata->boneid_transform_entry_type == boneid_transform_entry_e::players)
        {
            set_player_skeletons(temp_skeletons);
        }
        else
        {
            set_zombie_skeletons(temp_skeletons);
        }

        temp_skeletons.clear();
    }

    skeleton_cache.clear();
}
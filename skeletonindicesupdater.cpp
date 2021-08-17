/*
@file

    skeletonindicesupdater.cpp

@purpose

    Updating bone indices for relevant bone updaters
*/

#include "skeletonindicesupdater.h"
#include "../lib/fpsmanager.h"
#include "entities.h"
#include "offsets.h"
#include "sdk.h"

DWORD WINAPI b4b::skeletonindicesupdater_t::thread(LPVOID param)
{
    auto userdata = reinterpret_cast<skeletonindicesupdater_thread_userdata_t*>(param);

    userdata->instance.run(userdata);
    userdata->closed = true;

    return 0ul;
}

void b4b::skeletonindicesupdater_t::run(skeletonindicesupdater_thread_userdata_t* userdata)
{
    boneid_transform_type temp_bonetransform;
    std::array<std::unordered_map<uint64_t, std::pair<timer_t, boneid_transform_type>>, static_cast<size_t>(boneid_transform_entry_e::size)> temp_bone_index_cache;
    std::unordered_map<std::string, uint32_t> temp_bonename_map;
    std::deque<player_entity_t> players;
    std::deque<zombie_entity_t> zombies;
    std::array<bool, static_cast<size_t>(boneid_transform_entry_e::size)> updated;

    const auto get_bone_indices = [&temp_bonetransform, &temp_bonename_map](const uint64_t sockets, boneid_transform_type& transform) -> bool
    {
        for (uint32_t i = 0u; i < 300u; i++)
        {
            try
            {
                const auto name_index = interfaces::memorymanager->read<uint32_t>(sockets + 12ull * i);

                if (name_index == 0ull) break;

                auto name = get_name(name_index);

                if (name.empty()) break;

                // only allow sane characters
                for (const auto& c : name)
                {
                    if (isalpha(static_cast<int>(c)) || (c >= '0' && c <= '9') || c == '_') continue;

                    name.clear();

                    break;
                }

                if (name.empty()) break;

                temp_bonename_map[name] = i;
            }
            catch (...) {}
        }

        if (temp_bonename_map.empty())
        {
            temp_bonename_map.clear();

            return false;
        }

#define GET_BONE(name, enm)\
	{\
		const auto f = temp_bonename_map.find(sstrenc(name));\
		if (f == temp_bonename_map.end()) { temp_bonename_map.clear(); return false; }\
		transform.at(static_cast<size_t>(enm)) = f->second;\
	}

        // get bone indices by bone name
        GET_BONE("Head", boneids_e::head);
        GET_BONE("Root", boneids_e::root);
        GET_BONE("ik_hand_l", boneids_e::left_hand);
        GET_BONE("ik_hand_r", boneids_e::right_hand);
        GET_BONE("neck_02", boneids_e::neck);
        GET_BONE("pelvis", boneids_e::pelvis);
        GET_BONE("ik_foot_r", boneids_e::right_foot);
        GET_BONE("ik_foot_l", boneids_e::left_foot);
        GET_BONE("elbow_twist_01_l", boneids_e::left_elbow);
        GET_BONE("elbow_twist_01_r", boneids_e::right_elbow);
        GET_BONE("shoulder_twist_01_r", boneids_e::right_shoulder);
        GET_BONE("shoulder_twist_01_l", boneids_e::left_shoulder);
        GET_BONE("knee_twist_01_r", boneids_e::right_knee);
        GET_BONE("knee_twist_01_l", boneids_e::left_knee);

        temp_bonename_map.clear();

        return true;
    };

    const auto remove_outdated_entries = [&](const boneid_transform_entry_e type)
    {
        auto& mutex = mutexes.at(static_cast<size_t>(type));

        std::lock_guard<std::shared_mutex> guard(mutex);

        auto& current_bone_index_cache = bone_index_cache.at(static_cast<size_t>(type));

        for (auto it = current_bone_index_cache.begin(); it != current_bone_index_cache.end();)
        {
            if (it->second.first.get_elapsed() >= 1000)
            {
                it = current_bone_index_cache.erase(it);

                continue;
            }

            it++;
        }

        temp_bone_index_cache.at(static_cast<size_t>(type)) = bone_index_cache.at(static_cast<size_t>(type));
    };

    for (fpsmanager_t fpsmanager(1u, 1.f); !userdata->close; fpsmanager.run())
    {
        userdata->framerate = fpsmanager.get_framerate();
        userdata->last_frametime = static_cast<float>(fpsmanager.get_last_frametime());

        // remove entries that haven't been updated in over a second
        for (uint32_t i = 0; i < static_cast<uint64_t>(boneid_transform_entry_e::size); i++) remove_outdated_entries(static_cast<boneid_transform_entry_e>(i));
        
        // reset updated flags
        for (uint32_t i = 0; i < static_cast<uint64_t>(boneid_transform_entry_e::size); i++) updated.at(i) = false;

        // get players and zombies
        players = get_players();
        zombies = get_zombies();

        // if there's no players and zombies, nothing to do
        if (players.empty() && zombies.empty()) continue;

        auto update = [&get_bone_indices, &temp_bone_index_cache, &temp_bonetransform, &updated](const auto& entity_vector, const boneid_transform_entry_e type)
        {
            auto& current_temp_bone_index_cache = temp_bone_index_cache.at(static_cast<size_t>(type));

            for (const auto& entity : entity_vector)
            {
                try
                {
                    const auto sockets = entity.get_sockets_address();

                    if (sockets == 0ull) continue;
                    if (!get_bone_indices(sockets, temp_bonetransform)) continue;

                    auto& entry = current_temp_bone_index_cache[sockets];

                    // got bone indices, start the timer and set the bone index transform
                    entry.first.start();
                    entry.second = temp_bonetransform;

                    // tell the caller that we've updated something
                    updated.at(static_cast<size_t>(type)) = true;
                }
                catch (...) {}
            }
        };

        update(players, boneid_transform_entry_e::players);
        update(zombies, boneid_transform_entry_e::zombies);

        // if nothing updated, nothing to do
        if (std::find(updated.begin(), updated.end(), true) == updated.end()) continue;

        for (uint32_t i = 0; i < static_cast<uint64_t>(boneid_transform_entry_e::size); i++)
        {
            if (!updated.at(static_cast<size_t>(i))) continue;

            auto& mutex = mutexes.at(static_cast<size_t>(i));

            std::lock_guard<std::shared_mutex> guard(mutex);

            bone_index_cache.at(static_cast<size_t>(i)) = temp_bone_index_cache.at(static_cast<size_t>(i));
        }
    }
}
/*
@file

	skeletonindicesupdater.h

@purpose

	Updating bone indices for relevant bone updaters
*/

#pragma once

#include <Windows.h>
#include <unordered_map>
#include <shared_mutex>
#include <vector>
#include <array>
#include "../lib/timer.h"
#include "thread_userdata.h"
#include "entities.h"

namespace b4b
{

	class skeletonindicesupdater_t;

	struct skeletonindicesupdater_thread_userdata_t final : public thread_userdata_t
	{

		skeletonindicesupdater_t& instance;

		skeletonindicesupdater_thread_userdata_t(const bool& close, skeletonindicesupdater_t& instance) : thread_userdata_t(close), instance(instance) {}

	};

	using boneid_transform_type = std::array<uint32_t, static_cast<size_t>(boneids_e::size)>;

	class skeletonindicesupdater_t final
	{

	protected:



	private:

		std::array<std::shared_mutex, static_cast<size_t>(boneid_transform_entry_e::size)> mutexes;
		std::array<std::unordered_map<uint64_t, std::pair<timer_t, boneid_transform_type>>, static_cast<size_t>(boneid_transform_entry_e::size)> bone_index_cache;

	public:

		static DWORD WINAPI thread(LPVOID param);

		void run(skeletonindicesupdater_thread_userdata_t* userdata);

		std::unordered_map<uint64_t, std::pair<timer_t, boneid_transform_type>> get_bone_index_cache(const boneid_transform_entry_e type)
		{
			auto& mutex = mutexes.at(static_cast<size_t>(type));

			std::shared_lock<std::shared_mutex> guard(mutex);

			return bone_index_cache.at(static_cast<size_t>(type));
		}

		// called when skeletons have been updated
		bool update_timers(const boneid_transform_entry_e type, const std::vector<uint64_t> addresses)
		{
			if (addresses.empty()) return false;

			bool result = true;

			auto& mutex = mutexes.at(static_cast<size_t>(type));

			std::unique_lock<std::shared_mutex> lock_guard(mutex);

			auto& current_bone_index_cache = bone_index_cache.at(static_cast<size_t>(type));

			if (current_bone_index_cache.empty()) return false;

			for (const auto address : addresses)
			{
				const auto f = current_bone_index_cache.find(address);

				if (f == current_bone_index_cache.end())
				{
					result = false;

					continue;
				}

				f->second.first.start(); // (re)start the timer
			}

			return result;
		}

	};

	// threads.cpp
	extern std::unique_ptr<skeletonindicesupdater_t> skeletonindicesupdater;

	// entities.cpp
	extern std::array<std::unordered_map<uint64_t, std::pair<timer_t, boneid_transform_type>>, static_cast<size_t>(boneid_transform_entry_e::size)> boneid_transforms;
	extern std::array<std::vector<uint64_t>, static_cast<size_t>(boneid_transform_entry_e::size)> updated_boneid_transforms;

}
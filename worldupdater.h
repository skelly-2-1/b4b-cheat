/*
@file

	worldupdater.h

@purpose

	Updating the world addresses
*/

#pragma once

#include <Windows.h>
#include <shared_mutex>
#include <vector>
#include <atomic>
#include <memory>
#include "thread_userdata.h"

namespace b4b
{

	class worldupdater_t;

	struct worldupdater_thread_userdata_t final : public thread_userdata_t
	{

		worldupdater_t& instance;

		worldupdater_thread_userdata_t(const bool& close, worldupdater_t& instance) : thread_userdata_t(close), instance(instance) {}

	};

	class world_t final
	{

	protected:



	private:



	public:

		bool valid;

		uint64_t world;
		uint64_t persistent_level;

		std::vector<uint64_t> levels;

		void invalidate(void)
		{
			valid = false;
			world = persistent_level = 0u;

			levels.clear();
		}

		world_t()
		{
			invalidate();
		}

	};

	class worldupdater_t final
	{

	protected:



	private:

		std::shared_mutex mutex;

		world_t world;

	public:

		world_t get_world(void)
		{
			std::shared_lock<std::shared_mutex> guard(mutex);

			return world;
		}

		void set_world(const world_t& world)
		{
			std::lock_guard<std::shared_mutex> guard(mutex);

			this->world = world;
		}

		static DWORD WINAPI thread(LPVOID param);

		void run(worldupdater_thread_userdata_t* userdata);

	};

	// threads.cpp
	extern std::unique_ptr<worldupdater_t> worldupdater;

}
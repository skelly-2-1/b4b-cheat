/*
@file

	zombieupdater.h

@purpose

	Thread for updating all zombies
*/

#pragma once

#include <Windows.h>
#include <array>
#include <vector>
#include <atomic>
#include <memory>
#include "thread_userdata.h"
#include "entities.h"

namespace b4b
{

	class zombieupdater_t;

	struct zombieupdater_thread_userdata_t final : public thread_userdata_t
	{

		zombieupdater_t& instance;

		uint32_t target_fps;

		zombieupdater_thread_userdata_t(const bool& close, zombieupdater_t& instance, const uint32_t target_fps) : thread_userdata_t(close), instance(instance), target_fps(target_fps) {}

	};

	class zombieupdater_t final
	{

	protected:



	private:



	public:

		static DWORD WINAPI thread(LPVOID param);

		void run(zombieupdater_thread_userdata_t* userdata);

	};

	// threads.cpp
	extern std::unique_ptr<zombieupdater_t> zombieupdater;

}
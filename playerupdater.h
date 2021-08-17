/*
@file

	playerupdater.h

@purpose

	Thread for updating all players
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

	class playerupdater_t;

	struct playerupdater_thread_userdata_t final : public thread_userdata_t
	{

		playerupdater_t& instance;

		uint32_t target_fps;

		playerupdater_thread_userdata_t(const bool& close, playerupdater_t& instance, const uint32_t target_fps) : thread_userdata_t(close), instance(instance), target_fps(target_fps) {}

	};

	class playerupdater_t final
	{

	protected:



	private:



	public:

		static DWORD WINAPI thread(LPVOID param);

		void run(playerupdater_thread_userdata_t* userdata);

	};

	// threads.cpp
	extern std::unique_ptr<playerupdater_t> playerupdater;

}
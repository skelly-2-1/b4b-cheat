/*
@file

	skeletonupdater.h

@purpose

	Thread for updating skeletons (bones)
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

	class skeletonupdater_t;

	struct skeletonupdater_thread_userdata_t final : public thread_userdata_t
	{

		skeletonupdater_t& instance;

		uint32_t target_fps;

		boneid_transform_entry_e boneid_transform_entry_type;

		skeletonupdater_thread_userdata_t(const bool& close, skeletonupdater_t& instance, const uint32_t target_fps, const boneid_transform_entry_e type) : thread_userdata_t(close), instance(instance), target_fps(target_fps), boneid_transform_entry_type(type) {}

	};

	class skeletonupdater_t final
	{

	protected:



	private:



	public:

		static DWORD WINAPI thread(LPVOID param);

		void run(skeletonupdater_thread_userdata_t* userdata);

	};

	// threads.cpp
	extern std::unique_ptr<skeletonupdater_t> skeletonupdater;

}

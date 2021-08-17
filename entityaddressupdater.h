/*
@file

	entityaddressupdater.h

@purpose

	Thread for updating all entity addresses
*/

#pragma once

#include <Windows.h>
#include <array>
#include <shared_mutex>
#include <vector>
#include <atomic>
#include <memory>
#include <unordered_map>
#include "thread_userdata.h"
#include "entities.h"

namespace b4b
{

	class entityaddressupdater_t;

	struct entityaddressupdater_thread_userdata_t final : public thread_userdata_t
	{

		entityaddressupdater_t& instance;

		entityaddressupdater_thread_userdata_t(const bool& close, entityaddressupdater_t& instance) : thread_userdata_t(close), instance(instance) {}

	};

	struct entityaddress_t final
	{

		uint64_t address;

		static_entity_type_e static_entity_type;

		std::string data;

	};

	class entityaddressupdater_t final
	{

	protected:



	private:

		std::array<std::shared_mutex, static_cast<size_t>(entity_type_e::SIZE)> mutexes;
		std::array<std::deque<entityaddress_t>, static_cast<size_t>(entity_type_e::SIZE)> addresses;
		std::atomic<uint64_t> localplayer_address;

	public:

		static DWORD WINAPI thread(LPVOID param);

		void run(entityaddressupdater_thread_userdata_t* userdata);

		std::deque<entityaddress_t> get_addresses(const entity_type_e type)
		{
			auto& mutex = mutexes.at(static_cast<size_t>(type));

			std::shared_lock<std::shared_mutex> guard(mutex);

			return addresses.at(static_cast<size_t>(type));
		}

		uint64_t get_localplayer_address(void) const
		{
			return localplayer_address;
		}

	};

	// threads.cpp
	extern std::unique_ptr<entityaddressupdater_t> entityaddressupdater;

}
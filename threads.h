/*
@file

	threads.h

@purpose

	Managing threads
*/

#pragma once

#include <deque>
#include <memory>
#include <string>
#include "thread_userdata.h"

using HANDLE = void*;

namespace b4b
{

	bool create_threads(const uint32_t target_fps, const bool& force_close, std::string* error = nullptr);

	void end_threads(void);

	struct thread_t final
	{

		std::unique_ptr<thread_userdata_t>* userdata;

		HANDLE* handle;

	};

	using thread_list = std::deque<std::pair<thread_userdata_t*, HANDLE*>>;

	const thread_list& get_threads(void);

}
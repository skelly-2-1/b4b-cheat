/*
@file

	thread_userdata.h

@purpose

	Base struct for thread userdata
*/

#pragma once

#include <atomic>

namespace b4b
{

	struct thread_userdata_t
	{

		std::atomic<float> framerate, last_frametime;

		const bool& close;

		bool closed;

		thread_userdata_t(const bool& close) : close(close), closed(false) {}

	};

}
/*
@file

	viewmatrixupdater.h

@purpose

	Thread for updating the viewmatrix
*/

#pragma once

#include <Windows.h>
#include <array>
#include <shared_mutex>
#include <vector>
#include <atomic>
#include <memory>
#include "thread_userdata.h"
#include "entities.h"
#include "d3dmatrix.h"

namespace b4b
{

	struct viewmatrix_t
	{

		bool valid;

		vector3d location;

		D3DMATRIX matrix;

		float fov;



	};

	class viewmatrixupdater_t;

	struct viewmatrixupdater_thread_userdata_t final : public thread_userdata_t
	{

		viewmatrixupdater_t& instance;

		uint32_t target_fps;

		viewmatrixupdater_thread_userdata_t(const bool& close, viewmatrixupdater_t& instance, const uint32_t target_fps) : thread_userdata_t(close), instance(instance), target_fps(target_fps) {}

	};

	class viewmatrixupdater_t final
	{

	protected:



	private:

		viewmatrix_t viewmatrix;

		std::shared_mutex mutex;

	public:

		viewmatrix_t get_viewmatrix(void)
		{
			std::shared_lock<std::shared_mutex> guard(mutex);

			return viewmatrix;
		}

		void set_viewmatrix(const viewmatrix_t& viewmatrix)
		{
			std::lock_guard<std::shared_mutex> guard(mutex);

			this->viewmatrix = viewmatrix;
		}

		static DWORD WINAPI thread(LPVOID param);

		void run(viewmatrixupdater_thread_userdata_t* userdata);

	};

	// threads.cpp
	extern std::unique_ptr<viewmatrixupdater_t> viewmatrixupdater;

}
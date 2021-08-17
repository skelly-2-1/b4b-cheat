/*
@file

	threads.cpp

@purpose

	Managing threads
*/

#include "../lib/strenc/strenc.h"
#include "threads.h"
#include "worldupdater.h"
#include "zombieupdater.h"
#include "playerupdater.h"
#include "viewmatrixupdater.h"
#include "entityaddressupdater.h"
#include "skeletonupdater.h"
#include "skeletonindicesupdater.h"

#define DECLARE_THREAD(updater)\
	std::unique_ptr<##updater##_t> ##updater##;\
	namespace{\
		std::unique_ptr<##updater##_thread_userdata_t> updater##_userdata;\
		HANDLE updater##_thread_handle = nullptr;\
	}

#define DECLARE_THREAD_2(updater)\
	std::unique_ptr<##updater##_t> ##updater##_2;\
	namespace{\
		std::unique_ptr<##updater##_thread_userdata_t> updater##_userdata_2;\
		HANDLE updater##_thread_handle_2 = nullptr;\
	}

namespace b4b
{

	DECLARE_THREAD(worldupdater);
	DECLARE_THREAD(playerupdater);
	DECLARE_THREAD(zombieupdater);
	DECLARE_THREAD(viewmatrixupdater);
	DECLARE_THREAD(entityaddressupdater);
	DECLARE_THREAD(skeletonupdater);
	DECLARE_THREAD_2(skeletonupdater);
	DECLARE_THREAD(skeletonindicesupdater);

	namespace
	{

		thread_list threads;

	}

}

void b4b::end_threads(void)
{
    // wait for any additional threads to close
    for (auto& thread : threads)
    {
        if (*thread.second == nullptr || thread.first == nullptr) continue;
        if (thread.first->closed) continue;

        for (auto timeout_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds(2); std::chrono::high_resolution_clock::now() < timeout_time && !thread.first->closed; std::this_thread::sleep_for(std::chrono::milliseconds(50))) {}

        if (thread.first->closed) continue;

        // thread didn't close after a second, terminate it
        TerminateThread(*thread.second, 0ul);
    }

	// helper macros
#define CLEAN_THREAD(updater)\
	updater##.reset();\
	##updater##_userdata.reset()

#define CLEAN_THREAD_2(updater)\
	updater##_2.reset();\
	##updater##_userdata_2.reset()

	// clean threads
	CLEAN_THREAD(worldupdater);
	CLEAN_THREAD(playerupdater);
	CLEAN_THREAD(zombieupdater);
	CLEAN_THREAD(viewmatrixupdater);
	CLEAN_THREAD(entityaddressupdater);
	CLEAN_THREAD(skeletonupdater);
	CLEAN_THREAD_2(skeletonupdater);
	CLEAN_THREAD(skeletonindicesupdater);

	threads.clear();
};

bool b4b::create_threads(const uint32_t target_fps, const bool& force_close, std::string* error/* = nullptr*/)
{
	// helper macros
#define DEFINE_THREAD(updater, ...)\
	updater## = std::make_unique<##updater##_t>();\
	##updater##_userdata = std::make_unique<##updater##_thread_userdata_t>(force_close, *##updater##, ##__VA_ARGS__##);\
	threads.push_back({##updater##_userdata.get(), &##updater##_thread_handle})

#define DEFINE_THREAD_2(updater, ...)\
	updater##_2 = std::make_unique<##updater##_t>();\
	##updater##_userdata_2 = std::make_unique<##updater##_thread_userdata_t>(force_close, *##updater##_2, ##__VA_ARGS__##);\
	threads.push_back({##updater##_userdata_2.get(), &##updater##_thread_handle_2})

#define CREATE_THREAD(updater)\
	{\
		##updater##_thread_handle = CreateThread(nullptr, 0ull, ##updater##_t::thread, ##updater##_userdata.get(), 0ul, nullptr);\
		if (##updater##_thread_handle == nullptr)\
		{\
			if (error != nullptr) *error = sstrenc("Failed to create ") + sstrenc(#updater) + sstrenc(" thread, exiting...");\
			end_threads();\
			return false;\
		}\
	}

#define CREATE_THREAD_2(updater)\
	{\
		##updater##_thread_handle_2 = CreateThread(nullptr, 0ull, ##updater##_t::thread, ##updater##_userdata_2.get(), 0ul, nullptr);\
		if (##updater##_thread_handle_2 == nullptr)\
		{\
			if (error != nullptr) *error = sstrenc("Failed to create ") + sstrenc(#updater) + sstrenc(" thread, exiting...");\
			end_threads();\
			return false;\
		}\
	}

	// defined threads will show up in the order here in the frametime/framerate info, but aren't actually created yet
	DEFINE_THREAD(viewmatrixupdater, target_fps);
	DEFINE_THREAD(playerupdater, target_fps);
	DEFINE_THREAD(zombieupdater, target_fps);
	DEFINE_THREAD(skeletonupdater, 30u, boneid_transform_entry_e::players); // TODO: get from config
	DEFINE_THREAD_2(skeletonupdater, 30u, boneid_transform_entry_e::zombies); // TODO: get from config
	DEFINE_THREAD(entityaddressupdater);
	DEFINE_THREAD(worldupdater);
	DEFINE_THREAD(skeletonindicesupdater);

	// create threads
	// note: any additional arguments can be passed after the updater
	// note 2: if any thread has a dependency on another thread, it has to be created AFTER the dependency thread
	CREATE_THREAD(worldupdater);
	CREATE_THREAD(entityaddressupdater); // has a dependency on worldupdater, so this has to come after
	CREATE_THREAD(playerupdater); // has a dependency on entityaddressupdater, so this has to come after
	CREATE_THREAD(viewmatrixupdater); // has a dependency on entityaddressupdater, so this has to come after
	CREATE_THREAD(skeletonindicesupdater);
	CREATE_THREAD(skeletonupdater);
	CREATE_THREAD(zombieupdater);
	CREATE_THREAD_2(skeletonupdater);

	return true;
}

const b4b::thread_list& b4b::get_threads(void)
{
	return threads;
}
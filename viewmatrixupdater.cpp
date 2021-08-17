/*
@file

	viewmatrixupdater.h

@purpose

	Thread for updating the viewmatrix
*/

#include "viewmatrixupdater.h"
#include "entityaddressupdater.h"
#include "offsets.h"
#include "../lib/fpsmanager.h"
#include "entities.h"
#include "d3dmatrix.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

namespace b4b
{

	namespace
	{

		auto Matrix = [](const vector3d& rot, vector3d origin = vector3d(0, 0, 0))
		{
			float radPitch = (rot.x * float(M_PI) / 180.f);
			float radYaw = (rot.y * float(M_PI) / 180.f);
			float radRoll = (rot.z * float(M_PI) / 180.f);

			float SP = sinf(radPitch);
			float CP = cosf(radPitch);
			float SY = sinf(radYaw);
			float CY = cosf(radYaw);
			float SR = sinf(radRoll);
			float CR = cosf(radRoll);

			D3DMATRIX matrix;

			matrix.m[0][0] = CP * CY;
			matrix.m[0][1] = CP * SY;
			matrix.m[0][2] = SP;
			matrix.m[0][3] = 0.f;

			matrix.m[1][0] = SR * SP * CY - CR * SY;
			matrix.m[1][1] = SR * SP * SY + CR * CY;
			matrix.m[1][2] = -SR * CP;
			matrix.m[1][3] = 0.f;

			matrix.m[2][0] = -(CR * SP * CY + SR * SY);
			matrix.m[2][1] = CY * SR - CR * SP * SY;
			matrix.m[2][2] = CR * CP;
			matrix.m[2][3] = 0.f;

			matrix.m[3][0] = origin.x;
			matrix.m[3][1] = origin.y;
			matrix.m[3][2] = origin.z;
			matrix.m[3][3] = 1.f;

			return matrix;
		};

	}

}

DWORD WINAPI b4b::viewmatrixupdater_t::thread(LPVOID param)
{
	auto userdata = reinterpret_cast<viewmatrixupdater_thread_userdata_t*>(param);

	userdata->instance.run(userdata);
	userdata->closed = true;

	return 0ul;
}

void b4b::viewmatrixupdater_t::run(viewmatrixupdater_thread_userdata_t* userdata)
{
	viewmatrix_t temp_viewmatrix;

	struct minimal_view_info_t final
	{

		vector3d location;
		vector3d rotation;

		float fov;

		// not needed
		/*float desired_fov;
		float ortho_width;
		float ortho_near_clip_plane;
		float ortho_far_clip_plane;
		float aspect_ratio;*/

	};

	auto extract_viewmatrix = [&temp_viewmatrix](void) -> void
	{
		const auto localplayer_address = entityaddressupdater->get_localplayer_address();

		if (localplayer_address == 0ull) return;

		const auto localplayer_controller = interfaces::memorymanager->read<uint64_t>(localplayer_address + offsets::player::controller);

		if (localplayer_controller == 0ull) return;

		const auto camera_manager = interfaces::memorymanager->read<uint64_t>(localplayer_controller + offsets::playercontroller::player_camera_manager);

		if (camera_manager == 0ull) return;

		const auto view_info = interfaces::memorymanager->read<minimal_view_info_t>(camera_manager + offsets::player_camera_manager::camera_cache_private + 0x10);

		temp_viewmatrix.location = view_info.location;
		temp_viewmatrix.fov = view_info.fov;
		temp_viewmatrix.matrix = Matrix(view_info.rotation);
		temp_viewmatrix.valid = true;
	};

	for (fpsmanager_t fpsmanager(userdata->target_fps, 1.f); !userdata->close; fpsmanager.run())
	{
		userdata->framerate = fpsmanager.get_framerate();
		userdata->last_frametime = static_cast<float>(fpsmanager.get_last_frametime());

		temp_viewmatrix.valid = false;

		try
		{
			extract_viewmatrix();
		}
		catch (...)
		{
			temp_viewmatrix.valid = false;
		}

		// update the viewmatrix
		set_viewmatrix(temp_viewmatrix);
	}
}
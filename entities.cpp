/*
@file

	entities.cpp

@purpose

	Easy access to all needed types of entities
*/

#include <shared_mutex>
#include <array>
#include <algorithm>
#include <unordered_map>
#include "entities.h"
#include "offsets.h"
#include "d3dmatrix.h"
#include "sdk.h"
#include "skeletonindicesupdater.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace b4b
{

	namespace
	{

		// mutexes
		std::array<std::shared_mutex, static_cast<size_t>(entity_type_e::SIZE)> mutexes;

		// localplayer
		std::shared_mutex localplayer_mutex;
		player_entity_t localplayer;

		// players
		std::deque<player_entity_t> players;

		// player skeletons
		std::deque<skeleton_entity_t> player_skeletons;

		// zombie skeletons
		std::deque<skeleton_entity_t> zombie_skeletons;

		// zombies
		std::deque<zombie_entity_t> zombies;

		struct fstring_t final
		{

			uint64_t location;

			int32_t length;

		};

		template <size_t N>
		bool read_fstring(const uint64_t address, wchar_t(&buffer)[N], std::string& output)
		{
			try
			{
				const auto fstring = interfaces::memorymanager->read<fstring_t>(address);

				auto len = std::min(N, static_cast<size_t>(fstring.length));

				if (len <= 1) return false;

				len -= 1;

				if (!interfaces::memorymanager->read_raw(fstring.location, buffer, len * sizeof(wchar_t))) return false;

				output.resize(len);

				for (size_t i = 0; i < len; i++) output[i] = static_cast<char>(buffer[i]);

				return true;
			}
			catch (...) {}

			return false;
		}

		wchar_t player_name_buffer[64]{};

		// for transforming bone positions
		struct FTransform
		{

			vector3d rotation;
			float rotation_w;
			vector3d translation;
			uint8_t pad[4];
			vector3d scale3d;

			D3DMATRIX to_matrix_with_scale(void)
			{
				D3DMATRIX m;

				m._41 = translation.x;
				m._42 = translation.y;
				m._43 = translation.z;

				float x2 = rotation.x + rotation.x;
				float y2 = rotation.y + rotation.y;
				float z2 = rotation.z + rotation.z;

				float xx2 = rotation.x * x2;
				float yy2 = rotation.y * y2;
				float zz2 = rotation.z * z2;

				m._11 = (1.0f - (yy2 + zz2)) * scale3d.x;
				m._22 = (1.0f - (xx2 + zz2)) * scale3d.y;
				m._33 = (1.0f - (xx2 + yy2)) * scale3d.z;

				float yz2 = rotation.y * z2;
				float wx2 = rotation_w * x2;

				m._32 = (yz2 - wx2) * scale3d.z;
				m._23 = (yz2 + wx2) * scale3d.y;

				float xy2 = rotation.x * y2;
				float wz2 = rotation_w * z2;

				m._21 = (xy2 - wz2) * scale3d.y;
				m._12 = (xy2 + wz2) * scale3d.x;

				float xz2 = rotation.x * z2;
				float wy2 = rotation_w * y2;

				m._31 = (xz2 + wy2) * scale3d.z;
				m._13 = (xz2 - wy2) * scale3d.x;

				m._14 = 0.0f;
				m._24 = 0.0f;
				m._34 = 0.0f;
				m._44 = 1.0f;

				return m;
			}

		};

		D3DMATRIX D3DXMatrixMultiply(const D3DMATRIX& pM1, const D3DMATRIX& pM2)
		{
			D3DMATRIX pOut;

			pOut._11 = pM1._11 * pM2._11 + pM1._12 * pM2._21 + pM1._13 * pM2._31 + pM1._14 * pM2._41;
			pOut._12 = pM1._11 * pM2._12 + pM1._12 * pM2._22 + pM1._13 * pM2._32 + pM1._14 * pM2._42;
			pOut._13 = pM1._11 * pM2._13 + pM1._12 * pM2._23 + pM1._13 * pM2._33 + pM1._14 * pM2._43;
			pOut._14 = pM1._11 * pM2._14 + pM1._12 * pM2._24 + pM1._13 * pM2._34 + pM1._14 * pM2._44;
			pOut._21 = pM1._21 * pM2._11 + pM1._22 * pM2._21 + pM1._23 * pM2._31 + pM1._24 * pM2._41;
			pOut._22 = pM1._21 * pM2._12 + pM1._22 * pM2._22 + pM1._23 * pM2._32 + pM1._24 * pM2._42;
			pOut._23 = pM1._21 * pM2._13 + pM1._22 * pM2._23 + pM1._23 * pM2._33 + pM1._24 * pM2._43;
			pOut._24 = pM1._21 * pM2._14 + pM1._22 * pM2._24 + pM1._23 * pM2._34 + pM1._24 * pM2._44;
			pOut._31 = pM1._31 * pM2._11 + pM1._32 * pM2._21 + pM1._33 * pM2._31 + pM1._34 * pM2._41;
			pOut._32 = pM1._31 * pM2._12 + pM1._32 * pM2._22 + pM1._33 * pM2._32 + pM1._34 * pM2._42;
			pOut._33 = pM1._31 * pM2._13 + pM1._32 * pM2._23 + pM1._33 * pM2._33 + pM1._34 * pM2._43;
			pOut._34 = pM1._31 * pM2._14 + pM1._32 * pM2._24 + pM1._33 * pM2._34 + pM1._34 * pM2._44;
			pOut._41 = pM1._41 * pM2._11 + pM1._42 * pM2._21 + pM1._43 * pM2._31 + pM1._44 * pM2._41;
			pOut._42 = pM1._41 * pM2._12 + pM1._42 * pM2._22 + pM1._43 * pM2._32 + pM1._44 * pM2._42;
			pOut._43 = pM1._41 * pM2._13 + pM1._42 * pM2._23 + pM1._43 * pM2._33 + pM1._44 * pM2._43;
			pOut._44 = pM1._41 * pM2._14 + pM1._42 * pM2._24 + pM1._43 * pM2._34 + pM1._44 * pM2._44;

			return pOut;
		}

		std::vector<uint8_t> skeleton_buffer;
		std::vector<uint8_t> zombie_skeleton_buffer;

		uint32_t lifestate_alive_name_index = 0u;

		using bonename_map_type = std::unordered_map<std::string, uint32_t>;

		std::unordered_map<uint64_t, bonename_map_type> zombie_bone_index_cache;

	}

	void set_players(const std::deque<player_entity_t>& players)
	{
		auto& mutex = mutexes.at(static_cast<size_t>(entity_type_e::PLAYER));

		std::lock_guard<std::shared_mutex> guard(mutex);

		::b4b::players = players;
	}

	std::deque<player_entity_t> get_players(void)
	{
		auto& mutex = mutexes.at(static_cast<size_t>(entity_type_e::PLAYER));

		std::shared_lock<std::shared_mutex> guard(mutex);

		return players;
	}

	void set_player_skeletons(const std::deque<skeleton_entity_t>& skeletons)
	{
		auto& mutex = mutexes.at(static_cast<size_t>(entity_type_e::PLAYER_SKELETON));

		std::lock_guard<std::shared_mutex> guard(mutex);

		::b4b::player_skeletons = skeletons;
	}

	std::deque<skeleton_entity_t> get_player_skeletons(void)
	{
		auto& mutex = mutexes.at(static_cast<size_t>(entity_type_e::PLAYER_SKELETON));

		std::shared_lock<std::shared_mutex> guard(mutex);

		return player_skeletons;
	}

	void set_zombie_skeletons(const std::deque<skeleton_entity_t>& skeletons)
	{
		auto& mutex = mutexes.at(static_cast<size_t>(entity_type_e::ZOMBIE_SKELETON));

		std::lock_guard<std::shared_mutex> guard(mutex);

		::b4b::zombie_skeletons = skeletons;
	}

	std::deque<skeleton_entity_t> get_zombie_skeletons(void)
	{
		auto& mutex = mutexes.at(static_cast<size_t>(entity_type_e::ZOMBIE_SKELETON));

		std::shared_lock<std::shared_mutex> guard(mutex);

		return zombie_skeletons;
	}

	void set_zombies(const std::deque<zombie_entity_t>& zombies)
	{
		auto& mutex = mutexes.at(static_cast<size_t>(entity_type_e::ZOMBIE));

		std::lock_guard<std::shared_mutex> guard(mutex);

		::b4b::zombies = zombies;
	}

	std::deque<zombie_entity_t> get_zombies(void)
	{
		auto& mutex = mutexes.at(static_cast<size_t>(entity_type_e::ZOMBIE));

		std::lock_guard<std::shared_mutex> guard(mutex);

		return zombies;
	}

	void set_localplayer(const player_entity_t& localplayer)
	{
		std::lock_guard<std::shared_mutex> guard(localplayer_mutex);

		::b4b::localplayer = localplayer;
	}

	bool get_localplayer(player_entity_t& localplayer)
	{
		std::shared_lock<std::shared_mutex> guard(localplayer_mutex);

		if (!::b4b::localplayer.valid) return false;

		localplayer = ::b4b::localplayer;

		return true;
	}

	// skeletonindicesupdater.cpp
	std::array<std::unordered_map<uint64_t, std::pair<timer_t, boneid_transform_type>>, static_cast<size_t>(boneid_transform_entry_e::size)> boneid_transforms;
	std::array<std::vector<uint64_t>, static_cast<size_t>(boneid_transform_entry_e::size)> updated_boneid_transforms;

}

b4b::base_entity_t::base_entity_t(const uint64_t address) : address(address) {}

b4b::base_entity_t::base_entity_t(const base_entity_t& other)
{
	*this = other;
}

b4b::base_entity_t& b4b::base_entity_t::operator = (const b4b::base_entity_t& other)
{
	// copy members
	address = other.address;

	return *this;
}

uint64_t b4b::base_entity_t::get_sockets_address(void) const
{
	if (address == 0ull) return 0ull;

	const auto mesh = interfaces::memorymanager->read<uint64_t>(address + offsets::character::mesh);

	if (mesh == 0ull) return 0ull;

	const auto skeletal_mesh = interfaces::memorymanager->read<uint64_t>(mesh + offsets::skinned_mesh_component::skeletal_mesh);

	return skeletal_mesh == 0ull ? 0ull : interfaces::memorymanager->read<uint64_t>(skeletal_mesh + offsets::skeleton::sockets);
}

b4b::player_entity_t::player_entity_t(void) : base_entity_t(0ull), valid(false), health(0.f), max_health(0.f) {}
b4b::player_entity_t::player_entity_t(const uint64_t address) : base_entity_t(address), valid(false), health(0.f), max_health(0.f) {}

b4b::player_entity_t::player_entity_t(const player_entity_t& other) : base_entity_t(other.address)
{
	*this = other;
}

b4b::player_entity_t& b4b::player_entity_t::operator = (const b4b::player_entity_t& other)
{
	// copy members
	address = other.address;
	valid = other.valid;

	if (valid)
	{
		angle = other.angle;
		origin = other.origin;
		name = other.name;
		health = other.health;
		max_health = other.max_health;
	}

	return *this;
}

void b4b::player_entity_t::update(void)
{
	valid = false;

	if (address == 0ull) return;

	try
	{
		// check if the player has a valid state
		const auto state = interfaces::memorymanager->read<uint64_t>(address + offsets::pawn::player_state);

		if (state == 0ull) return;

		// need access to the root component for origin and angle
		const auto root_component = interfaces::memorymanager->read<uint64_t>(address + offsets::actor::root_component);

		if (root_component == 0ull) return;

		// get origin and angle
		origin = interfaces::memorymanager->read<vector3d>(root_component + offsets::scenecomponent::relative_location);
		angle = interfaces::memorymanager->read<vector2d>(root_component + offsets::scenecomponent::relative_rotation);

		// read name
		read_fstring(state + offsets::player_state::player_name_private, player_name_buffer, name);

		// get current health & max health
		const auto health_component = interfaces::memorymanager->read<uint64_t>(address + offsets::hero_bp::health);

		if (health_component == 0ull) return;

		health = interfaces::memorymanager->read<float>(health_component + offsets::health_component::health);
		max_health = interfaces::memorymanager->read<float>(health_component + offsets::health_component::max_health);
	}
	catch (...)
	{
		return;
	}

	valid = true;
}

b4b::zombie_entity_t::zombie_entity_t(void) : base_entity_t(0ull), valid(false) {}
b4b::zombie_entity_t::zombie_entity_t(const uint64_t address) : base_entity_t(address), valid(false) {}

b4b::zombie_entity_t::zombie_entity_t(const zombie_entity_t& other) : base_entity_t(other.address)
{
	*this = other;
}

b4b::zombie_entity_t& b4b::zombie_entity_t::operator = (const b4b::zombie_entity_t& other)
{
	// copy members
	address = other.address;
	valid = other.valid;

	if (valid)
	{
		angle = other.angle;
		origin = other.origin;
	}

	return *this;
}

void b4b::zombie_entity_t::update(void)
{
	valid = false;

	if (address == 0ull) return;

	try
	{
		// need access to the root component for origin and angle
		const auto root_component = interfaces::memorymanager->read<uint64_t>(address + offsets::actor::root_component);

		if (root_component == 0ull) return;

		// get origin and angle
		origin = interfaces::memorymanager->read<vector3d>(root_component + offsets::scenecomponent::relative_location);
		angle = interfaces::memorymanager->read<vector2d>(root_component + offsets::scenecomponent::relative_rotation);

		// check if zombie is alive
		const auto life_state_comp = interfaces::memorymanager->read<uint64_t>(address + offsets::gobi_character::life_state_comp);

		if (life_state_comp == 0ull) return;

		const auto active_life_state_tag_name_index = interfaces::memorymanager->read<uint32_t>(life_state_comp + offsets::life_state_component::active_life_state_tag);

		if (active_life_state_tag_name_index == 0ull) return;

		if (lifestate_alive_name_index == 0u)
		{
			const auto life_state_name = get_name(static_cast<uint64_t>(active_life_state_tag_name_index));

			if (life_state_name.empty() || life_state_name.compare(sstrenc("LifeState.Alive")) != 0) return;

			lifestate_alive_name_index = active_life_state_tag_name_index;
		}
		else if (active_life_state_tag_name_index != lifestate_alive_name_index) return;
	}
	catch (...)
	{
		return;
	}

	valid = true;
}

b4b::skeleton_entity_t::skeleton_entity_t(void) : base_entity_t(0ull), valid(false), type(boneid_transform_entry_e::size) {}
b4b::skeleton_entity_t::skeleton_entity_t(uint64_t address, const boneid_transform_entry_e type) : base_entity_t(address), valid(false), type(type) {}

b4b::skeleton_entity_t::skeleton_entity_t(const skeleton_entity_t& other) : base_entity_t(other.address)
{
	*this = other;
}

b4b::skeleton_entity_t& b4b::skeleton_entity_t::operator = (const b4b::skeleton_entity_t& other)
{
	// copy members
	address = other.address;
	valid = other.valid;

	if (valid)
	{
		type = other.type;
		bones = other.bones;
	}

	return *this;
}

b4b::vector3d b4b::skeleton_entity_t::get_bone_position(const boneids_e bone) const
{
	return bones.at(static_cast<size_t>(bone));
}

void b4b::skeleton_entity_t::update(void)
{
	valid = false;

	try
	{
		auto& updated_boneid_transform = updated_boneid_transforms.at(static_cast<size_t>(type));
		const auto& boneid_transform = boneid_transforms.at(static_cast<size_t>(type));
		const auto mesh = interfaces::memorymanager->read<uint64_t>(address + offsets::character::mesh);

		if (mesh == 0ull) return;

		const auto skeletal_mesh = interfaces::memorymanager->read<uint64_t>(mesh + offsets::skinned_mesh_component::skeletal_mesh);

		if (skeletal_mesh == 0ull) return;

		const auto sockets = interfaces::memorymanager->read<uint64_t>(skeletal_mesh + offsets::skeleton::sockets);

		if (sockets == 0ull) return;

		const auto& f = boneid_transform.find(sockets);

		if (f == boneid_transform.end()) return;

		const auto component_to_world_matrix = interfaces::memorymanager->read<FTransform>(mesh + offsets::skeleton::component_to_world_matrix).to_matrix_with_scale();
		const auto bone_start = interfaces::memorymanager->read<uint64_t>(mesh + offsets::skeleton::bones);

		if (bone_start == 0ull) return;

		const auto& boneid_translation = f->second.second;
		const auto min_index = *std::min_element(boneid_translation.begin(), boneid_translation.end());
		const auto max_index = *std::max_element(boneid_translation.begin(), boneid_translation.end());
		constexpr uint32_t bone_entry_size = 48u;
		const uint32_t required_size = (max_index - min_index) * bone_entry_size;
		const uint32_t read_offset = min_index * bone_entry_size;

		if (skeleton_buffer.size() < required_size) skeleton_buffer.resize(required_size);
		if (!interfaces::memorymanager->read_raw(bone_start + read_offset, skeleton_buffer.data(), required_size)) return;

		for (size_t i = 0; i < boneid_translation.size(); i++)
		{
			const auto bone_matrix = reinterpret_cast<FTransform*>(skeleton_buffer.data() + static_cast<uint64_t>(boneid_translation.at(i) - min_index) * bone_entry_size)->to_matrix_with_scale();
			const auto final_matrix = D3DXMatrixMultiply(bone_matrix, component_to_world_matrix);
			const auto bone_position = vector3d{ final_matrix._41, final_matrix._42, final_matrix._43 };

			bones.at(i) = bone_position;
		}

		updated_boneid_transform.push_back(sockets);
	}
	catch (...)
	{
		return;
	}

	valid = true;
}
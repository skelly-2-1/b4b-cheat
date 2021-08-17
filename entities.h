/*
@file

	entities.h

@purpose

	Easy access to all needed types of entities
*/

#pragma once

#include <stdint.h>
#include <string>
#include <deque>
#include <array>
#include <vector>
#include "../lib/vector.h"

namespace b4b
{

	enum class entity_type_e
	{

		PLAYER,
		PLAYER_SKELETON,
		ZOMBIE,
		ZOMBIE_SKELETON,

		// do not modify
		SIZE

	};

	class base_entity_t
	{

	protected:



	private:



	public:

		uint64_t address = 0ull;

		base_entity_t(const uint64_t address);
		base_entity_t(const base_entity_t& other);

		virtual ~base_entity_t() = default;
		virtual void update(void) = 0;

		base_entity_t& operator = (const base_entity_t& other);

		uint64_t get_sockets_address(void) const;

	};

	enum class boneids_e : uint8_t
	{

		root,
		neck,
		pelvis,
		right_elbow,
		right_hand,
		left_elbow,
		left_hand,
		left_shoulder,
		right_shoulder,
		left_knee,
		left_foot,
		right_knee,
		right_foot,
		head,

		size

	};

	enum class boneid_transform_entry_e
	{

		players,
		zombies,

		// do not modify
		size

	};

	// separate class for skeletons because they take a long time to update
	class skeleton_entity_t final : public base_entity_t
	{

	protected:



	private:



	public:

		bool valid;

		std::array<vector3d, static_cast<size_t>(boneids_e::size)> bones;

		boneid_transform_entry_e type;

		skeleton_entity_t(void);
		skeleton_entity_t(uint64_t address, const boneid_transform_entry_e type);
		skeleton_entity_t(const skeleton_entity_t& other);

		void update(void) override;

		skeleton_entity_t& operator = (const skeleton_entity_t& other);

		vector3d get_bone_position(const boneids_e bone) const;

	};

	class player_entity_t final : public base_entity_t
	{

	protected:



	private:



	public:

		float health, max_health;

		bool valid;

		vector3d origin;
		vector2d angle;

		std::string name;

		player_entity_t(void);
		player_entity_t(const uint64_t address);
		player_entity_t(const player_entity_t& other);

		void update(void) override;

		player_entity_t& operator = (const player_entity_t& other);

	};

	class zombie_entity_t final : public base_entity_t
	{

	protected:



	private:



	public:

		bool valid;

		vector3d origin;
		vector2d angle;

		zombie_entity_t(void);
		zombie_entity_t(const uint64_t address);
		zombie_entity_t(const zombie_entity_t& other);

		void update(void) override;

		zombie_entity_t& operator = (const zombie_entity_t& other);

	};

	// entities.cpp
	extern void set_localplayer(const player_entity_t& localplayer);
	extern bool get_localplayer(player_entity_t& localplayer);

	extern void set_players(const std::deque<player_entity_t>& players);
	extern std::deque<player_entity_t> get_players(void);

	extern void set_zombies(const std::deque<zombie_entity_t>& zombies);
	extern std::deque<zombie_entity_t> get_zombies(void);

	extern void set_player_skeletons(const std::deque<skeleton_entity_t>& skeletons);
	extern std::deque<skeleton_entity_t> get_player_skeletons(void);

	extern void set_zombie_skeletons(const std::deque<skeleton_entity_t>& skeletons);
	extern std::deque<skeleton_entity_t> get_zombie_skeletons(void);

}
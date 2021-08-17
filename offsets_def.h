/*
@file

	offsets_def.h

@purpose

	Defining of all our offsets, will be automatically applied to the header and .cpp (initialized to 0)
*/

namespace b4b
{

	namespace offsets
	{

		namespace gnames
		{

			OFFSET(uint64_t, pointer); // GNames

		}

		namespace gobjects
		{

			OFFSET(uint64_t, pointer); // GObjects

		}

		namespace uworld
		{

			OFFSET(uint64_t, pointer); // UWorld
			OFFSET(uint32_t, owning_game_instance); // OwningGameInstance
			OFFSET(uint32_t, persistent_level); // PersistentLevel
			OFFSET(uint32_t, levels); // Levels

		}

		namespace skeleton
		{

			OFFSET(uint32_t, bones);
			OFFSET(uint32_t, sockets);
			OFFSET(uint32_t, component_to_world_matrix);

		}

		// GameInstance
		namespace gameinstance
		{

			OFFSET(uint32_t, local_players); // LocalPlayers

		}

		// Player
		namespace player
		{

			OFFSET(uint32_t, controller); // PlayerController

		}

		// Controller
		namespace controller
		{

			OFFSET(uint32_t, player_state); // PlayerState
			OFFSET(uint32_t, control_rotation); // ControlRotation

		}

		// Actor
		namespace actor
		{

			OFFSET(uint32_t, root_component); // RootComponent

		}

		// PlayerController
		namespace playercontroller
		{

			OFFSET(uint32_t, acknowledged_pawn); // AcknowledgedPawn
			OFFSET(uint32_t, player_camera_manager); // PlayerCameraManager

		}

		// SceneComponent
		namespace scenecomponent
		{

			OFFSET(uint32_t, relative_location); // RelativeLocation
			OFFSET(uint32_t, relative_rotation); // RelativeRotation

		}

		// Pawn
		namespace pawn
		{

			OFFSET(uint32_t, player_state); // PlayerState
			OFFSET(uint32_t, controller); // Controller

		}

		// PlayerState
		namespace player_state
		{

			OFFSET(uint32_t, player_name_private);

		}

		// Character
		namespace character
		{

			OFFSET(uint32_t, mesh);

		}

		// GobiCharacter
		namespace gobi_character
		{

			OFFSET(uint32_t, life_state_comp);

		}

		// LifeStateComponent
		namespace life_state_component
		{

			OFFSET(uint32_t, active_life_state_tag);

		}

		// PlayerCameraManager
		namespace player_camera_manager
		{

			OFFSET(uint32_t, camera_cache_private);

		}

		// SkeletalMeshComponent
		namespace skeletal_mesh_component
		{

			OFFSET(uint32_t, cached_bone_space_transforms);

		}

		// SkinnedMeshComponent
		namespace skinned_mesh_component
		{

			OFFSET(uint32_t, skeletal_mesh);

		}

		// Hero_BP_C
		namespace hero_bp
		{

			OFFSET(uint32_t, health);

		}

		// HealthComponent
		namespace health_component
		{

			OFFSET(uint32_t, health);
			OFFSET(uint32_t, max_health);

		}

	}

}
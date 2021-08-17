/*
@file

    main.cpp

@purpose

    Main program entry point
*/

#include <Windows.h>
#include <chrono>
#include <thread>
#include "../lib/external_cheat_interfaces/memorymanager.h"
#include "../lib/external_cheat_interfaces/overlayrenderer.h"
#include "../lib/external_cheat_interfaces/io.h"
#include "../lib/external_cheat_interfaces/cfg.h"
#include "../lib/external_cheat_interfaces/sigscanner.h"
#include "../lib/fpsmanager.h"
#include "../lib/json.h"
#include "../lib/vector.h"
#include "sdk.h"
#include "config.h"
#include "threads.h"
#include "playerupdater.h"
#include "zombieupdater.h"
#include "viewmatrixupdater.h"

#define OFFSET(t, o) t o = 0
#include "offsets_def.h"
#undef OFFSET

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace b4b
{

    // our main thread
    DWORD __stdcall main_thread(void* param);

    // actual function where we will be doing or devious deeds
    void main(void);

    // the path where the loader was started
    namespace interfaces
    {

        external_cheat_interfaces::memorymanager_t* memorymanager = nullptr;
        external_cheat_interfaces::overlayrenderer_t* overlayrenderer = nullptr;
        external_cheat_interfaces::cfg_t* cfg = nullptr;
        external_cheat_interfaces::io_t* io = nullptr;
        external_cheat_interfaces::sigscanner_t* sigscanner = nullptr;

    }

    namespace modules
    {

        module_t main;

    }

    config_t config;

}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID param)
{
    if (reason == DLL_PROCESS_ATTACH) return CreateThread(nullptr, 0ull, b4b::main_thread, nullptr, 0ul, nullptr) != nullptr;

    return TRUE;
}

DWORD b4b::main_thread(void* param)
{
    b4b::main();

    interfaces::overlayrenderer->cleanup();

    return 0ul;
}

void b4b::main(void)
{
    // get the main module
    modules::main = interfaces::memorymanager->get_main_module();

    // parse the config
    config.create(interfaces::cfg);

    // TODO: check if any features are active

    // initialize the overlay
    bool timeout = true;

    std::string error;

    uint32_t loader_target_fps = 0u;

    for (std::chrono::high_resolution_clock::time_point timeout_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(20); std::chrono::high_resolution_clock::now() < timeout_point; std::this_thread::sleep_for(std::chrono::milliseconds(100)))
    {
        if (!interfaces::overlayrenderer->initialize(sstrenc("UnrealWindow"), sstrenc("Back 4 Blood  "), loader_target_fps, &error)) continue;

        timeout = false;

        break;
    }

    if (timeout) return;

    auto target_fps = loader_target_fps;

    if (config.misc_target_fps >= 0) target_fps = std::max(std::min(target_fps, 500u), 30u);

    // needed signatures
    external_cheat_interfaces::sigscanner_t::patterns_t signatures{

        external_cheat_interfaces::sigscanner_t::create_pattern(offsets::gnames::pointer, sstrenc("48 8D 35 ? ? ? ? EB 16")),
        external_cheat_interfaces::sigscanner_t::create_pattern(offsets::gobjects::pointer, sstrenc("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8D 04 D1 EB")),
        external_cheat_interfaces::sigscanner_t::create_pattern(offsets::uworld::pointer, sstrenc("48 8B 1D ? ? ? ? 48 85 DB 74 3B 41"))

    };

    timeout = true;

    // grab all the patterns we need
    for (std::chrono::high_resolution_clock::time_point timeout_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(10); std::chrono::high_resolution_clock::now() < timeout_point; std::this_thread::sleep_for(std::chrono::milliseconds(100)))
    {
        if (!interfaces::sigscanner->find_patterns_in_section(modules::main, signatures, sstrenc(".xpdata"))) continue;

        timeout = false;

        break;
    }

    if (timeout) return;

    // these offsets are relative to their code location, resolve them
    offsets::gnames::pointer += static_cast<int64_t>(interfaces::memorymanager->read<int32_t>(offsets::gnames::pointer + 3)) + 7;
    offsets::gobjects::pointer += static_cast<int64_t>(interfaces::memorymanager->read<int32_t>(offsets::gobjects::pointer + 3)) + 7;
    offsets::uworld::pointer += static_cast<int64_t>(interfaces::memorymanager->read<int32_t>(offsets::uworld::pointer + 3)) + 7;

    // dump class offsets and retrieve them
    timeout = true;

    for (std::chrono::high_resolution_clock::time_point timeout_point = std::chrono::high_resolution_clock::now() + std::chrono::seconds(10); std::chrono::high_resolution_clock::now() < timeout_point; std::this_thread::sleep_for(std::chrono::milliseconds(100)))
    {
        nlohmann::json json;

        const auto object_array = interfaces::memorymanager->read<TUObjectArray>(offsets::gobjects::pointer);

        for (uint32_t i = 0; i < object_array.num_elements; i++)
        {
            try
            {
                const auto object_address = object_array.get_object(i);

                if (object_address == 0ull) continue;

                const auto uobject = interfaces::memorymanager->read<UStruct>(object_address);

                if (uobject.cls == 0ull) continue;

                auto fullname = uobject.get_full_name();

                if (fullname.empty()) continue;

                const bool is_class = fullname.substr(0, sizeof("Class ") - 1).compare(sstrenc("Class ")) == 0;
                const bool is_blueprint_generated_class = !is_class && fullname.substr(0, sizeof("BlueprintGeneratedClass ") - 1).compare(sstrenc("BlueprintGeneratedClass ")) == 0;

                if (!is_class && !is_blueprint_generated_class) continue;

                fullname = fullname.substr((is_class ? sizeof("Class ") : sizeof("BlueprintGeneratedClass ")) - 1);

                const auto children = uobject.children;

                if (children == 0ull) continue;

                for (auto cur = children; cur != 0ull;)
                {
                    try
                    {
                        const auto property = interfaces::memorymanager->read<FProperty>(cur);
                        const auto property_name = property.get_name();

                        if (!property_name.empty()) json[fullname][property_name] = property.offset;

                        if (property.next == 0ull) break;

                        cur = property.next;
                    }
                    catch (...)
                    {
                        break;
                    }
                }
            }
            catch (...) {}
        }

        const auto get_class_offset = [&json](const std::string& cls, const std::string& prop) -> uint32_t
        {
            if (json.empty()) throw std::runtime_error(sstrenc("json is empty"));

            const auto class_iterator = json.find(cls);

            if (class_iterator == json.end()) throw std::runtime_error(sstrenc("Class \"") + cls + sstrenc("\" not found"));
            if (class_iterator->empty()) throw std::runtime_error(sstrenc("Class \"") + cls + sstrenc("\" has no props"));

            const auto prop_iterator = class_iterator->find(prop);

            if (prop_iterator == class_iterator->end()) throw std::runtime_error(sstrenc("Class \"") + cls + sstrenc("\" has no property named \"") + prop + sstrenc("\""));

            return static_cast<uint32_t>(prop_iterator->get<int32_t>());
        };

        try
        {
            // World
            offsets::uworld::owning_game_instance = get_class_offset(sstrenc("Engine.World"), sstrenc("OwningGameInstance"));
            offsets::uworld::persistent_level = get_class_offset(sstrenc("Engine.World"), sstrenc("PersistentLevel"));
            offsets::uworld::levels = get_class_offset(sstrenc("Engine.World"), sstrenc("Levels"));

            // GameInstance
            offsets::gameinstance::local_players = get_class_offset(sstrenc("Engine.GameInstance"), sstrenc("LocalPlayers"));

            // Player
            offsets::player::controller = get_class_offset(sstrenc("Engine.Player"), sstrenc("PlayerController"));

            // Controller
            offsets::controller::player_state = get_class_offset(sstrenc("Engine.Controller"), sstrenc("PlayerState"));
            offsets::controller::control_rotation = get_class_offset(sstrenc("Engine.Controller"), sstrenc("ControlRotation"));

            // PlayerController
            offsets::playercontroller::acknowledged_pawn = get_class_offset(sstrenc("Engine.PlayerController"), sstrenc("AcknowledgedPawn"));
            offsets::playercontroller::player_camera_manager = get_class_offset(sstrenc("Engine.PlayerController"), sstrenc("PlayerCameraManager"));

            // Actor
            offsets::actor::root_component = get_class_offset(sstrenc("Engine.Actor"), sstrenc("RootComponent"));

            // SceneComponent
            offsets::scenecomponent::relative_location = get_class_offset(sstrenc("Engine.SceneComponent"), sstrenc("RelativeLocation"));
            offsets::scenecomponent::relative_rotation = get_class_offset(sstrenc("Engine.SceneComponent"), sstrenc("RelativeRotation"));

            // Pawn
            offsets::pawn::player_state = get_class_offset(sstrenc("Engine.Pawn"), sstrenc("PlayerState"));
            offsets::pawn::controller = get_class_offset(sstrenc("Engine.Pawn"), sstrenc("Controller"));

            // PlayerState
            offsets::player_state::player_name_private = get_class_offset(sstrenc("Engine.PlayerState"), sstrenc("PlayerNamePrivate"));

            // Character
            offsets::character::mesh = get_class_offset(sstrenc("Engine.Character"), sstrenc("Mesh"));

            // GobiCharacter
            offsets::gobi_character::life_state_comp = get_class_offset(sstrenc("Gobi.GobiCharacter"), sstrenc("LifeStateComp"));

            // LifeStateComponent
            offsets::life_state_component::active_life_state_tag = get_class_offset(sstrenc("Gobi.LifeStateComponent"), sstrenc("ActiveLifeStateTag"));

            // PlayerCameraManager
            offsets::player_camera_manager::camera_cache_private = get_class_offset(sstrenc("Engine.PlayerCameraManager"), sstrenc("CameraCachePrivate"));

            // SkeletalMeshComponent
            offsets::skeletal_mesh_component::cached_bone_space_transforms = get_class_offset(sstrenc("Engine.SkeletalMeshComponent"), sstrenc("CachedBoneSpaceTransforms"));

            // SkinnedMeshComponent
            offsets::skinned_mesh_component::skeletal_mesh = get_class_offset(sstrenc("Engine.SkinnedMeshComponent"), sstrenc("SkeletalMesh"));

            // Hero_BP_C
            offsets::hero_bp::health = get_class_offset(sstrenc("Hero_BP.Hero_BP_C"), sstrenc("Health"));

            // HealthComponent
            offsets::health_component::health = get_class_offset(sstrenc("Gobi.HealthComponent"), sstrenc("Health")); // note: TemporaryHealth also exists?
            offsets::health_component::max_health = get_class_offset(sstrenc("Gobi.HealthComponent"), sstrenc("CurrentMaxHealth"));
        }
        catch (const std::exception& e)
        {
            error = e.what();

            continue;
        }

        timeout = false;

        break;
    }

    if (timeout) return;

    // TODO: figure out a way to get these offsets dynamically
    offsets::skeleton::bones = 0x538;
    offsets::skeleton::component_to_world_matrix = 0x1F0;
    offsets::skeleton::sockets = 0x1C0;

    // offsets successfully grabbed, create threads
    bool force_close = false;

    if (!create_threads(target_fps, force_close, &error))
    {
        MessageBoxA(nullptr, error.c_str(), strenc("b4b"), MB_SETFOREGROUND | MB_ICONERROR);

        return;
    }

    // things needed for drawing
    vector2d cached_screen_resolution{}, cached_screen_resolution_half{};

    float text_size_min, text_size_max, cached_aspect_ratio;
    bool dynamic_text_scaling;
    vector2d last_resolution{};
    std::deque<player_entity_t> players;
    std::deque<zombie_entity_t> zombies;
    std::deque<skeleton_entity_t> player_skeletons;
    std::deque<skeleton_entity_t> zombie_skeletons;
    viewmatrix_t viewmatrix;
    player_entity_t localplayer;

    // utility functions
    auto world_to_screen = [&viewmatrix, &cached_screen_resolution_half](const vector3d& pos, vector2d& screen) -> bool
    {
        const auto& matrix = viewmatrix.matrix;
        const vector3d axis[] = {

            vector3d(matrix.m[0][0], matrix.m[0][1], matrix.m[0][2]),
            vector3d(matrix.m[1][0], matrix.m[1][1], matrix.m[1][2]),
            vector3d(matrix.m[2][0], matrix.m[2][1], matrix.m[2][2])

        };

        const auto delta = pos - viewmatrix.location;
        auto transformed = vector3d(delta.dot(axis[1]), delta.dot(axis[2]), delta.dot(axis[0]));

        if (transformed.z < 1.f) transformed.z = 1.f;

        const auto fov = viewmatrix.fov;
        const auto& screen_center_x = cached_screen_resolution_half.x;
        const auto& screen_center_y = cached_screen_resolution_half.y;

        screen = {

            screen_center_x + transformed.x * (screen_center_x / tanf(fov * (float)M_PI / 360.f)) / transformed.z,
            screen_center_y - transformed.y * (screen_center_x / tanf(fov * (float)M_PI / 360.f)) / transformed.z

        };

        return true;
    };

    auto draw_entity_info_text = [](const base_entity_t* entity, const vector2d& pos, const uint32_t size, const std::string& text)
    {
        const auto is_player = dynamic_cast<const player_entity_t*>(entity) != nullptr;
        const auto esp_info_text_shadow = is_player ? config.player_esp_info_text_shadow : config.zombie_esp_info_text_shadow;
        const auto esp_info_text_color = is_player ? config.player_esp_info_text_color : config.zombie_esp_info_text_color;

        if (esp_info_text_shadow)
        {
            const auto esp_info_text_shadow_color = is_player ? config.player_esp_info_text_shadow_color : config.zombie_esp_info_text_shadow_color;

            interfaces::overlayrenderer->draw_shadow_string_unformatted(pos, esp_info_text_color, esp_info_text_shadow_color, size, text);
        }
        else
        {
            interfaces::overlayrenderer->draw_string_unformatted(pos, esp_info_text_color, size, text);
        }
    };

    auto visualize_bounding_box_3d = [&world_to_screen](vector2d screen_coordinates[8], const vector3d& bbmin, const vector3d& bbmax, const vector3d& origin, const float yaw, float scale_width = 1.f, float scale_height = 1.f)
    {
        auto min = bbmin;
        auto max = bbmax;

        min.x *= scale_width;
        min.z *= scale_width;
        min.y *= scale_height;

        max.x *= scale_width;
        max.z *= scale_width;
        max.y *= scale_height;

        vector3d points[] = {

            vector3d(max.x, min.y, min.z),
            vector3d(max.x, max.y, min.z),
            vector3d(min.x, max.y, min.z),
            vector3d(min.x, min.y, min.z),
            vector3d(max.x, min.y, max.z),
            vector3d(max.x, max.y, max.z),
            vector3d(min.x, max.y, max.z),
            vector3d(min.x, min.y, max.z)

        };

        vector3d bounding_box_center = origin;

        bounding_box_center.y += bbmax.y * .5f;
        bounding_box_center.y -= max.y * .5f;

        const float rad = yaw * static_cast<float>(M_PI / 180.0);
        const float s = -sin(rad);
        const float c = cos(rad);

        for (uint8_t i = 0; i <= 7; ++i)
        {
            const auto x = points[i].x;
            const auto y = points[i].y;

            if (yaw != 0.f)
            {
                points[i].x = (x * c) - (y * s);
                points[i].y = (y * c) + (x * s);
            }

            points[i] += bounding_box_center;

            if (!world_to_screen(points[i], screen_coordinates[i])) return false;
        }

        return true;
    };

    auto draw_bounding_box_3d = [](external_cheat_interfaces::overlayrenderer_t* overlayrenderer, const vector2d bounding_box[8], const color_t clr, const float thickness = 1.f)
    {
        constexpr int32_t bounding_box_matrix[][2] = {

            // bottom
            { 0, 1 },
            { 1, 2 },
            { 2, 3 },
            { 3, 0 },

            // top
            { 4, 5 },
            { 5, 6 },
            { 6, 7 },
            { 7, 4 },

            // sides
            { 0, 4 },
            { 1, 5 },
            { 2, 6 },
            { 3, 7 }

        };

        for (auto& matrix : bounding_box_matrix) overlayrenderer->draw_line(bounding_box[matrix[0]], bounding_box[matrix[1]], clr, thickness);
    };

    auto visualize_bounding_box_2d = [](external_cheat_interfaces::overlayrenderer_t* overlayrenderer, vector2d bounding_box[8])
    {
        float l = bounding_box[0].x;
        float t = bounding_box[0].y;
        float r = bounding_box[0].x;
        float b = bounding_box[0].y;

        for (int32_t i = 1; i < 8; i++)
        {
            if (l > bounding_box[i].x) l = bounding_box[i].x;
            if (t > bounding_box[i].y) t = bounding_box[i].y;
            if (r < bounding_box[i].x) r = bounding_box[i].x;
            if (b < bounding_box[i].y) b = bounding_box[i].y;
        }

        return std::make_pair(vector3d(std::roundf(l), std::roundf(t)), vector2d(std::roundf(r), std::roundf(b)));
    };

    const auto get_distance_from_diff = [](const vector3d& diff) -> float
    {
        return diff.length() / 100.f;
    };

    const auto get_distance = [&localplayer, &get_distance_from_diff](const base_entity_t* entity) -> float
    {
        if (const auto player = dynamic_cast<const player_entity_t*>(entity))
        {
            return get_distance_from_diff(localplayer.origin - player->origin);
        }
        else if (const auto zombie = dynamic_cast<const zombie_entity_t*>(entity))
        {
            return get_distance_from_diff(localplayer.origin - zombie->origin);
        }

        return 0.f;
    };

    const auto draw_skeleton = [&world_to_screen](external_cheat_interfaces::overlayrenderer_t* overlayrenderer, const color_t& color, const skeleton_entity_t* skeleton, const uint32_t thickness) -> void
    {
        constexpr std::array<std::array<boneids_e, 2ull>, 11ull> bone_list{{

             { boneids_e::left_elbow, boneids_e::left_hand },
             { boneids_e::right_elbow, boneids_e::right_hand },
             { boneids_e::neck, boneids_e::left_shoulder },
             { boneids_e::neck, boneids_e::right_shoulder },
             { boneids_e::left_shoulder, boneids_e::left_elbow },
             { boneids_e::right_shoulder, boneids_e::right_elbow },
             { boneids_e::neck, boneids_e::pelvis },
             { boneids_e::pelvis, boneids_e::left_knee },
             { boneids_e::left_knee, boneids_e::left_foot },
             { boneids_e::pelvis, boneids_e::right_knee },
             { boneids_e::right_knee, boneids_e::right_foot }

        }};

        for (const auto& bone : bone_list)
        {
            vector2d start, end;

            if (!world_to_screen(skeleton->get_bone_position(bone.at(0)), start) ||
                !world_to_screen(skeleton->get_bone_position(bone.at(1)), end)) continue;

            overlayrenderer->draw_line(start, end, color, static_cast<float>(thickness));
        }
    };

    const auto draw_skeletal_entity = [&](external_cheat_interfaces::overlayrenderer_t* overlayrenderer, const base_entity_t* entity, const skeleton_entity_t* skeleton)
    {
        if (entity == nullptr || skeleton == nullptr) return;

        const auto is_player = dynamic_cast<const player_entity_t*>(entity) != nullptr;
        const auto bone_esp_active = is_player ? config.player_bone_esp_active : config.zombie_bone_esp_active;
        const auto bone_esp_max_distance = is_player ? config.player_bone_esp_max_distance : config.zombie_bone_esp_max_distance;

        // skeleton esp
        if (bone_esp_active && (bone_esp_max_distance == 0.f || get_distance(entity) <= bone_esp_max_distance))
        {
            const auto bone_esp_color = is_player ? config.player_bone_esp_color : config.zombie_bone_esp_color;
            const auto bone_esp_thickness = is_player ? config.player_bone_esp_thickness : config.zombie_bone_esp_thickness;

            draw_skeleton(overlayrenderer, bone_esp_color, &(*skeleton), bone_esp_thickness);
        }

        const auto box_esp_active = is_player ? config.player_box_esp_active : config.zombie_box_esp_active;
        const auto esp_distance = is_player ? config.player_esp_distance : config.zombie_esp_distance;
        const auto esp_healthbar = is_player ? config.player_esp_healthbar : false;

        if (!box_esp_active && !esp_distance && !esp_healthbar) return;

        vector2d screen[8];

        const auto size = vector2d{ 80.f, 220.f };
        const auto bbmin = vector3d{ -size.x * .5f, -size.x * .5f, -5.f };
        const auto bbmax = vector3d{ size.x * .5f, size.x * .5f, ((skeleton->get_bone_position(boneids_e::head).z - skeleton->get_bone_position(boneids_e::root).z)) + 30.f };
        const auto angle = is_player ? dynamic_cast<const player_entity_t*>(entity)->angle : dynamic_cast<const zombie_entity_t*>(entity)->angle;
        const auto bbox_scale_width = is_player ? config.player_esp_bbox_scale_width : config.zombie_esp_bbox_scale_width;
        const auto bbox_scale_height = is_player ? config.player_esp_bbox_scale_height : config.zombie_esp_bbox_scale_height;

        if (!visualize_bounding_box_3d(screen, bbmin, bbmax, skeleton->get_bone_position(boneids_e::root), angle.y, bbox_scale_width, bbox_scale_height)) return;

        const auto box_esp_3d = is_player ? config.player_box_esp_3d : config.zombie_box_esp_3d;

        if (box_esp_active && box_esp_3d)
        {
            draw_bounding_box_3d(overlayrenderer, screen, is_player ? config.player_box_esp_color : config.zombie_box_esp_color, is_player ? static_cast<float>(config.player_box_esp_thickness) : static_cast<float>(config.zombie_box_esp_thickness));

            if (!esp_distance && !esp_healthbar) return;
        }

        float l, r, t, b;
        {
            const auto coords = visualize_bounding_box_2d(overlayrenderer, screen);

            l = coords.first.x; // left
            t = coords.first.y; // top
            r = coords.second.x; // right
            b = coords.second.y; // bottom
        }

        if (box_esp_active && !box_esp_3d)
        {
            const auto clr = is_player ? config.player_box_esp_color : config.zombie_box_esp_color;
            const auto thickness = is_player ? static_cast<float>(config.player_box_esp_thickness) : static_cast<float>(config.zombie_box_esp_thickness);

            overlayrenderer->draw_line(vector2d(l, b), vector2d(l, t), clr, thickness);
            overlayrenderer->draw_line(vector2d(l, t), vector2d(r, t), clr, thickness);
            overlayrenderer->draw_line(vector2d(r, t), vector2d(r, b), clr, thickness);
            overlayrenderer->draw_line(vector2d(r, b), vector2d(l, b), clr, thickness);
        }

        // check if anything else is active
        if (!esp_distance && !esp_healthbar) return;

        // draw health bar
        const auto s = (cached_screen_resolution.y / 400.f) + box_esp_active ? (is_player ? static_cast<float>(config.player_box_esp_thickness) : static_cast<float>(config.zombie_box_esp_thickness)) : 0.f; // spacing from box to other elements

        // healthbar
        if (esp_healthbar && b - t >= 5.f)
        {
            const auto health = dynamic_cast<const player_entity_t*>(entity)->health;
            const auto max_health = dynamic_cast<const player_entity_t*>(entity)->max_health;
            const auto thickness_ = std::min(std::max(3.f, (r - l) * .05f), std::max(3.f, s));
            const auto health_scale = std::max(std::min(health / max_health, 1.f), 0.f);
            const auto health_color = color_t(static_cast<uint8_t>((1.f - health_scale) * 255), static_cast<uint8_t>(health_scale * 255), 0);

            vector2d healthbar_pos{ l - s - thickness_, t };
            vector2d healthbar_size{ thickness_, b - t };

            overlayrenderer->draw_filled_rect(healthbar_pos, healthbar_size, color_t(0, 0, 0));

            auto inner_healthbar_pos = healthbar_pos + vector2d{ 1.f, 1.f };
            auto inner_healthbar_size = healthbar_size - vector2d{ 2.f, 2.f };

            const auto offset = std::floorf(inner_healthbar_size.y * health_scale);

            inner_healthbar_pos.y += (inner_healthbar_size.y - offset);
            inner_healthbar_size.y = offset;

            overlayrenderer->draw_filled_rect(inner_healthbar_pos, inner_healthbar_size, health_color);
        }

        if (esp_distance) return;

        // draw distance
        float bottom_offset = 0.f;

        // calculate the text size
        const float text_size = dynamic_text_scaling ? std::min(std::max(std::floorf(std::min(r - l, b - t) * config.misc_esp_text_scaling_factor), text_size_min), text_size_max) : text_size_min;

        if (esp_distance)
        {
            const auto distance_text = std::to_string(static_cast<int32_t>(get_distance(entity)))/* + 'm'*/; // ace's fault
            const auto calculated_text_size = overlayrenderer->calc_text_size(static_cast<uint32_t>(text_size), distance_text);

            if (calculated_text_size.x != 0.f && calculated_text_size.y != 0.f) // wait until the font is loaded
            {
                draw_entity_info_text(entity, { l + ((r - l) * .5f) - calculated_text_size.x * .5f, b/* + s*/ + bottom_offset }, static_cast<uint32_t>(text_size), distance_text);
            }
        }
    };

    auto draw = [&](external_cheat_interfaces::overlayrenderer_t* overlayrenderer) -> void
    {
        constexpr float text_size_min_pixels = 6.f;
        {
            const auto current_resolution = overlayrenderer->get_resolution();

            if (last_resolution.x != current_resolution.x || last_resolution.y != current_resolution.y)
            {
                cached_screen_resolution = interfaces::overlayrenderer->get_resolution();
                cached_aspect_ratio = cached_screen_resolution.x / cached_screen_resolution.y;
                cached_screen_resolution_half = cached_screen_resolution * .5f;
                last_resolution = current_resolution;
                text_size_min = std::floorf(std::max(std::floorf(cached_screen_resolution.y / 90.f), text_size_min_pixels) * config.misc_esp_min_text_scale); // smallest possible text size
                dynamic_text_scaling = config.misc_esp_text_scaling;

                if (dynamic_text_scaling)
                {
                    text_size_max = std::floorf((text_size_min * 2.f) * config.misc_esp_max_text_scale); // biggest possible text size

                    if (text_size_min == text_size_max)
                    {
                        dynamic_text_scaling = false;
                    }
                    else
                    {
                        // check if min is above max, in which case, swap them around
                        if (text_size_min > text_size_max) std::swap(text_size_min, text_size_max);

                        // if any text features are enabled, preload the esp font range
                        if (config.player_esp_distance || config.zombie_esp_distance)
                        {
                            overlayrenderer->preload_font_range(static_cast<uint32_t>(text_size_min), static_cast<uint32_t>(text_size_max));
                        }
                    }
                }
            }
        }

        if (!get_localplayer(localplayer)) return;

        viewmatrix = viewmatrixupdater->get_viewmatrix();

        if (!viewmatrix.valid) return;

        // draw zombies
        if (config.zombie_bone_esp_active || config.zombie_box_esp_active || config.zombie_esp_distance)
        {
            zombies = get_zombies();

            if (!zombies.empty() && !(zombie_skeletons = get_zombie_skeletons()).empty())
            {
                for (const auto& zombie : zombies)
                {
                    const auto skeleton = std::find_if(zombie_skeletons.begin(), zombie_skeletons.end(), [&zombie](const skeleton_entity_t& skel) { return skel.address == zombie.address; });

                    if (skeleton == zombie_skeletons.end()) continue;

                    draw_skeletal_entity(overlayrenderer, &zombie, &(*skeleton));
                }
            }
        }

        // draw players
        if (config.player_bone_esp_active || config.player_box_esp_active || config.player_esp_distance || config.player_esp_healthbar)
        {
            players = get_players();

            if (!players.empty())
            {
                player_skeletons = get_player_skeletons();
            }
            else
            {
                player_skeletons.clear();
            }

            if (players.empty()) return;

            for (const auto& player : players)
            {
                const auto skeleton = std::find_if(player_skeletons.begin(), player_skeletons.end(), [&player](const skeleton_entity_t& skel) { return skel.address == player.address; });

                if (skeleton == player_skeletons.end()) continue;

                draw_skeletal_entity(overlayrenderer, &player, &(*skeleton));
            }
        }
    };

    for (fpsmanager_t fpsmanager(target_fps, 1.f);;)
    {
        if (!interfaces::overlayrenderer->update_overlay()) continue;
        if (interfaces::overlayrenderer->start_drawing())
        {
            try
            {
                draw(interfaces::overlayrenderer);
            }
            catch (...) {}

            // draw fps/frametimes if need be
            if (config.misc_showfps || config.misc_showframetime)
            {
                char fpsinfo_text[256]{};

                auto handle_fps_text = [&fpsinfo_text](const float framerate, const float frametime)
                {
                    if (config.misc_showfps && config.misc_showframetime)
                    {
                        const auto fmt = sstrenc("%s%s%.0f (%.2fms)");

                        sprintf_s(fpsinfo_text, fmt.c_str(), fpsinfo_text, strlen(fpsinfo_text) == 0 ? "" : strenc(" - "), framerate, frametime);
                    }
                    else if (config.misc_showfps)
                    {
                        const auto fmt = sstrenc("%s%s%.0f");

                        sprintf_s(fpsinfo_text, fmt.c_str(), fpsinfo_text, strlen(fpsinfo_text) == 0 ? "" : strenc(" - "), framerate);
                    }
                    else
                    {
                        const auto fmt = sstrenc("%s%s%.2fms");

                        sprintf_s(fpsinfo_text, fmt.c_str(), fpsinfo_text, strlen(fpsinfo_text) == 0 ? "" : strenc(" - "), frametime);
                    }
                };

                handle_fps_text(fpsmanager.get_framerate(), static_cast<float>(fpsmanager.get_last_frametime()));

                for (const auto& thread : get_threads())
                {
                    if (*thread.second == nullptr) continue;

                    handle_fps_text(thread.first->framerate, thread.first->last_frametime);
                }

                if (strlen(fpsinfo_text) > 0)
                {
                    interfaces::overlayrenderer->draw_string_unformatted({ 0.f,0.f }, color_t(255, 255, 255), static_cast<uint32_t>(floorf(10.f * (interfaces::overlayrenderer->get_resolution().y / 480.f))), fpsinfo_text);
                }
            }

            interfaces::overlayrenderer->end_drawing();
        }

        fpsmanager.run();
    }

    end_threads();
}
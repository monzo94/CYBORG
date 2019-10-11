#include "version.h"

#include <lazarus/common.h>
#include <lazarus/Graphics.h>
#include <lazarus/FOV.h>
#include <lazarus/Random.h>
#include "VisibilitySystem.h"
#include "InputSystem.h"
#include "RenderSystem.h"
#include "MovementSystem.h"
#include "AISystem.h"
#include "Components.h"
#include "Dungeon.h"

using namespace std;

// Map dimensions
constexpr int MAP_WIDTH = 70;
constexpr int MAP_HEIGHT = 70;

// Tile IDs
// TODO: Remove this
constexpr int PLAYER_IMG = 0;
constexpr int MONSTER_IMG = 3;

// TODO: Move abstract game loop logic to lazarus
void game_loop(lz::ECSEngine &engine, lz::Window &window)
{
    while (window.is_open())
    {
        // Process events
        lz::Event event;
        while (window.poll_event(event))
        {
            switch (event.type)
            {
            case lz::Event::Closed:
                window.close();
                break;
            case lz::Event::KeyPressed:
                // TODO: Add key press to a queue and handle them all later to be
                // able to parse key combinations
                engine.emit(KeyPressedEvent{event});
                break;
            }
        }

        // Update all systems that require updating
        engine.update();

        // Render window
        window.render();
    }
}

int main(int argc, char const *argv[])
{
    DEBUG("Version %d.%d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR);

    // Create engine
    lz::ECSEngine engine;

    // Create player entity and set it at the center
    lz::Entity player;
    player.add_component<lz::Position2D>(MAP_WIDTH / 2, MAP_HEIGHT / 2);
    player.add_component<Renderable>(PLAYER_IMG);
    player.add_component<Player>();
    engine.add_entity(player);

    // Add some NPCs
    lz::Entity npc;
    npc.add_component<lz::Position2D>(MAP_WIDTH / 2 - 15, MAP_HEIGHT / 2 - 15);
    npc.add_component<Renderable>(MONSTER_IMG);
    npc.add_component<AI>(AIBehaviour::Follow);
    engine.add_entity(npc);

    // Create map with one big room where the player can walk freely
    Dungeon &dungeon = Dungeon::instance();
    int level = dungeon.generate_new_level(MAP_WIDTH, MAP_HEIGHT);
    dungeon.switch_level(level);
    lz::SquareGridMap &map = dungeon.get_level();
    
    // Player position is always floor
    map.set_walkable(MAP_WIDTH / 2, MAP_HEIGHT / 2, true);
    map.set_transparency(MAP_WIDTH / 2, MAP_HEIGHT / 2, true);

    // Create display window and load tileset
    lz::Window window(20, 20);
    window.load_tileset("../res/test_tileset_48x48.png");

    // Create systems and subscribe them
    VisibilitySystem visibility_system(10);
    InputSystem input_system;
    MovementSystem movement_system;
    RenderSystem render_system(window);
    AISystem ai_system;
    // Subscribe the systems as event listeners
    engine.subscribe<EntityMovedEvent>(&visibility_system);
    engine.subscribe<KeyPressedEvent>(&input_system);
    engine.subscribe<MovementIntentEvent>(&movement_system);
    engine.subscribe<RefreshAI>(&ai_system);
    // Set render system to update every tick
    engine.register_updateable(&render_system);

    // Emit a first event so that the FOV is computed on game start
    engine.emit<EntityMovedEvent>({});

    // TODO: Store current map in engine, or somewhere more accessible for events
    game_loop(engine, window);

    return 0;
}

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "font/font_inc.h"
#include "render/render_inc.h"
#include "sound/sound_inc.h"
#include "async/async_inc.h"
#include "game/game_inc.h"

#include "base/base_inc.cpp"
#include "os/os_inc.cpp"
#include "font/font_inc.cpp"
#include "render/render_inc.cpp"
#include "sound/sound_inc.cpp"
#include "async/async_inc.cpp"
#include "game/game_inc.cpp"

struct GameState {
  Arena *arena;
  OS_Handle window;
  R_Context *renderer;
  R_Font *font;
  B32 quit;
  
  G_UndoContext undo;
  G_AssetContext *assets;
  G_Level level;
  G_Input input;
  G_MineState mine;
  B32 lost;
};

global GameState game;

function void 
update(void)
{
  OS_Handle window = game.window;
  
  OS_EventList *events = os_get_events();
  for (OS_Event *e = events->first; e != 0; e = e->next) {
    if (e->kind == OS_EventKind_WindowClose) {
      game.quit = 1;
    }
  }
  g_get_input(window, &game.input, events);
  
  F32 delta_time = 0;
  {
    local F64 prev_ticks = 0;
    F64 curr_ticks = os_get_ticks();
    delta_time = (F32)(curr_ticks - prev_ticks);
    delta_time = delta_time / (F32)os_get_ticks_frequency();
    prev_ticks = curr_ticks; 
  }
  
  // @Todo: Wrap all these operations in level-granularity helper functions
  if (g_key_pressed(&game.input, G_Key_Z)) {
    G_TileMap *map = game.level.map;
    g_undo_pop(&game.undo, map);
    game.level.player = g_player_from_tilemap(map);
    game.level.ground_enemies_count = g_ground_enemies_from_tilemap(game.level.ground_enemies, map);
    game.level.air_enemies_count = g_air_enemies_from_tilemap(game.level.air_enemies, map);
    G_LevelBoulderInfo boulders = g_boulders_from_tilemap(game.level.small_boulders, game.level.large_boulders, map);
    game.level.small_boulders_count = boulders.small_count;
    game.level.large_boulders_count = boulders.large_count;
    
    game.mine.v += 1;
    game.mine.v = Min(game.mine.v, game.mine.max);
    game.mine.ore_collected -= 1;
    game.mine.ore_collected = Max(game.mine.ore_collected, 0);
    
    game.lost = 0;
  }
  else if (g_key_pressed(&game.input, G_Key_R)) {
    G_TileMap *map = game.level.map;
    g_undo_restart(&game.undo, map);
    game.level.player = g_player_from_tilemap(map);
    game.level.ground_enemies_count = g_ground_enemies_from_tilemap(game.level.ground_enemies, map);
    game.level.air_enemies_count = g_air_enemies_from_tilemap(game.level.air_enemies, map);
    G_LevelBoulderInfo boulders = g_boulders_from_tilemap(game.level.small_boulders, game.level.large_boulders, map);
    game.level.small_boulders_count = boulders.small_count;
    game.level.large_boulders_count = boulders.large_count;
    
    game.mine.v = game.mine.max;
    game.mine.ore_collected = 0;
    
    game.lost = 0;
  }
  
  if (!game.lost) {
    G_LevelSimResult sim = g_simulate_level(&game.level, &game.undo, &game.input, &game.mine, delta_time);
    if (sim.lost) {
      game.lost = 1;
    }
  }
}

function void 
render(void)
{
  OS_Handle window = game.window;
  R_Context *renderer = game.renderer;
  G_AssetContext *assets = game.assets;
  
  r_set_clear_color(v4f32(0,0,0,1));
  r_set_viewport(os_window_client_rect(window));
  r_begin_frame(renderer); 
  
  {
    g_render_level(renderer, assets, &game.level);
    
    V4F32 col = v4f32(0.6f,0.526f,0.622f,1.f);
    
    F32 picaxe_bar_w = 128;
    F32 picaxe_bar_h = 32;
    
    F32 picaxe_bar_x = 700;
    F32 picaxe_bar_y = 40;
    
    {
      R_Quad quad = {0};
      quad.rect = 
        rect_f32(picaxe_bar_x, picaxe_bar_y, picaxe_bar_x+picaxe_bar_w, picaxe_bar_y+picaxe_bar_h);
      quad.colors[0] = col;
      quad.colors[1] = col;
      quad.colors[2] = col;
      quad.colors[3] = col;
      quad.corner_softness = 0.5f;
      quad.border_thickness = 3.f;
      quad.radius = 5.f;
      r_quad(renderer, &quad, 0);
    }
    
    {
      R_Quad quad = {0};
      F32 w = picaxe_bar_w * ((F32)game.mine.v/(F32)game.mine.max);
      quad.rect = rect_f32(picaxe_bar_x, picaxe_bar_y, picaxe_bar_x+w, picaxe_bar_y+picaxe_bar_h);
      quad.colors[0] = col;
      quad.colors[1] = col;
      quad.colors[2] = col;
      quad.colors[3] = col;
      quad.corner_softness = 0.5f;
      quad.radius = 5.f;
      r_quad(renderer, &quad, 0);
    }
    
    {
      TempArena tmp = arena_temp_begin(game.arena);
      
      r_text(renderer, S8("Picaxe: "), game.font, 32, v2f32(picaxe_bar_x,0), col, 0);
      
      U32 ore_collected = game.mine.ore_collected;
      U32 ore_count = game.level.ore_count;
      
      String8 ore_progress_str = str8_pushf(tmp.arena, "Ores: %d/%d", ore_collected, ore_count);
      r_text(renderer, ore_progress_str, game.font, 32, v2f32(picaxe_bar_x, 92), col, 0);
      
      arena_temp_end(tmp);
    }
    
    if (game.lost) {
      r_text(renderer, S8("YOU LOST! Press Z to undo or R to restart."), 
             game.font, 32, v2f32(picaxe_bar_x, 128), v4f32(0.6f,0.4f,0.4f,1.f), 0);
    }
  }
  
  r_flush(renderer);
  r_end_frame(renderer);
}

void 
entry_point(void) 
{
  os_init();
  os_gfx_init();
  r_init(); 
  
  game.arena = arena_alloc_default();
  game.window = os_window_open(S8("Game"), 1280, 720);
  
  r_equip_window(game.window);
  game.renderer = r_context_alloc();
  game.font = r_font_ttf_parse_and_bake(game.arena, S8("../data/fonts/Ithaca.ttf"));
  game.assets = g_assets_alloc(S8("../data"));
  
  // Temp init
  G_TileMap map = {0};
#if 0
  G_TileToken tokens[] = {
    G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
    G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Player,G_TileToken_EnemyGround,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_StoneT1,G_TileToken_OreT2,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
  };
  map.width = 6;
  map.height = 6;
#endif
#if 0
  G_TileToken tokens[] = {
    G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_StoneT1,G_TileToken_Dirt,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_BoulderSmall,G_TileToken_Player,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_BoulderLarge,G_TileToken_Dirt,
    G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
    G_TileToken_StoneT1,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_EnemyGround,G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_EnemyAir,G_TileToken_Dirt,G_TileToken_StoneT1,G_TileToken_OreT2,G_TileToken_Dirt,G_TileToken_EnemyGround,G_TileToken_StoneT1,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_OreT2,G_TileToken_Dirt,
  };
  map.width = 8;
  map.height = 6;
#endif
  G_TileToken tokens[] = {
    G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_StoneT1,G_TileToken_Dirt,G_TileToken_Dirt,
    G_TileToken_Player,G_TileToken_BoulderSmall,G_TileToken_BoulderSmall,G_TileToken_EnemyAir,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_BoulderLarge,G_TileToken_Empty,
    G_TileToken_StoneT1,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_StoneT1,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
    G_TileToken_StoneT1,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_EnemyGround,G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_EnemyAir,G_TileToken_Dirt,G_TileToken_StoneT1,G_TileToken_OreT2,G_TileToken_Dirt,G_TileToken_BoulderSmall,G_TileToken_StoneT1,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_OreT2,G_TileToken_Dirt,
  };
  map.width = 8;
  map.height = 6;
  
  MemoryCopy(map.tokens, tokens, map.width*map.height*sizeof(G_TileToken));
  
  game.level = g_level_from_tilemap(&map);
  game.level.map = &map;
  
  game.undo = g_undo_make();
  
  // @Note: Starting value may differ across levels or change in some other way.
  game.mine.max = 10;
  game.mine.v = game.mine.max;
  
  while (!game.quit) {
    update();
    render();
  }
  
  g_undo_release(&game.undo);
  g_assets_release(game.assets);
  r_context_release(game.renderer);
  os_window_close(game.window);
  arena_release(game.arena);
}
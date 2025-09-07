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
  G_AssetContext *assets;
  
  S32 current_map_idx; 
  G_TileMap current_map;
  G_MapStorage maps;
  
  G_UndoContext undo;
  G_Level level;
  G_Input input;
  G_MineState mine;
  
  M_Context *mixer;
  M_Sound *sound;
  
  B32 quit;
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
  
  S32 new_map_idx = game.current_map_idx;
  
  // DEBUG: Swap current map 
  if (g_key_pressed(&game.input, G_Key_F2)) {
    new_map_idx += 1;
    new_map_idx = Min(new_map_idx, G_MAPS_MAX);
  }
  if (g_key_pressed(&game.input, G_Key_F1)) {
    new_map_idx -= 1;
    new_map_idx = Max(new_map_idx, 0);
  }
  if (new_map_idx != game.current_map_idx) {
    game.current_map = g_maps_get_copy(&game.maps, new_map_idx);
    game.current_map_idx = new_map_idx;
    
    game.level = g_level_from_tilemap(&game.current_map);
    game.level.map = &game.current_map;
    
    g_undo_clear(&game.undo);
  }
  
  // DEBUG: Load editor changes
  if (g_key_pressed(&game.input, G_Key_F12)) {
    game.maps = g_maps_read(S8("../data/maps/default.maps"));
    game.current_map = g_maps_get_copy(&game.maps, game.current_map_idx);
    
    G_TileMap *map = &game.maps.maps[game.current_map_idx];
    game.level = g_level_from_tilemap(map);
    game.level.map = map;
  }
  
  static F64 prev_ticks = 0;
  F64 curr_ticks = os_get_ticks();
  F32 delta_time = (F32)(curr_ticks - prev_ticks) / (F32)os_get_ticks_frequency();
  prev_ticks = curr_ticks; 
  
  if (g_key_pressed(&game.input, G_Key_Z)) {
    G_TileMap *map = game.level.map;
    g_undo_pop(&game.undo, map);
    
    g_level_refresh_entities(&game.level);
    
    game.mine.v += 1;
    game.mine.v = Min(game.mine.v, game.mine.max);
    game.mine.ore_collected -= 1;
    game.mine.ore_collected = Max(game.mine.ore_collected, 0);
    
    game.lost = 0;
  }
  else if (g_key_pressed(&game.input, G_Key_R)) {
    G_TileMap *map = game.level.map;
    g_undo_restart(&game.undo, map);
    
    g_level_refresh_entities(&game.level);
    
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
  
  //
  // Sounds
  //
  
  m_begin_frame(game.mixer);
  m_process(game.mixer);
  m_mix(game.mixer);
  m_end_frame(game.mixer);
  
#if 0
  {
    s_lock_buffer();
    
    U64 write_count = s_get_write_count();
    U64 play_count = s_get_play_count();
    U32 bytes_per_sample = s_win32_backend->sample_buff_fmt->nBlockAlign; 
    
    s_source_update(&game.source, play_count);
    
    F32 *buff = s_get_sample_buffer();
    MemorySet(buff, 0, write_count * bytes_per_sample);
    
    s_source_mix(&game.source, buff, write_count, 1.f);
    
    s_unlock_buffer();
    s_advance_buffer(write_count);
  }
#endif
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
  s_init(48000, 2, SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);
  
  game.arena = arena_alloc_default();
  game.window = os_window_open(S8("Shrewm"), 1280, 720);
  
  r_equip_window(game.window);
  game.renderer = r_context_alloc();
  game.font = r_font_ttf_parse_and_bake(game.arena, S8("../data/fonts/Ithaca.ttf"));
  game.assets = g_assets_alloc(S8("../data"));
  
  game.mixer = m_context_alloc();
  
  game.sound = m_sound_from_file(game.mixer, S8("../data/sounds/mine_stone.mp3"));
  m_sound_set_loop(game.sound, 1);
  m_sound_set_pan(game.sound, 0);
  
  game.maps = g_maps_read(S8("../data/maps/default.maps"));
  if (game.maps.count > 0) {
    U32 last_played_idx = game.maps.last_played_map_idx;
    G_TileMap last_played_map = g_maps_get_copy(&game.maps, last_played_idx);
    
    game.current_map_idx = last_played_idx;
    game.current_map = last_played_map;
    game.level = g_level_from_tilemap(&game.current_map);
    game.level.map = &game.current_map;
  }
  else {
    // @Todo: Show an error MessageBox?
  }
  
  game.undo = g_undo_make();
  
  // @Note: Starting value may differ across levels or change in some other way.
  game.mine.max = 10;
  game.mine.v = game.mine.max;
  
  while (!game.quit) {
    update();
    render();
  }
  
  //g_maps_write(&game.maps, S8("../data/maps/default.maps"));
  
  m_context_release(game.mixer);
  g_undo_release(&game.undo);
  g_assets_release(game.assets);
  r_context_release(game.renderer);
  os_window_close(game.window);
  arena_release(game.arena);
}
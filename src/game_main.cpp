#include "base/base_inc.h"
#include "os/os_inc.h"
#include "font/font_inc.h"
#include "render/render_inc.h"
#include "sound/sound_inc.h"
#include "game/game_inc.h"

#include "base/base_inc.cpp"
#include "os/os_inc.cpp"
#include "font/font_inc.cpp"
#include "render/render_inc.cpp"
#include "sound/sound_inc.cpp"
#include "game/game_inc.cpp"

// @Note: Level select menu (i.e. offsets) should be edited in editor

/*

  - Each level gets its own level select menu offset; either relative to previous level or relative to top left corner
    of the screen. This offset can be edited in the editor.

  - A tunnel should run between each sequential completed level. As a level's offsets in the level select screen will
    probably be adjusted frequently, this tunnel's path should be calculated automatically using a shortest path algorithm or
    something along those lines, while avoiding diagonals. This can be done in the game's code rather than the editor.

  - Tunneling algorithm can't connect arbitrary tiles on two levels. Gonna choose to just have them all connect in their vertical
    center.

  - When the player wants to go from a level to level select and vice versa, just fade to black.
 */

enum G_Mode {
  G_Mode_PlayLevel,
  G_Mode_SelectLevel,
  G_Mode_Menu,
  G_Mode_Pause
};

struct G_BlackoutState {
  F32 v = 1.f;
  G_TransitionF32 tr;
};

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
  G_ResourceContext rsrc;
  G_Level level;
  G_Input input;
  G_BlackoutState blackout_state;

  M_Context *mixer;
  M_Sound *sound;

  B32 quit;
  B32 lost;
  B32 level_finished;
};

global GameState game;

function void
update(F32 delta_time)
{
  OS_Handle window = game.window;

  OS_EventList *events = os_get_events();
  for (OS_Event *e = events->first; e != 0; e = e->next) {
    if (e->kind == OS_EventKind_WindowClose) {
      game.quit = 1;
    }
  }
  g_get_input(window, &game.input, events);

  {
    // DEBUG: Swap current map
    S32 new_map_idx = game.current_map_idx;

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
      g_rsrc_init(&game.rsrc, G_PLAYER_RESOURCE_START, game.level.ore_count);

      g_undo_clear(&game.undo);
    }

    // DEBUG: Load editor changes
    if (g_key_pressed(&game.input, G_Key_F12)) {
      g_undo_clear(&game.undo);

      game.maps = g_maps_read(S8("../data/maps/default.maps"));
      game.current_map = g_maps_get_copy(&game.maps, game.current_map_idx);

      G_TileMap *map = &game.maps.maps[game.current_map_idx];
      game.level = g_level_from_tilemap(map);
      game.level.map = map;
    }
  }

  if (g_key_pressed(&game.input, G_Key_Z)) {
    G_TileMap *map = game.level.map;
    g_undo_pop(&game.undo, map, &game.rsrc.state);

    g_level_refresh_entities(&game.level);

    game.lost = 0;
    game.blackout_state.v = 1.f;
  }
  else if (g_key_pressed(&game.input, G_Key_R)) {
    G_TileMap *map = game.level.map;
    g_undo_restart(&game.undo, map, &game.rsrc.state);

    g_level_refresh_entities(&game.level);

    game.lost = 0;
    game.blackout_state.v = 1.f;
  }

  if (!game.lost) {
    G_LevelSimResult sim = g_simulate_level(&game.level, &game.undo, &game.input, &game.rsrc, delta_time);
    if (sim.lost) {
      g_transition_f32_start(&game.blackout_state.tr, game.blackout_state.v, 0.f, 0.4f, G_TransitionCurve_Linear);
      game.lost = 1;
    }
    if (sim.finished) {
      g_transition_f32_start(&game.blackout_state.tr, game.blackout_state.v, 0.f, 0.2f, G_TransitionCurve_Bump);
      game.level_finished = 1;
    }
  }

  if (game.lost) {
    g_transition_f32_update(&game.blackout_state.tr, &game.blackout_state.v, delta_time);
  }

  g_transition_f32_update(&game.blackout_state.tr, &game.blackout_state.v, delta_time);
  if (game.level_finished) {
    // @Note: Temporary code
    if (absf32(game.blackout_state.tr.dst - game.blackout_state.v) <= 0.f) {
      S32 new_map_idx = game.current_map_idx;
      new_map_idx += 1;
      new_map_idx = Min(new_map_idx, G_MAPS_MAX);

      if (new_map_idx != game.current_map_idx) {
        game.current_map = g_maps_get_copy(&game.maps, new_map_idx);
        game.current_map_idx = new_map_idx;

        game.level = g_level_from_tilemap(&game.current_map);
        game.level.map = &game.current_map;
        g_rsrc_init(&game.rsrc, G_PLAYER_RESOURCE_START, game.level.ore_count);
        g_undo_clear(&game.undo);

        game.level_finished = 0;
      }
    }
  }

  m_begin_frame(game.mixer);
  m_process(game.mixer);
  m_mix(game.mixer);
  m_end_frame(game.mixer);
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
    F32 scale = 64;
    F32 screen_width  = 1264.f;
    F32 screen_height = 681.f;

    F32 tile_size = scale;
    F32 tiles_x = (F32)game.level.map->width;
    F32 tiles_y = (F32)game.level.map->height;

    F32 total_width  = tiles_x * tile_size;
    F32 total_height = tiles_y * tile_size;

    F32 base_x = (screen_width - total_width) * 0.5f;
    F32 base_y = (screen_height - total_height) * 0.5f;
    g_render_level(renderer, assets, &game.level, base_x, base_y, scale);

    V4F32 col = v4f32(0.9f,0.9f,0.9f, game.blackout_state.v);

    F32 picaxe_bar_w = 128;
    F32 picaxe_bar_h = 16;

    F32 picaxe_bar_x = 32;
    F32 picaxe_bar_y_base = 16;
    F32 picaxe_bar_y = picaxe_bar_y_base + 32;

    {
      R_Quad quad = {0};
      quad.rect =
        rect_f32(picaxe_bar_x, picaxe_bar_y, picaxe_bar_x+picaxe_bar_w, picaxe_bar_y+picaxe_bar_h);
      quad.colors[0] = col;
      quad.colors[1] = col;
      quad.colors[2] = col;
      quad.colors[3] = col;
      quad.border_thickness = 3.f;
      r_quad(renderer, &quad, 0);
    }

    {
      R_Quad quad = {0};
      F32 w = picaxe_bar_w * ((F32)g_rsrc_get_rsrc_count(&game.rsrc)/(F32)game.rsrc.rsrc_max);
      quad.rect = rect_f32(picaxe_bar_x, picaxe_bar_y, picaxe_bar_x+w, picaxe_bar_y+picaxe_bar_h);
      quad.colors[0] = col;
      quad.colors[1] = col;
      quad.colors[2] = col;
      quad.colors[3] = col;
      r_quad(renderer, &quad, 0);
    }

    {
      TempArena tmp = arena_temp_begin(game.arena);

      r_text(renderer, S8("Resources"), game.font, 24, v2f32(picaxe_bar_x,picaxe_bar_y_base), col, 0);

      U32 ore_collected = g_rsrc_get_ores_count(&game.rsrc);
      U32 ore_count = game.level.ore_count;

      String8 ore_progress_str = str8_pushf(tmp.arena, "Ores: %d/%d", ore_collected, ore_count);
      r_text(renderer, ore_progress_str, game.font, 24, v2f32(picaxe_bar_x, 92), col, 0);

      arena_temp_end(tmp);
    }

    R_Font *font = game.font;

    F32 loss_font_size = 32;
    V4F32 loss_text_col = v4f32(0.95f, 0.95f, 0.95f, 1.f-game.blackout_state.v);

    String8 loss_str_0 = S8("Z to Undo");
    V2S32 text_dim_0 = f_text_string_dim_px(loss_str_0, font->cache, loss_font_size);
    F32 x0 = (screen_width  - text_dim_0.x) * 0.5f;

    String8 loss_str_1 = S8("R to Restart");
    V2S32 text_dim_1 = f_text_string_dim_px(loss_str_1, font->cache, loss_font_size);
    F32 x1 = (screen_width  - text_dim_1.x) * 0.5f;

    F32 loss_text_vertical_spacing = 48;
    F32 total_text_height = text_dim_0.y + text_dim_1.y + loss_text_vertical_spacing;
    F32 lost_text_y = (screen_height - total_text_height) * 0.5f;

    {
      R_Quad quad = {0};
      V4F32 col = v4f32(0, 0, 0, 1.f-game.blackout_state.v);
      quad.rect = rect_f32(0, 0, screen_width, screen_height);
      quad.colors[0] = col;
      quad.colors[1] = col;
      quad.colors[2] = col;
      quad.colors[3] = col;
      r_quad(renderer, &quad, 0);
    }

    if (game.lost) {
      r_text(renderer, loss_str_0, game.font, loss_font_size, v2f32(x0, lost_text_y + 0), loss_text_col, 0);
      r_text(renderer, loss_str_1, game.font, loss_font_size, v2f32(x1, lost_text_y + loss_text_vertical_spacing), loss_text_col, 0);
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
  Assert(game.maps.count > 0);

  U32 last_played_idx = game.maps.last_played_map_idx;
  G_TileMap last_played_map = g_maps_get_copy(&game.maps, last_played_idx);
  game.current_map_idx = last_played_idx;
  game.current_map = last_played_map;
  game.level = g_level_from_tilemap(&game.current_map);
  game.level.map = &game.current_map;
  g_rsrc_init(&game.rsrc, G_PLAYER_RESOURCE_START, game.level.ore_count);
  game.undo = g_undo_make();

  while (!game.quit) {
    static F64 prev_ticks = os_get_ticks();
    static F32 acc = 0;
    static F32 frame_time = 1.f/60.f;

    F64 curr_ticks = os_get_ticks();
    F32 delta_time = (F32)(curr_ticks - prev_ticks) / (F32)os_get_ticks_frequency();
    delta_time = Min(delta_time, frame_time);
    prev_ticks = curr_ticks;

    acc += delta_time;
    while (acc >= frame_time) {
      update(frame_time);
      acc -= frame_time;
    }
    render();
  }

  m_context_release(game.mixer);
  g_undo_release(&game.undo);
  g_assets_release(game.assets);
  r_context_release(game.renderer);
  os_window_close(game.window);
  arena_release(game.arena);
}

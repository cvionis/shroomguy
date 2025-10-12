#include "base/base_inc.h"
#include "os/os_inc.h"
#include "font/font_inc.h"
#include "render/render_inc.h"
#include "sound/sound_inc.h"
#include "game/game_inc.h"
#include "editor/editor_inc.h"

#include "base/base_inc.cpp"
#include "os/os_inc.cpp"
#include "font/font_inc.cpp"
#include "render/render_inc.cpp"
#include "sound/sound_inc.cpp"
#include "game/game_inc.cpp"
#include "editor/editor_inc.cpp"

struct AppState {
  B32 quit;
  
  Arena *arena;
  OS_Handle window;
  R_Context *renderer;
  R_Font *font;
  
  G_Input input;
  G_AssetContext *assets;
  G_MapStorage maps;
  
  ED_ChoiceControl choice_ctrl;
  ED_ResizeControl resize_ctrl;
  ED_MapControl map_ctrl;
  
  ED_UndoContext undo;
  
  F32 save_text_a;
  G_TransitionF32 save_text_tr;
  
  B32 ctrl_toggle = 1;
};

function void
update(AppState *app)
{
  OS_Handle window = app->window;
  G_Input *input = &app->input;
  
  static F32 delta_time = 0.0167f; // @Note: Temporary.
  
  OS_EventList *events = os_get_events();
  for (OS_Event *e = events->first; e != 0; e = e->next) {
    if (e->kind == OS_EventKind_WindowClose) {
      app->quit = 1;
    }
  }
  
  g_get_input(window, input, events);
  
  ed_map_ctrl_update(&app->map_ctrl, &app->choice_ctrl, &app->maps, &app->undo, input);
  ed_choice_ctrl_update(&app->choice_ctrl, input);
  ed_resize_ctrl_update(&app->resize_ctrl, &app->map_ctrl, &app->maps, &app->undo, input);
  
  //
  // Show/hide ctrls
  //
  
  B32 do_ctrl_transition = 0;
  if (g_key_pressed(input, G_Key_F1)) {
    do_ctrl_transition = 1;
  }
  
  ED_ChoiceControl *choice_ctrl = &app->choice_ctrl;
  ED_ResizeControl *resize_ctrl = &app->resize_ctrl;
  
  B32 transition_inactive = !(resize_ctrl->base_x_transition.active && choice_ctrl->base_x_transition.active);
  
  if (do_ctrl_transition && transition_inactive) {
    static F32 base_x_offset = 400;
    
    switch (app->ctrl_toggle) {
      case 0: {
        g_transition_f32_start(&choice_ctrl->base_x_transition, choice_ctrl->base_x, choice_ctrl->base_x - base_x_offset, 0.123f, G_TransitionCurve_QuinticEaseInOut);
        g_transition_f32_start(&resize_ctrl->base_x_transition, resize_ctrl->base_x, resize_ctrl->base_x - base_x_offset, 0.123f, G_TransitionCurve_QuinticEaseInOut);
        app->ctrl_toggle = 1;
      }break;
      case 1: {
        g_transition_f32_start(&choice_ctrl->base_x_transition, choice_ctrl->base_x, choice_ctrl->base_x + base_x_offset, 0.115f, G_TransitionCurve_ExponentialEaseIn);
        g_transition_f32_start(&resize_ctrl->base_x_transition, resize_ctrl->base_x, resize_ctrl->base_x + base_x_offset, 0.115f, G_TransitionCurve_ExponentialEaseIn);
        app->ctrl_toggle = 0;
      }break;
    }
  }
  
  ED_MapControl *map_ctrl = &app->map_ctrl;
  G_TileMap *map = ed_map_get(&app->maps, map_ctrl->current_map_idx);
  
  //
  // Undo
  //
  
  if (g_key_pressed(input, G_Key_Z)) {
    ed_undo_pop(&app->undo, map);
  }
  
  //
  // Clear map tiles
  //
  
  if (g_key_pressed(input, G_Key_F8)) {
    ed_undo_push(&app->undo, map);
    MemorySet(map->tokens, G_TileToken_Empty, map->width*map->height*sizeof(G_TileToken));
  }
  
  //
  // Save changes
  //
  
  if (g_key_pressed(input, G_Key_F12)) {
    g_maps_write(&app->maps, ED_DEFAULT_MAPS_PATH);
    
    app->save_text_a = 1;
    g_transition_f32_start(&app->save_text_tr, 1, 0, 0.05f, G_TransitionCurve_QuinticEaseInOut);
  }
  
  //
  // Update transitions
  //
  
  g_transition_f32_update(&choice_ctrl->base_x_transition, &choice_ctrl->base_x, delta_time);
  g_transition_f32_update(&resize_ctrl->base_x_transition, &resize_ctrl->base_x, delta_time);
  g_transition_f32_update(&app->save_text_tr, &app->save_text_a, delta_time);
}

function void
render(AppState *app) 
{
  OS_Handle window = app->window;
  R_Context *renderer = app->renderer;
  R_Font *font = app->font;
  G_AssetContext *assets = app->assets;
  
  r_set_clear_color(v4f32(0,0,0,1));
  r_set_viewport(os_window_client_rect(window));
  r_begin_frame(renderer); 
  
  // @Todo: Why even pass an arena to choice_ctrl_render? Just use a scratch
  // arena inside the function.
  TempArena tmp = arena_temp_begin(app->arena);
  
  ed_choice_ctrl_render(tmp.arena, renderer, font, assets, &app->choice_ctrl);
  ed_map_ctrl_render(renderer, assets, &app->maps, &app->map_ctrl);
  ed_resize_ctrl_render(renderer, &app->resize_ctrl);
  
  //
  // Text and labels
  //
  
  V4F32 text_color = v4f32(0.4f,0.4f,0.4f,1.f);
  
  r_text(renderer, ED_DEFAULT_MAPS_PATH, font, 24, v2f32(16,8), text_color, 0);
  
  String8 current_map_idx_str = str8_pushf(tmp.arena, "Current map: %d", app->map_ctrl.current_map_idx);
  r_text(renderer, current_map_idx_str, font, 20, v2f32(16,32), v4f32_scale(text_color,0.88f), 0);
  
  r_text(renderer, S8("SAVED"), font, 32, v2f32(16, 620), v4f32(0.5f,0.6f,0.5f,app->save_text_a), 0);
  
  arena_temp_end(tmp);
  
  r_flush(renderer);
  r_end_frame(renderer);
}

void 
entry_point(void) 
{
  os_init();
  os_gfx_init();
  r_init(); 
  
  AppState app;
  
  app.window = os_window_open(S8("Editor"), 1280, 720);
  r_equip_window(app.window);
  app.arena = arena_alloc_default();
  app.renderer = r_context_alloc();
  app.font = r_font_ttf_parse_and_bake(app.arena, S8("../data/fonts/Ithaca.ttf"));
  app.assets = g_assets_alloc(S8("../data"));
  
  app.maps = g_maps_read(ED_DEFAULT_MAPS_PATH);
  app.maps.count = G_MAPS_MAX;
  app.undo = ed_undo_make();
  G_TileMap *map = ed_map_get(&app.maps, app.map_ctrl.current_map_idx);
  
  {
    app.map_ctrl.base_x = 400;
    app.map_ctrl.base_y = 120;
    app.map_ctrl.scale = 60;
  }
  
  {
    app.choice_ctrl.scale = 60;
    app.choice_ctrl.base_x = 916;
    app.choice_ctrl.base_y = 40;
    app.choice_ctrl.pad_x = 120;
    app.choice_ctrl.pad_y = 8;
    app.choice_ctrl.column_size = 6;
  }
  
  {
    app.resize_ctrl.base_x = 1060;
    app.resize_ctrl.base_y = 540;
    app.resize_ctrl.square_size = 50;
    app.resize_ctrl.mode = ED_ResizeMode_Append;
  }
  
  while (!app.quit) {
    update(&app);
    render(&app);
  }
  
  ed_undo_release(&app.undo);
  g_assets_release(app.assets);
  r_context_release(app.renderer);
  arena_release(app.arena);
  os_window_close(app.window);
}

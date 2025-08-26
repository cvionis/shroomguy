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

#define ED_DEFAULT_MAPS_PATH S8("../data/maps/default.maps")

struct ED_View {
  V2F32 map_offset;
  F32 scale;
};

struct ED_Context {
  B32 quit;
  
  Arena *arena;
  OS_Handle window;
  R_Context *renderer;
  R_Font *font;
  
  G_Input input;
  G_AssetContext *assets;
  G_MapStorage maps;
  S32 current_map_idx;
  
  S32 hovered_tile_idx;
  S32 selected_tile_idx;
  
  ED_View view;
};

global ED_Context editor;

function void
ed_render_sprite(R_Context *renderer, G_Sprite *sprite, F32 x, F32 y, F32 scale, V4F32 color)
{
  if (sprite) {
    R_Quad quad = {0};
    
    quad.rect = {x, y, x + scale, y + scale};
    quad.uv_rect = sprite->uv_rect;
    
    quad.colors[0] = color;
    quad.colors[1] = color;
    quad.colors[2] = color;
    quad.colors[3] = color;
    
    r_quad(renderer, &quad, sprite->texture);
  }
}

function void
ed_render_tilemap(R_Context *renderer, G_AssetContext *assets, G_TileMap *map, ED_View *view)
{
  S32 map_width = map->width;
  S32 map_height = map->height;
  G_TileToken *tokens = map->tokens;
  S32 token_count = map_width*map_height;
  
  V2F32 pos = view->map_offset;
  F32 scale = view->scale;
  
  F32 spr_x;
  F32 spr_y;
  G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/empty.png"));
  
  // Background tiles
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    spr_x = pos.x + scale * (token_idx % map_width);
    spr_y = pos.y + scale * (token_idx / map_width);
    
    g_render_sprite(renderer, sprite, spr_x, spr_y, scale);
  }
  
  // All other tiles
  for (S32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    sprite = g_assets_get_sprite(assets, S8("sprites/empty.png"));
    
    spr_x = pos.x + scale * (token_idx % map_width);
    spr_y = pos.y + scale * (token_idx / map_width);
    
    G_TileToken token = tokens[token_idx];
    switch (token) {
      case G_TileToken_Dirt: {
        sprite = g_assets_get_sprite(assets, S8("sprites/dirt.png"));
      }break;
      case G_TileToken_StoneT0: {
        sprite = g_assets_get_sprite(assets, S8("sprites/stone_t0.png"));
      }break;
      case G_TileToken_StoneT1: {
        sprite = g_assets_get_sprite(assets, S8("sprites/stone_t1.png"));
      }break;
      case G_TileToken_OreT0: {
        sprite = g_assets_get_sprite(assets, S8("sprites/ore_t0.png"));
      }break;
      case G_TileToken_OreT1: {
        sprite = g_assets_get_sprite(assets, S8("sprites/ore_t1.png"));
      }break;
      case G_TileToken_OreT2: {
        sprite = g_assets_get_sprite(assets, S8("sprites/ore_t2.png"));
      }break;
      case G_TileToken_Bedrock: {
        sprite = g_assets_get_sprite(assets, S8("sprites/bedrock.png"));
      }break;
      case G_TileToken_Player: {
        sprite = g_assets_get_sprite(assets, S8("sprites/player.png"));
      }break;
      case G_TileToken_EnemyGround: {
        sprite = g_assets_get_sprite(assets, S8("sprites/enemy_ground.png"));
      }break;
      case G_TileToken_EnemyAir: {
        sprite = g_assets_get_sprite(assets, S8("sprites/enemy_air.png"));
      }break;
      case G_TileToken_BoulderSmall: {
        sprite = g_assets_get_sprite(assets, S8("sprites/boulder_small.png"));
      }break;
      case G_TileToken_BoulderLarge: {
        sprite = g_assets_get_sprite(assets, S8("sprites/boulder_large.png"));
      }break;
      case G_TileToken_Lava: {
        sprite = g_assets_get_sprite(assets, S8("sprites/lava.png"));
      }break;
    }
    
    // @Todo: Don't access editor via global here
    V4F32 col = v4f32(1,1,1,1);
    if (editor.hovered_tile_idx == token_idx) {
      col = v4f32(0.88f,0.88f,0.88f,1.f);
    }
    if (editor.selected_tile_idx == token_idx) {
      col = v4f32(1.f,2.f,1.f,1.f);
    }
    ed_render_sprite(renderer, sprite, spr_x, spr_y, scale, col);
  }
}

function G_TileMap *
ed_map_get(G_MapStorage *maps, S32 map_idx)
{
  return &maps->maps[map_idx];
}

function void
ed_update(ED_Context *editor)
{
  OS_Handle window = editor->window;
  G_Input *input = &editor->input;
  ED_View *view = &editor->view;
  
  OS_EventList *events = os_get_events();
  for (OS_Event *e = events->first; e != 0; e = e->next) {
    if (e->kind == OS_EventKind_WindowClose) {
      editor->quit = 1;
    }
  }
  
  g_get_input(window, input, events);
  
  // Map selection
  
  if (g_key_pressed(input, G_Key_Right)) {
    editor->current_map_idx += 1;
    editor->current_map_idx = Min(editor->current_map_idx, G_MAPS_MAX-1);
  }
  if (g_key_pressed(input, G_Key_Left)) {
    editor->current_map_idx -= 1;
    editor->current_map_idx = Max(editor->current_map_idx, 0);
  }
  
  // Tile selection 
  
  G_TileMap *map = ed_map_get(&editor->maps, editor->current_map_idx);
  S32 tiles_count = map->width * map->height;
  
  S32 hovered_tile_idx = -1;
  
  F32 mx = input->mouse.x;
  F32 my = input->mouse.y;
  
  for (S32 tile_idx = 0; tile_idx < tiles_count; tile_idx += 1) {
    F32 col = tile_idx % map->width;
    F32 row = tile_idx / map->width;
    
    F32 tile_x_min = view->scale * col + view->map_offset.x;
    F32 tile_y_min = view->scale * row + view->map_offset.y;
    F32 tile_x_max = tile_x_min + view->scale;
    F32 tile_y_max = tile_y_min + view->scale;
    
    B32 mouse_in_tile = 
    (mx >= tile_x_min) && (my >= tile_y_min) && (mx < tile_x_max) && (my < tile_y_max);
    if (mouse_in_tile) {
      hovered_tile_idx = tile_idx;
    }
  }
  
  if (hovered_tile_idx >= 0 && hovered_tile_idx < tiles_count) {
    editor->hovered_tile_idx = hovered_tile_idx;
    if (g_mouse_pressed(input, G_MouseButton_Left)) {
      editor->selected_tile_idx = hovered_tile_idx;
    }
  }
}

function void
ed_render(ED_Context *editor) 
{
  OS_Handle window = editor->window;
  R_Context *renderer = editor->renderer;
  R_Font *font = editor->font;
  G_AssetContext *assets = editor->assets;
  
  r_set_clear_color(v4f32(0,0,0,1));
  r_set_viewport(os_window_client_rect(window));
  r_begin_frame(renderer); 
  
  TempArena tmp = arena_temp_begin(editor->arena);
  {
    V4F32 col = v4f32(0.4f,0.4f,0.4f,1.f);
    r_text(renderer, ED_DEFAULT_MAPS_PATH, font, 24, v2f32(16,8), col, 0);
    
    String8 current_map_idx_str = str8_pushf(tmp.arena, "Current map: %d", editor->current_map_idx);
    r_text(renderer, current_map_idx_str, font, 20, v2f32(16,32), v4f32_scale(col,0.88f), 0);
    
    G_TileMap *current_map = ed_map_get(&editor->maps, editor->current_map_idx);
    ed_render_tilemap(renderer, assets, current_map, &editor->view);
  }
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
  
  editor.arena = arena_alloc_default();
  editor.window = os_window_open(S8("Editor"), 1280, 720);
  
  r_equip_window(editor.window);
  editor.renderer = r_context_alloc();
  editor.font = r_font_ttf_parse_and_bake(editor.arena, S8("../data/fonts/Ithaca.ttf"));
  editor.assets = g_assets_alloc(S8("../data"));
  
  editor.maps = g_maps_read(ED_DEFAULT_MAPS_PATH);
  //editor.maps.count = G_MAPS_MAX;
  
  editor.current_map_idx = 0;
  editor.view.map_offset = v2f32(400,120);
  editor.view.scale = 60.f;
  
  while (!editor.quit) {
    ed_update(&editor);
    ed_render(&editor);
  }
  
  g_assets_release(editor.assets);
  r_context_release(editor.renderer);
  os_window_close(editor.window);
  arena_release(editor.arena);
}

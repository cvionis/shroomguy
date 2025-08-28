#include "base/base_inc.h"
#include "os/os_inc.h"
#include "font/font_inc.h"
#include "render/render_inc.h"
#include "sound/sound_inc.h"
#include "async/async_inc.h"
#include "game/game_inc.h"
#include "editor/editor_inc.h"

#include "base/base_inc.cpp"
#include "os/os_inc.cpp"
#include "font/font_inc.cpp"
#include "render/render_inc.cpp"
#include "sound/sound_inc.cpp"
#include "async/async_inc.cpp"
#include "game/game_inc.cpp"
#include "editor/editor_inc.cpp"

#define ED_DEFAULT_MAPS_PATH S8("../data/maps/default.maps")

struct ED_View {
  V2F32 map_offset;
  F32 scale;
};

// @Note: Could just combine update() and render() and do a purely
// immediate-mode style of updating and drawing menus so all the location, spacing,
// sizing etc. information are local to the single place the menu is being both updated
// and drawn.
struct ED_ChoiceMenu {
  F32 base_x;
  F32 base_y;
  F32 pad_x;
  F32 pad_y;
  S32 column_size;
  
  S32 hovered_choice_idx = -1;
  S32 selected_choice_idx = -1;
};

enum ED_ResizeDirection {
  ED_ResizeDirection_None = -1,
  ED_ResizeDirection_Left,
  ED_ResizeDirection_Right,
  ED_ResizeDirection_Top,
  ED_ResizeDirection_Bottom,
  ED_ResizeDirection_COUNT,
};

struct ED_ResizeOption {
  RectF32 rect;
};

enum ED_ResizeMode {
  ED_ResizeMode_Append,
  ED_ResizeMode_Remove,
};

struct ED_ResizeMenu {
  F32 base_x;
  F32 base_y;
  F32 square_size;
  
  ED_ResizeMode mode;
  ED_ResizeOption options[ED_ResizeDirection_COUNT];
  ED_ResizeDirection hovered_direction = ED_ResizeDirection_None;
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
  
  S32 hovered_tile_idx = -1;
  S32 selected_tile_idx = -1;
  
  ED_ChoiceMenu choice_menu;
  ED_ResizeMenu resize_menu;
  ED_View view;
};

global ED_Context editor;

function G_TileMap *
ed_map_get(G_MapStorage *maps, S32 map_idx)
{
  return &maps->maps[map_idx];
}

function U32
ed_codepoint_from_key(G_Key key)
{
  U32 codepoint = 0;
  
  if ((key >= G_Key_0) && (key <= G_Key_9)) {
    U32 delta = (U32)(key - G_Key_0);
    codepoint = '0' + delta;
  }
  else {
    // @Note: Temporary
    switch (key) {
      case G_Key_E: {
        codepoint = 'E';
      }break;
      case G_Key_R: {
        codepoint = 'R';
      }break;
    }
  }
  
  return codepoint;
}

struct ED_ChoiceEntry {
  String8 path;
  G_TileToken token;
  G_Key keybind;
};
#define ED_CHOICES_COUNT 11
global ED_ChoiceEntry ed_choice_table[ED_CHOICES_COUNT] = {
  { S8("sprites/empty.png"), G_TileToken_Empty, G_Key_1 },
  { S8("sprites/dirt.png"), G_TileToken_Dirt, G_Key_2 },
  { S8("sprites/stone_t1.png"), G_TileToken_StoneT1, G_Key_3 },
  { S8("sprites/ore_t2.png"), G_TileToken_OreT2, G_Key_4 },
  { S8("sprites/bedrock.png"), G_TileToken_Bedrock, G_Key_5 },
  { S8("sprites/lava.png"), G_TileToken_Lava, G_Key_6 },
  { S8("sprites/boulder_small.png"), G_TileToken_BoulderSmall, G_Key_7 },
  { S8("sprites/boulder_large.png"), G_TileToken_BoulderLarge, G_Key_8 },
  { S8("sprites/player.png"), G_TileToken_Player, G_Key_9 },
  { S8("sprites/enemy_ground.png"), G_TileToken_EnemyGround, G_Key_E },
  { S8("sprites/enemy_air.png"), G_TileToken_EnemyAir, G_Key_R },
};

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
ed_choice_menu_update(ED_ChoiceMenu *menu, G_Input *input, ED_View *view)
{
  F32 scale = view->scale;
  
  F32 base_x = menu->base_x;
  F32 base_y = menu->base_y;
  F32 pad_x = menu->pad_x;
  F32 pad_y = menu->pad_y;
  S32 column_size = menu->column_size;
  
  F32 mx = input->mouse.x;
  F32 my = input->mouse.y;
  
  S32 hovered_choice_idx = -1;
  
  for (S32 choice_idx = 0; choice_idx < ED_CHOICES_COUNT; choice_idx += 1) {
    ED_ChoiceEntry *choice = &ed_choice_table[choice_idx];
    
    F32 choice_x_min = base_x + pad_x*(choice_idx / column_size);
    F32 choice_y_min = base_y + (scale+pad_y)*(choice_idx % column_size);
    F32 choice_x_max = choice_x_min + scale;
    F32 choice_y_max = choice_y_min + scale;
    
    B32 mouse_in_choice = 
    (mx >= choice_x_min) && (my >= choice_y_min) && (mx < choice_x_max) && (my < choice_y_max);
    if (mouse_in_choice) {
      hovered_choice_idx = choice_idx;
    }
  }
  
  if (hovered_choice_idx >= 0 && hovered_choice_idx < ED_CHOICES_COUNT) {
    menu->hovered_choice_idx = hovered_choice_idx; // @Todo: Why are we doing this here
    if (g_mouse_pressed(input, G_MouseButton_Left)) {
      menu->selected_choice_idx = hovered_choice_idx;
    }
  }
  
  if (g_key_pressed(input, G_Key_1)) {
    menu->selected_choice_idx = 0;
  }
  else if (g_key_pressed(input, G_Key_2)) {
    menu->selected_choice_idx = 1;
  }
  else if (g_key_pressed(input, G_Key_3)) {
    menu->selected_choice_idx = 2;
  }
  else if (g_key_pressed(input, G_Key_4)) {
    menu->selected_choice_idx = 3;
  }
  else if (g_key_pressed(input, G_Key_5)) {
    menu->selected_choice_idx = 4;
  }
  else if (g_key_pressed(input, G_Key_6)) {
    menu->selected_choice_idx = 5;
  }
  else if (g_key_pressed(input, G_Key_7)) {
    menu->selected_choice_idx = 6;
  }
  else if (g_key_pressed(input, G_Key_8)) {
    menu->selected_choice_idx = 7;
  }
  else if (g_key_pressed(input, G_Key_9)) {
    menu->selected_choice_idx = 8;
  }
  else if (g_key_pressed(input, G_Key_E)) {
    menu->selected_choice_idx = 9;
  }
  else if (g_key_pressed(input, G_Key_R)) {
    menu->selected_choice_idx = 10;
  }
}

function void
ed_choice_menu_render(Arena *arena, R_Context *renderer, R_Font *font, G_AssetContext *assets, ED_ChoiceMenu *menu, ED_View *view)
{
  F32 scale = view->scale;
  
  F32 base_x = menu->base_x;
  F32 base_y = menu->base_y;
  F32 pad_x = menu->pad_x;
  F32 pad_y = menu->pad_y;
  S32 column_size = menu->column_size;
  
  V4F32 text_color = v4f32(0.4f,0.4f,0.4f,1.f);
  
  for (S32 choice_idx = 0; choice_idx < ED_CHOICES_COUNT; choice_idx += 1) {
    ED_ChoiceEntry *choice = &ed_choice_table[choice_idx];
    G_Sprite *sprite = g_assets_get_sprite(assets, choice->path);
    
    F32 spr_y = base_y + (scale + pad_y)*(choice_idx % column_size);
    F32 spr_x = base_x + pad_x*(choice_idx / column_size);
    
    V4F32 sprite_color = v4f32(1,1,1,1);
    if (menu->hovered_choice_idx == choice_idx) {
      sprite_color = v4f32(0.88f,0.88f,0.88f,1.f);
    }
    if (menu->selected_choice_idx == choice_idx) {
      sprite_color = v4f32(1.f,2.f,1.f,1.f);
    }
    
    ed_render_sprite(renderer, sprite, spr_x, spr_y, scale, sprite_color);
    
    U32 keybind_codepoint = ed_codepoint_from_key(choice->keybind);
    String8 keybind_str = str8_pushf(arena, "%c", keybind_codepoint);
    
    V2F32 text_pos = v2f32(spr_x + scale*1.4f, spr_y + pad_y*2);
    r_text(renderer, keybind_str, font, 32, text_pos, text_color, 0);
  }
}

function void
ed_resize_menu_update(ED_ResizeMenu *menu, G_Input *input)
{
  F32 mx = input->mouse.x;
  F32 my = input->mouse.y;
  
  ED_ResizeDirection hovered_direction = ED_ResizeDirection_None;
  
  for (S32 option_idx = 0; option_idx < 4; option_idx += 1) {
    RectF32 rect = menu->options[option_idx].rect;
    
    B32 mouse_in_option = (mx >= rect.x0) && (my >= rect.y0) && (mx < rect.x1) && (my < rect.y1);
    if (mouse_in_option) {
      hovered_direction = (ED_ResizeDirection)option_idx;
    }
  }
  
  Arena *scratch = arena_get_scratch(0,0);
  
  menu->hovered_direction = hovered_direction;
  if (g_mouse_pressed(input, G_MouseButton_Left)) {
    // @Todo: Don't reference global context directly.
    G_TileMap *map = ed_map_get(&editor.maps, editor.current_map_idx);
    switch (menu->mode) {
      case ED_ResizeMode_Append: {
        switch (hovered_direction) {
          case ED_ResizeDirection_Left: {
            ed_tilemap_column_append_left(scratch, map);
          }break;
          case ED_ResizeDirection_Right: {
            ed_tilemap_column_append_right(scratch, map);
          }break;
          case ED_ResizeDirection_Top: {
            ed_tilemap_row_append_top(scratch, map);
          }break;
          case ED_ResizeDirection_Bottom: {
            ed_tilemap_row_append_bottom(scratch, map);
          }break;
        }
      }break;
      case ED_ResizeMode_Remove: {
        switch (hovered_direction) {
          case ED_ResizeDirection_Left: {
            ed_tilemap_column_remove_left(scratch, map);
          }break;
          case ED_ResizeDirection_Right: {
            ed_tilemap_column_remove_right(scratch, map);
          }break;
          case ED_ResizeDirection_Top: {
            ed_tilemap_row_remove_top(scratch, map);
          }break;
          case ED_ResizeDirection_Bottom: {
            ed_tilemap_row_remove_bottom(scratch, map);
          }break;
        }
      }break;
    }
    // @Resume
  }
  
  if (g_key_pressed(input, G_Key_Space)) {
    menu->mode = (ED_ResizeMode)!(U32)menu->mode;
  }
}

function void
ed_resize_menu_render(R_Context *renderer, ED_ResizeMenu *menu)
{
  V4F32 square_color = v4f32(0.5f,0.58f,0.5f,1.f);
  if (menu->mode == ED_ResizeMode_Remove) {
    square_color = v4f32(0.58f,0.5f,0.5f,1.f);
  }
  
  R_Quad quad = {0};
  quad.uv_rect = rect_f32(0,0,1,1);
  quad.corner_softness = 0.3f;
  quad.radius = 3.2f;
  
  for (S32 option_idx = 0; option_idx < ED_ResizeDirection_COUNT; option_idx += 1) {
    ED_ResizeOption *option = &menu->options[option_idx];
    
    V4F32 col = square_color;
    if (option_idx == menu->hovered_direction) {
      col = v4f32_sub(col, v4f32(0.1f,0.1f,0.1f,0.f));
    }
    
    quad.colors[0] = col;
    quad.colors[1] = col;
    quad.colors[2] = col;
    quad.colors[3] = col;
    
    quad.rect = option->rect;
    r_quad(renderer, &quad, 0);
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
    ed_render_sprite(renderer, sprite, spr_x, spr_y, scale, col);
  }
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
  F32 mx = input->mouse.x;
  F32 my = input->mouse.y;
  
  //
  // Map selection
  //
  
  if (g_key_pressed(input, G_Key_Right)) {
    editor->current_map_idx += 1;
    editor->current_map_idx = Min(editor->current_map_idx, G_MAPS_MAX-1);
  }
  if (g_key_pressed(input, G_Key_Left)) {
    editor->current_map_idx -= 1;
    editor->current_map_idx = Max(editor->current_map_idx, 0);
  }
  
  //
  // Tile selection in map 
  //
  
  G_TileMap *map = ed_map_get(&editor->maps, editor->current_map_idx);
  S32 tiles_count = map->width * map->height;
  
  editor->hovered_tile_idx = -1;
  editor->selected_tile_idx = -1;
  
  S32 hovered_tile_idx = -1;
  
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
  
  //
  // Tile choice menu interaction
  //
  
  ed_choice_menu_update(&editor->choice_menu, input, &editor->view);
  
  //
  // Tilemap resizing menu interaction
  //
  
  ed_resize_menu_update(&editor->resize_menu, input);
  
  //
  // Update the map according to selected tile in map and choice
  //
  
  // @Note: Temp check; need upper bound too.
  {
    S32 selected_tile_idx = editor->selected_tile_idx;
    S32 selected_choice_idx = editor->choice_menu.selected_choice_idx;
    
    if ((selected_tile_idx >= 0) && (selected_choice_idx >= 0)) {
      ED_ChoiceEntry *choice = &ed_choice_table[selected_choice_idx];
      G_TileMap *map = ed_map_get(&editor->maps, editor->current_map_idx);
      map->tokens[selected_tile_idx] = choice->token;
      
      editor->selected_tile_idx = -1;
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
  
  G_TileMap *current_map = ed_map_get(&editor->maps, editor->current_map_idx);
  ed_render_tilemap(renderer, assets, current_map, &editor->view);
  
  V4F32 text_color = v4f32(0.4f,0.4f,0.4f,1.f);
  F32 scale = editor->view.scale;
  
  //
  // Draw the tile choice elements
  //
  
  ed_choice_menu_render(tmp.arena, renderer, font, assets, &editor->choice_menu, &editor->view);
  
  //
  // Draw the tilemap resizing menu
  //
  
  ed_resize_menu_render(renderer, &editor->resize_menu);
  
  //
  // Draw the current file and map's info
  //
  
  {
    r_text(renderer, ED_DEFAULT_MAPS_PATH, font, 24, v2f32(16,8), text_color, 0);
    String8 current_map_idx_str = str8_pushf(tmp.arena, "Current map: %d", editor->current_map_idx);
    r_text(renderer, current_map_idx_str, font, 20, v2f32(16,32), v4f32_scale(text_color,0.88f), 0);
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
  
  {
    editor.renderer = r_context_alloc();
    editor.font = r_font_ttf_parse_and_bake(editor.arena, S8("../data/fonts/Ithaca.ttf"));
    editor.assets = g_assets_alloc(S8("../data"));
  }
  
  {
    editor.maps = g_maps_read(ED_DEFAULT_MAPS_PATH);
    editor.current_map_idx = 0;
  }
  
  {
    editor.view.map_offset = v2f32(400,120);
    editor.view.scale = 60.f;
  }
  
  {
    editor.choice_menu.base_x = 948;
    editor.choice_menu.base_y = 100;
    editor.choice_menu.pad_x = 120;
    editor.choice_menu.pad_y = 8;
    editor.choice_menu.column_size = 6;
  }
  
  {
    editor.resize_menu.base_x = 120;
    editor.resize_menu.base_y = 300;
    editor.resize_menu.square_size = 64;
    editor.resize_menu.mode = ED_ResizeMode_Append;
    
    F32 base_x = editor.resize_menu.base_x;
    F32 base_y = editor.resize_menu.base_y;
    
    F32 square_x = base_x;
    F32 square_y = base_y;
    F32 square_size = editor.resize_menu.square_size;
    
    ED_ResizeOption *options = editor.resize_menu.options;
    
    options[ED_ResizeDirection_Left].rect = rect_f32(square_x, square_y, square_x+square_size,  square_y+square_size);
    
    square_x = base_x + 2*square_size;
    options[ED_ResizeDirection_Right].rect = rect_f32(square_x, square_y, square_x+square_size, square_y+square_size);
    
    square_x = base_x + square_size;
    square_y = base_y - square_size;
    options[ED_ResizeDirection_Top].rect = rect_f32(square_x, square_y, square_x+square_size, square_y+square_size);
    
    square_y = base_y + square_size;
    options[ED_ResizeDirection_Bottom].rect = rect_f32(square_x, square_y, square_x+square_size,  square_y+square_size);
  }
  
  while (!editor.quit) {
    ed_update(&editor);
    ed_render(&editor);
  }
  
  g_assets_release(editor.assets);
  r_context_release(editor.renderer);
  os_window_close(editor.window);
  arena_release(editor.arena);
}

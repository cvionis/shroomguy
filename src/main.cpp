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
};

global GameState game;

// --------------- --------------- ---------------

function G_Level
g_level_from_tilemap(G_TileMap *map) 
{
  G_Level level = {0};
  
  G_TileToken *tokens = map->tokens;
  U32 map_width = map->width;
  U32 map_height = map->height;
  U32 token_count = map_width * map_height;
  G_Entity *player = &level.player;
  
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    F32 px = (F32)(token_idx % map_width);
    F32 py = (F32)(token_idx / map_width);
    
    G_TileToken t = tokens[token_idx];
    if (t & G_TileToken_Player) {
      player->pos = v2f32(px, py);
    }
  }
  
  level.map = map;
  return level;
}

function void
g_render_sprite(R_Context *renderer, G_Sprite *sprite, F32 x, F32 y, F32 scale)
{
  if (sprite) {
    R_Quad quad = {0};
    
    quad.rect = {x, y, x + scale, y + scale};
    quad.uv_rect = sprite->uv_rect;
    
    V4F32 col = v4f32(1,1,1,1);
    quad.colors[0] = col;
    quad.colors[1] = col;
    quad.colors[2] = col;
    quad.colors[3] = col;
    
    r_quad(renderer, &quad, sprite->texture);
  }
}

function void
g_render_tilemap(R_Context *renderer, G_AssetContext *assets, G_TileMap *map)
{
  U32 map_width = map->width;
  U32 map_height = map->height;
  G_TileToken *tokens = map->tokens;
  U32 token_count = map_width*map_height;
  
  F32 spr_x;
  F32 spr_y;
  F32 scale = 64; // @Note: Temporary
  G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/empty.png"));
  
  // Background tiles
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    spr_x = scale * (token_idx % map_width);
    spr_y = scale * (token_idx / map_width);
    
    g_render_sprite(renderer, sprite, spr_x, spr_y, scale);
  }
  
  // All other tiles
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    sprite = g_assets_get_sprite(assets, S8("sprites/empty.png"));
    
    spr_x = scale * (token_idx % map_width);
    spr_y = scale * (token_idx / map_width);
    
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
      case G_TileToken_Hazard: {
        sprite = g_assets_get_sprite(assets, S8("sprites/hazard.png"));
      }break;
      case G_TileToken_Blocking: {
        sprite = g_assets_get_sprite(assets, S8("sprites/blocking.png"));
      }break;
      case G_TileToken_Enemy: {
        sprite = g_assets_get_sprite(assets, S8("sprites/enemy.png"));
      }break;
    }
    
    g_render_sprite(renderer, sprite, spr_x, spr_y, scale);
  }
}
function void
g_render_level(R_Context *renderer, G_AssetContext *assets, G_Level *level)
{
  F32 scale = 64; // @Note: Temporary
  
  g_render_tilemap(renderer, assets, level->map);
  
  G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/player.png"));
  G_Entity *player = &level->player;
  F32 scaled_spr_x = scale * player->pos.x;
  F32 scaled_spr_y = scale * player->pos.y;
  g_render_sprite(renderer, sprite, scaled_spr_x, scaled_spr_y, scale);
}

struct G_LevelSimResult {
  B32 lose;
};

function void
g_entity_move(G_Entity *entity, G_Level *level, S32 dx, S32 dy)
{
  B32 entity_moving = entity->move.active;
  if (!entity_moving) {
    G_TileMap *map = level->map;
    V2F32 delta = v2f32((F32)dx, (F32)dy);
    
    S32 map_width  = map->width;
    S32 map_height = map->height;
    
    V2F32 ent_pos = entity->pos;
    V2F32 dst_pos = v2f32_add(ent_pos, delta);
    
    F32 biased_dst_x = dst_pos.x + 0.5f;
    F32 biased_dst_y = dst_pos.y + 0.5f;
    
    B32 dst_in_map = (biased_dst_x >= 0 && biased_dst_x < map_width &&
                      biased_dst_y >= 0 && biased_dst_y < map_height);
    if (dst_in_map) {
      S32 ent_idx = (S32)(ent_pos.x + ent_pos.y * map_width + 0.5f);
      S32 dst_idx = ent_idx + (dx + dy * map_width);
      
      G_TileToken src_tile = G_TileToken_Empty; 
      G_TileToken dst_tile = map->tokens[dst_idx];
      G_TileToken ent_tile = map->tokens[ent_idx];
      
      B32 dst_non_blocking = (dst_tile & G_TileToken_Empty);
      if (dst_non_blocking) {
        F32 pos_transition_speed = 5.45f;
        G_TransitionCurve pos_transition_curve = G_TransitionCurve_QuinticEaseInOut;
        
        g_transition_v2f32_start(&entity->move, ent_pos, dst_pos, 
                                 pos_transition_speed, pos_transition_curve);
        map->tokens[ent_idx] = src_tile;
        map->tokens[dst_idx] = ent_tile;
      }
    }
  }
}

function void
g_entity_mine(G_Entity *entity, G_Level *level, S32 dx, S32 dy)
{
  B32 entity_moving = entity->move.active;
  if (!entity_moving) {
    G_TileMap *map = level->map;
    V2F32 delta = v2f32((F32)dx, (F32)dy);
    
    S32 map_width  = map->width;
    S32 map_height = map->height;
    
    V2F32 ent_pos = entity->pos;
    V2F32 dst_pos = v2f32_add(ent_pos, delta);
    
    F32 biased_dst_x = dst_pos.x + 0.5f;
    F32 biased_dst_y = dst_pos.y + 0.5f;
    
    B32 dst_in_map = (biased_dst_x >= 0 && biased_dst_x < map_width &&
                      biased_dst_y >= 0 && biased_dst_y < map_height);
    if (dst_in_map) {
      S32 ent_idx = (S32)(ent_pos.x + ent_pos.y * map_width + 0.5f);
      S32 dst_idx = ent_idx + (dx + dy * map_width);
      
      G_TileToken dst_tile = map->tokens[dst_idx];
      
      if (dst_tile & G_TileToken_Dirt) {
        map->tokens[dst_idx] = G_TileToken_Empty;
      }
      else if (dst_tile & G_TileToken_StoneT0) {
        map->tokens[dst_idx] = G_TileToken_Empty;
      }
      else if (dst_tile & G_TileToken_StoneT1) {
        map->tokens[dst_idx] = (G_TileToken)((U32)map->tokens[dst_idx] >> 1);
      }
      else if (dst_tile & G_TileToken_OreT0) {
        map->tokens[dst_idx] = G_TileToken_Empty;
      }
      else if (dst_tile & G_TileToken_OreT1) {
        map->tokens[dst_idx] = (G_TileToken)((U32)map->tokens[dst_idx] >> 1);
      }
      else if (dst_tile & G_TileToken_OreT2) {
        map->tokens[dst_idx] = (G_TileToken)((U32)map->tokens[dst_idx] >> 1);
      }
    }
  }
}

function G_LevelSimResult
g_simulate_level(G_Level *level, G_UndoContext *undo, G_Input *input, F32 delta_time)
{
  G_LevelSimResult result = {0};
  
  S32 dx = 0;
  S32 dy = 0;
  
  // @Todo: Gravity for entities should probably be done by adding dy instead of 
  // adding special logic in g_entity_move.
  
  // @Note: This only allows movement in one of two axes; don't want diagonal movement. 
  dy +=       (g_key_pressed(input, G_Key_S) || g_key_pressed(input, G_Key_Down));
  dy -=       (g_key_pressed(input, G_Key_W) || g_key_pressed(input, G_Key_Up));
  dx -= !dy * (g_key_pressed(input, G_Key_A) || g_key_pressed(input, G_Key_Left));
  dx += !dy * (g_key_pressed(input, G_Key_D) || g_key_pressed(input, G_Key_Right));
  
  dx = Clamp(dx, -1, 1);
  dy = Clamp(dy, -1, 1);
  
  // @Note: Maybe you can create a function that returns the data calculated in the // beginning of used by both _mine() and _move().
  
  G_TileMap *map = level->map;
  G_Entity *player = &level->player;
  // Player movement and gravity
  if (dx || dy) {
    g_undo_push(undo, map);
    g_entity_mine(player, level, dx, dy);
    dy = 0; // @Note: Temporary. To prevent jumping when mining upwards.
    g_entity_move(player, level, dx, dy);
  }
  g_entity_move(player, level, 0, 1); 
  // Animate player movement
  g_transition_v2f32_update(&player->move, &player->pos, delta_time);
  
  return result;
}

// --------------- --------------- ---------------

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
  
  if (g_key_pressed(&game.input, G_Key_Z)) {
    G_TileMap *map = game.level.map;
    g_undo_pop(&game.undo, map);
    game.level = g_level_from_tilemap(map);
  }
  else if (g_key_pressed(&game.input, G_Key_R)) {
    G_TileMap *map = game.level.map;
    g_undo_restart(&game.undo, map);
    game.level = g_level_from_tilemap(map);
  }
  
  g_simulate_level(&game.level, &game.undo, &game.input, delta_time);
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
  G_TileToken tokens[] = {
    G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
    G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Player,G_TileToken_Empty,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_StoneT1,G_TileToken_OreT2,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
  };
  map.width = 6;
  map.height = 6;
  MemoryCopy(map.tokens, tokens, map.width*map.height*sizeof(G_TileToken));
  game.level.map = &map;
  
  game.level = g_level_from_tilemap(&map);
  
  game.undo = g_undo_make();
  
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
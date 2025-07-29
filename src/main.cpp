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

struct G_MineState {
  S32 v;
  S32 max;
  S32 ore_collected;
};

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

// --------------- --------------- ---------------

function G_Entity
g_player_from_tilemap(G_TileMap *map)
{
  G_Entity player = {0};
  
  G_TileToken *tokens = map->tokens;
  U32 map_width = map->width;
  U32 map_height = map->height;
  U32 token_count = map_width * map_height;
  
  // @Note: This function assumes the level will only have one player.
  // This should be checked when reading a tilemap from disk, not here.
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    F32 px = (F32)(token_idx % map_width);
    F32 py = (F32)(token_idx / map_width);
    
    G_TileToken t = tokens[token_idx];
    if (t & G_TileToken_Player) {
      player.pos = v2f32(px, py);
    }
  }
  
  return player;
}

function U32
g_ground_enemies_from_tilemap(G_Entity *enemies, G_TileMap *map)
{
  G_TileToken *tokens = map->tokens;
  U32 map_width = map->width;
  U32 map_height = map->height;
  U32 token_count = map_width * map_height;
  
  U32 enemies_count = 0;
  
  // @Note: This function assumes the level will have <= G_LEVEL_GROUND_ENEMIES_MAX
  // ground enemies. This should be checked when reading a tilemap from disk, not here.
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    F32 px = (F32)(token_idx % map_width);
    F32 py = (F32)(token_idx / map_width);
    
    G_TileToken t = tokens[token_idx];
    if (t & G_TileToken_GroundEnemy) {
      G_Entity *ent = enemies + enemies_count;
      ent->pos = v2f32(px, py);
      enemies_count += 1;
    }
  }
  
  return enemies_count;
}

function U32
g_ore_count_from_tilemap(G_TileMap *map)
{
  G_TileToken *tokens = map->tokens;
  U32 token_count = map->width * map->height;
  
  U32 ore_count = 0;
  
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    G_TileToken t = tokens[token_idx];
    if (t & G_TileToken_OreT2) {
      ore_count += 1;
    }
  }
  
  return ore_count;
}

function G_Level
g_level_from_tilemap(G_TileMap *map) 
{
  G_Level level = {0};
  
  level.ore_count = g_ore_count_from_tilemap(map);
  level.player = g_player_from_tilemap(map);
  level.ground_enemies_count = g_ground_enemies_from_tilemap(level.ground_enemies, map);
  
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
      case G_TileToken_GroundEnemy: {
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
  B32 lost;
};

struct G_MoveResult {
  G_TileToken dst_token;
};

struct G_MoveTransition {
  F32 speed;
  G_TransitionCurve curve;
};

function G_MoveResult
g_entity_move(G_Entity *entity, G_Level *level, S32 dx, S32 dy, G_MoveTransition move_transition)
{
  G_MoveResult result = {0};
  
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
      
      if (dst_tile & G_TileToken_Empty) {
        g_transition_v2f32_start(&entity->move, ent_pos, dst_pos, 
                                 move_transition.speed, move_transition.curve);
        map->tokens[ent_idx] = src_tile;
        map->tokens[dst_idx] = ent_tile;
      }
      
      result.dst_token = dst_tile;
    }
  }
  
  return result;
}

function void
g_entity_mine(G_Entity *entity, G_Level *level, G_MineState *mine, S32 dx, S32 dy)
{
  B32 entity_moving = entity->move.active;
  B32 entity_can_mine = mine->v != 0;
  
  if (!entity_moving && entity_can_mine) {
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
        mine->v -= 1;
      }
      else if (dst_tile & G_TileToken_StoneT1) {
        map->tokens[dst_idx] = (G_TileToken)((U32)map->tokens[dst_idx] >> 1);
        mine->v -= 1;
      }
      else if (dst_tile & G_TileToken_StoneT0) {
        map->tokens[dst_idx] = G_TileToken_Empty;
        mine->v -= 1;
      }
      else if (dst_tile & G_TileToken_OreT2) {
        map->tokens[dst_idx] = (G_TileToken)((U32)map->tokens[dst_idx] >> 1);
        mine->v -= 1;
      }
      else if (dst_tile & G_TileToken_OreT1) {
        map->tokens[dst_idx] = (G_TileToken)((U32)map->tokens[dst_idx] >> 1);
        mine->v -= 1;
      }
      else if (dst_tile & G_TileToken_OreT0) {
        map->tokens[dst_idx] = G_TileToken_Empty;
        mine->ore_collected += 1;
        mine->v -= 1;
      }
      
      mine->v = Max(mine->v, 0);
    }
  }
}

function G_MoveResult
g_entity_do_gravity(G_Entity *entity, G_Level *level)
{
  S32 gravity_dx = 0;
  S32 gravity_dy = 1;
  G_MoveTransition gravity_tr = { 6.55f, G_TransitionCurve_QuinticEaseInOut };
  G_MoveResult result = g_entity_move(entity, level, gravity_dx, gravity_dy, gravity_tr );
  return result;
}

function G_LevelSimResult
g_simulate_level(G_Level *level, G_UndoContext *undo, G_Input *input, G_MineState *mine, F32 delta_time)
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
  
  // Player mining, movement, gravity, animation
  G_Entity *player = &level->player;
  if (dx || dy) {
    g_undo_push(undo, map);
    
    g_entity_mine(player, level, mine, dx, dy);
    
    G_MoveTransition tr = { 6.26f, G_TransitionCurve_ExponentialEaseIn };
    G_MoveResult move = g_entity_move(player, level, dx, dy, tr);
    if (move.dst_token == G_TileToken_GroundEnemy) {
      result.lost = 1; // @Note: Temporary behavior.
    }
  }
  g_entity_do_gravity(player, level);
  g_transition_v2f32_update(&player->move, &player->pos, delta_time);
  
  // Ground enemies gravity, animation
  U32 ground_enemies_count = level->ground_enemies_count;
  G_Entity *ground_enemies = level->ground_enemies;
  for (U32 enemy_idx = 0; enemy_idx < ground_enemies_count; enemy_idx += 1) {
    G_Entity *enemy = &ground_enemies[enemy_idx];
    
    G_MoveResult move = g_entity_do_gravity(enemy, level);
    if (move.dst_token == G_TileToken_Player) {
      // @Todo: Wait until animation is done.
      //result.lost = 1;
    }
    g_transition_v2f32_update(&enemy->move, &enemy->pos, delta_time);
  }
  
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
  
  // @Todo: Wrap all these operations in level-granularity helper functions
  if (g_key_pressed(&game.input, G_Key_Z)) {
    G_TileMap *map = game.level.map;
    g_undo_pop(&game.undo, map);
    game.level.player = g_player_from_tilemap(map);
    game.level.ground_enemies_count = g_ground_enemies_from_tilemap(game.level.ground_enemies, map);
    
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
    
    F32 picaxe_bar_width = 128;
    F32 picaxe_bar_height = 32;
    
    {
      R_Quad quad = {0};
      quad.rect = rect_f32(500, 40, 500 + picaxe_bar_width, 40 + picaxe_bar_height);
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
      F32 width = picaxe_bar_width * ((F32)game.mine.v/(F32)game.mine.max);
      quad.rect = rect_f32(500, 40, 500 + width, 40 + picaxe_bar_height);
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
      
      r_text(renderer, S8("Picaxe: "), game.font, 32, v2f32(500,0), col, 0);
      
      U32 ore_collected = game.mine.ore_collected;
      U32 ore_count = game.level.ore_count;
      
      String8 ore_progress_str = str8_pushf(tmp.arena, "Ores: %d/%d", ore_collected, ore_count);
      r_text(renderer, ore_progress_str, game.font, 32, v2f32(500, 92), col, 0);
      
      arena_temp_end(tmp);
    }
    
    if (game.lost) {
      r_text(renderer, S8("YOU LOST! Press Z to undo or R to restart."), 
             game.font, 32, v2f32(500, 128), v4f32(0.6f,0.4f,0.4f,1.f), 0);
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
  G_TileToken tokens[] = {
    G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
    G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Empty,G_TileToken_Player,G_TileToken_GroundEnemy,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_StoneT1,G_TileToken_OreT2,G_TileToken_Dirt,
    G_TileToken_Dirt,G_TileToken_Empty,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,G_TileToken_Dirt,
  };
  map.width = 6;
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
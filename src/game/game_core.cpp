//
// Input
//

function void
g_get_input(OS_Handle window, G_Input *input, OS_EventList *events)
{
  for (OS_Event *e = events->first; e != 0; e = e->next) {
    G_Key slot = G_Key_Null;
    G_MouseButton mouse_slot = G_MouseButton_Null;
    
    switch (e->key) {
      case OS_Key_Esc:   { slot = G_Key_Esc;   }break;
      case OS_Key_Space: { slot = G_Key_Space; }break;
      case OS_Key_Enter: { slot = G_Key_Enter; }break;
      case OS_Key_Up:    { slot = G_Key_Up;    }break;
      case OS_Key_Down:  { slot = G_Key_Down;  }break;
      case OS_Key_Left:  { slot = G_Key_Left;  }break;
      case OS_Key_Right: { slot = G_Key_Right; }break;
      case OS_Key_W:     { slot = G_Key_W;     }break;
      case OS_Key_A:     { slot = G_Key_A;     }break;
      case OS_Key_S:     { slot = G_Key_S;     }break;
      case OS_Key_D:     { slot = G_Key_D;     }break;
      case OS_Key_Q:     { slot = G_Key_Q;     }break;
      case OS_Key_E:     { slot = G_Key_E;     }break;
      case OS_Key_R:     { slot = G_Key_R;     }break;
      case OS_Key_Z:     { slot = G_Key_Z;     }break;
      case OS_Key_0:     { slot = G_Key_0;     }break;
      case OS_Key_1:     { slot = G_Key_1;     }break;
      case OS_Key_2:     { slot = G_Key_2;     }break;
      case OS_Key_3:     { slot = G_Key_3;     }break;
      case OS_Key_4:     { slot = G_Key_4;     }break;
      case OS_Key_5:     { slot = G_Key_5;     }break;
      case OS_Key_6:     { slot = G_Key_6;     }break;
      case OS_Key_7:     { slot = G_Key_7;     }break;
      case OS_Key_8:     { slot = G_Key_8;     }break;
      case OS_Key_9:     { slot = G_Key_9;     }break;
      case OS_Key_F1:    { slot = G_Key_F1;    }break;
      case OS_Key_F2:    { slot = G_Key_F2;    }break;
      case OS_Key_F3:    { slot = G_Key_F3;    }break;
      case OS_Key_F4:    { slot = G_Key_F4;    }break;
      case OS_Key_F5:    { slot = G_Key_F5;    }break;
      case OS_Key_F6:    { slot = G_Key_F6;    }break;
      case OS_Key_F7:    { slot = G_Key_F7;    }break;
      case OS_Key_F8:    { slot = G_Key_F8;    }break;
      case OS_Key_F9:    { slot = G_Key_F9;    }break;
      case OS_Key_F10:   { slot = G_Key_F10;   }break;
      case OS_Key_F11:   { slot = G_Key_F11;   }break;
      case OS_Key_F12:   { slot = G_Key_F12;   }break;
      case OS_Key_Minus: { slot = G_Key_Minus; }break;
      
      case OS_Key_MouseLeft:   { mouse_slot = G_MouseButton_Left;   }break;
      case OS_Key_MouseMiddle: { mouse_slot = G_MouseButton_Middle; }break;
      case OS_Key_MouseRight:  { mouse_slot = G_MouseButton_Right;  }break;
    }
    
    switch (e->kind) {
      case OS_EventKind_KeyPress:   { input->keys[slot] = 1; }break;
      case OS_EventKind_KeyRelease: { input->keys[slot] = 0; }break;
      
      case OS_EventKind_MousePress:   { input->mouse.buttons[mouse_slot] = 1; }break;
      case OS_EventKind_MouseRelease: { input->mouse.buttons[mouse_slot] = 0; }break;
    }
  }
  
  V2S32 mouse_pos = os_window_cursor_pos(window);
  input->mouse.x = mouse_pos.x;
  input->mouse.y = mouse_pos.y;
}

function B32
g_key_pressed(G_Input *input, G_Key key)
{
  B32 result = input->keys[key];
  input->keys[key] = 0;
  return result;
}

function B32
g_key_down(G_Input *input, G_Key key)
{
  return input->keys[key];
}

function B32 
g_mouse_pressed(G_Input *input, G_MouseButton btn)
{
  B32 result = input->mouse.buttons[btn];
  input->mouse.buttons[btn] = 0;
  return result;
}

function B32 
g_mouse_down(G_Input *input, G_MouseButton btn)
{
  return input->mouse.buttons[btn];
}

//
// Transitions
//

function void
g_transition_f32_start(G_TransitionF32 *tr, F32 src, F32 dst, F32 speed, G_TransitionCurve curve)
{
  tr->src = src;
  tr->dst = dst;
  tr->speed = speed;
  tr->curve = curve; 
  
  tr->active = 1;
  tr->t = 0;
}

function B32
g_transition_f32_update(G_TransitionF32 *tr, F32 *out, F32 delta_time)
{
  B32 finished = 0;
  
  tr->t += tr->speed * delta_time;
  
  if (tr->active) {
    F32 clamped_t = Min(tr->t, 1);
    if (clamped_t >= 1) {
      tr->active = 0;
      finished = 1;
    }
    
    F32 eased_t = clamped_t;
    switch (tr->curve) {
      case G_TransitionCurve_ExponentialEaseIn: {
        eased_t = ease_expo_in(clamped_t);
      }break;
      case G_TransitionCurve_QuinticEaseInOut: {
        eased_t = ease_quintic_inout(clamped_t);
      }break;
    }
    
    *out = lerpf32(tr->src, tr->dst, eased_t);
  }
  
  return finished;
}

function void
g_transition_v2f32_start(G_TransitionV2F32 *tr, V2F32 src, V2F32 dst, F32 speed, G_TransitionCurve curve)
{
  tr->src = src;
  tr->dst = dst;
  tr->speed = speed;
  tr->curve = curve;
  
  tr->active = 1;
  tr->t = 0;
}

// @Todo: Should probably set output to exact destination value when finished
// to avoid fp precision issues
function B32
g_transition_v2f32_update(G_TransitionV2F32 *tr, V2F32 *out, F32 delta_time)
{
  B32 finished = 0;
  
  tr->t += tr->speed * delta_time;
  
  if (tr->active) {
    F32 clamped_t = Min(tr->t, 1);
    if (clamped_t >= 1) {
      tr->active = 0;
      finished = 1;
      *out = tr->dst;
    }
    
    if (!finished) {
      F32 eased_t = clamped_t;
      switch (tr->curve) {
        case G_TransitionCurve_ExponentialEaseIn: {
          eased_t = ease_expo_in(clamped_t);
        }break;
        case G_TransitionCurve_QuinticEaseInOut: {
          eased_t = ease_quintic_inout(clamped_t);
        }break;
      }
      
      *out = v2f32_lerp(tr->src, tr->dst, eased_t);
    }
  }
  
  return finished;
}

//
// Undo System
//

function G_UndoContext 
g_undo_make(void)
{
  G_UndoContext undo = {0};
  
  Arena *arena = arena_alloc_default();
  undo.arena = arena;
  
  return undo;
}

function void
g_undo_release(G_UndoContext *undo)
{
  arena_release(undo->arena);
}

function void 
g_undo_clear(G_UndoContext *undo)
{
  arena_clear(undo->arena);
  undo->count = 0;
}

function void 
g_undo_push(G_UndoContext *undo, G_TileMap *map)
{
  G_UndoNode *n = undo->free;
  SLLStackPopN(undo->free, free_next);
  if (!n) {
    n = ArenaPushStruct(undo->arena, G_UndoNode);
  }
  
  U32 tiles_size = map->width * map->height * sizeof(G_TileToken);
  MemoryCopy(n->tokens, map->tokens, tiles_size);
  
  SLLQueuePushFrontN(undo->first, undo->last, n, stack_next);
  undo->count += 1;
}

function void
g_undo_pop(G_UndoContext *undo, G_TileMap *map)
{
  G_UndoNode *n = undo->first;
  if (n) {
    if (map) {
      U32 tiles_size = map->width * map->height * sizeof(G_TileToken);
      MemoryCopy(map->tokens, n->tokens, tiles_size);
      
      SLLQueuePopN(undo->first, undo->last, stack_next);
      SLLStackPushN(undo->free, n, free_next);
      undo->count -= 1;
    }
  }
}

function void
g_undo_restart(G_UndoContext *undo, G_TileMap *map)
{
  G_UndoNode *n = undo->last;
  if (n) {
    if (map) {
      U32 tiles_size = map->width * map->height * sizeof(G_TileToken);
      MemoryCopy(map->tokens, n->tokens, tiles_size);
      
      undo->count = 0;
    }
  }
}

//
// Tilemaps
//

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
      MemoryZeroStruct(&player.move);
      player.pos = v2f32(px, py);
      player.health = 1;
      player.tilemap_idx = token_idx;
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
  
  // @Note: This function assumes the level will have <= G_LEVEL__ENEMIES_MAX
  // ground enemies. This should be checked when reading a tilemap from disk, not here.
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    F32 px = (F32)(token_idx % map_width);
    F32 py = (F32)(token_idx / map_width);
    
    G_TileToken t = tokens[token_idx];
    if (t & G_TileToken_EnemyGround) {
      G_Entity *ent = enemies + enemies_count;
      MemoryZeroStruct(&ent->move);
      ent->pos = v2f32(px, py);
      ent->health = G_GROUND_ENEMY_HEALTH_MAX;
      ent->tilemap_idx = token_idx;
      enemies_count += 1;
    }
  }
  
  return enemies_count;
}

function U32
g_air_enemies_from_tilemap(G_Entity *enemies, G_TileMap *map)
{
  G_TileToken *tokens = map->tokens;
  U32 map_width = map->width;
  U32 map_height = map->height;
  U32 token_count = map_width * map_height;
  
  U32 enemies_count = 0;
  
  // @Note: This function assumes the level will have <= G_LEVEL__ENEMIES_MAX
  // air enemies. This should be checked when reading a tilemap from disk, not here.
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    F32 px = (F32)(token_idx % map_width);
    F32 py = (F32)(token_idx / map_width);
    
    G_TileToken t = tokens[token_idx];
    if (t & G_TileToken_EnemyAir) {
      G_Entity *ent = enemies + enemies_count;
      MemoryZeroStruct(&ent->move);
      ent->pos = v2f32(px, py);
      ent->health = G_AIR_ENEMY_HEALTH_MAX;
      ent->tilemap_idx = token_idx;
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

function G_LevelBoulderInfo
g_boulders_from_tilemap(G_Entity *small_boulders, G_Entity *large_boulders, G_TileMap *map)
{
  G_LevelBoulderInfo result = {0};
  
  G_TileToken *tokens = map->tokens;
  U32 map_width = map->width;
  U32 map_height = map->height;
  U32 token_count = map_width * map_height;
  
  U32 small_count = 0;
  U32 large_count = 0;
  
  // @Note: This function assumes the level will have <= G_LEVEL_BOULDERS_MAX
  // count of small and large boulders. This should be checked when reading a tilemap
  // from disk, not here.
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    F32 px = (F32)(token_idx % map_width);
    F32 py = (F32)(token_idx / map_width);
    
    G_TileToken t = tokens[token_idx];
    if (t & G_TileToken_BoulderSmall) {
      G_Entity *ent = small_boulders + small_count;
      MemoryZeroStruct(&ent->move);
      ent->pos = v2f32(px, py);
      ent->tilemap_idx = token_idx;
      small_count += 1;
    }
    else if (t & G_TileToken_BoulderLarge) {
      G_Entity *ent = large_boulders + large_count;
      MemoryZeroStruct(&ent->move);
      ent->pos = v2f32(px, py);
      ent->tilemap_idx = token_idx;
      large_count += 1;
    }
  }
  
  result.small_count = small_count;
  result.large_count = large_count;
  return result;
}

function U32
g_rsrc_tiles_from_tilemap(G_Entity *rsrc_tiles, G_TileMap *map)
{
  G_TileToken *tokens = map->tokens;
  U32 map_width = map->width;
  U32 map_height = map->height;
  U32 token_count = map_width * map_height;
  
  U32 rsrc_tiles_count = 0;
  
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    F32 px = (F32)(token_idx % map_width);
    F32 py = (F32)(token_idx / map_width);
    
    G_TileToken t = tokens[token_idx];
    if (t & G_TileToken_Resource) {
      G_Entity *ent = rsrc_tiles + rsrc_tiles_count;
      MemoryZeroStruct(&ent->move);
      ent->pos = v2f32(px, py);
      ent->health = 1;
      ent->tilemap_idx = token_idx;
      rsrc_tiles_count += 1;
    }
  }
  
  return rsrc_tiles_count;
}

// @Todo: Is there a good reason we don't set the level's 'map' member in here?
function G_Level
g_level_from_tilemap(G_TileMap *map) 
{
  G_Level level = {0};
  
  level.ore_count = g_ore_count_from_tilemap(map);
  level.player = g_player_from_tilemap(map);
  level.ground_enemies_count = g_ground_enemies_from_tilemap(level.ground_enemies, map);
  level.air_enemies_count = g_air_enemies_from_tilemap(level.air_enemies, map);
  level.rsrc_tiles_count = g_rsrc_tiles_from_tilemap(level.rsrc_tiles, map);
  
  G_LevelBoulderInfo boulders = g_boulders_from_tilemap(level.small_boulders, level.large_boulders, map);
  level.small_boulders_count = boulders.small_count;
  level.large_boulders_count = boulders.large_count;
  
  return level;
}

//
// Entities
//

function G_EntityMoveResult
g_entity_move(G_Entity *entity, G_Level *level, S32 dx, S32 dy, G_EntityMoveTransition move_transition)
{
  G_EntityMoveResult result = {0};
  
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
      
      B32 should_move = 0;
      
      if (dst_tile & G_TileToken_Empty) {
        should_move = 1;
      }
      if (dst_tile & G_TileToken_BoulderSmall) {
        G_Entity *ent = g_small_boulder_from_tilemap_idx(level, dst_idx);
        should_move = g_entity_move(ent, level, dx, dy, move_transition).moved;
      }
      
      // @Todo: I kind of just sharted this out. Clean up the logic a bit.
      B32 to_ladder = 0;
      B32 from_ladder = 0;
      if (ent_tile & G_TileToken_Player) {
        if (ent_tile & G_TileToken_Ladder) {
          from_ladder = 1;
          //should_move = 1;
        }
        if (dst_tile & G_TileToken_Ladder) {
          to_ladder = 1;
          should_move = 1;
          // Can't jump up to a ladder from a tile below the ladder
          if (!(ent_tile & G_TileToken_Ladder)) {
            if (dy < 0) {
              to_ladder = 0;
              should_move = 0;
            }
          }
        }
      }
      else if (ent_tile & G_TileToken_BoulderSmall) {
        if (dst_tile & G_TileToken_EnemyGround) {
          G_Entity *ent = g_ground_enemy_from_tilemap_idx(level, dst_idx);
          should_move = g_entity_move(ent, level, dx, dy, move_transition).moved;
        }
        else if (dst_tile & G_TileToken_EnemyAir) {
          G_Entity *ent = g_air_enemy_from_tilemap_idx(level, dst_idx);
          should_move = g_entity_move(ent, level, dx, dy, move_transition).moved;
        }
      }
      
      if (should_move) {
        g_transition_v2f32_start(&entity->move, ent_pos, dst_pos, 
                                 move_transition.speed, move_transition.curve);
        if (to_ladder) {
          if (from_ladder) {
            map->tokens[ent_idx] = G_TileToken_Ladder;
          }
          else {
            map->tokens[ent_idx] = src_tile;
          }
          map->tokens[dst_idx] = (G_TileToken)((U32)ent_tile | (U32)G_TileToken_Ladder);
        }
        else {
          if (from_ladder) {
            map->tokens[ent_idx] = G_TileToken_Ladder;
            map->tokens[dst_idx] = G_TileToken_Player;
          }
          else {
            map->tokens[ent_idx] = src_tile;
            map->tokens[dst_idx] = ent_tile;
          }
          entity->tilemap_idx = dst_idx;
        }
      }
      
      result.moved = should_move; 
      result.dst_token = dst_tile;
      result.dst_idx = dst_idx;
    }
  }
  
  return result;
}

function G_EntityCrushResult
g_entity_crush(G_Entity *entity, G_Level *level)
{
  G_EntityCrushResult result = {0};
  
  G_TileMap *map = level->map;
  S32 dx = 0;
  S32 dy = 1;
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
    
    B32 can_crush = 1;
    
    switch (dst_tile) {
      case G_TileToken_EnemyGround: {
        G_Entity *ent = g_ground_enemy_from_tilemap_idx(level, dst_idx);
        ent->health = 0;
      }break;
      case G_TileToken_BoulderSmall: {
        can_crush = 0;
      }break;
      case G_TileToken_BoulderLarge: {
        can_crush = 0;
      }break;
    }
    
    if (dst_tile & G_TileToken_Player) {
      level->player.health = 0;
    }
    
    if (can_crush) {
      F32 tr_speed = 2.25f;
      G_TransitionCurve tr_curve = G_TransitionCurve_QuinticEaseInOut;
      g_transition_v2f32_start(&entity->move, ent_pos, dst_pos, 
                               tr_speed, tr_curve);
      map->tokens[ent_idx] = src_tile;
      map->tokens[dst_idx] = ent_tile;
      entity->tilemap_idx = dst_idx;
      
      result.crushed_token = dst_tile;
    }
  }
  
  return result;
}

function void
g_entity_sink(G_Entity *entity, G_Level *level)
{
  B32 entity_moving = entity->move.active;
  if (!entity_moving) {
    G_TileMap *map = level->map;
    
    V2F32 delta = v2f32(0, 1);
    
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
      
      G_TileToken src_tile = G_TileToken_Empty; 
      G_TileToken ent_tile = map->tokens[ent_idx];
      
      // @Todo: This is garbage
      U32 dst_idx = ent_idx;
      G_TileToken dst_tile = map->tokens[dst_idx];
      S32 drop_count = 0;
      for (;;) {
        dst_idx += map_width;
        dst_tile = map->tokens[dst_idx];
        if (dst_tile != G_TileToken_Lava) {
          break;
        }
        drop_count += 1;
        dst_pos.y += 1;
      }
      
      // @Todo: Decrement drop count?
      dst_idx -= map_width;
      dst_tile = map->tokens[dst_idx];
      dst_pos.y -= 1;
      
      F32 tr_speed = 6.25f;
      if (ent_tile == G_TileToken_Player) {
        tr_speed = 1.2f;
      }
      G_TransitionCurve tr_curve = G_TransitionCurve_QuinticEaseInOut;
      
      if (drop_count > 0) {
        // @Todo: Should entity health be set to 0 when sinking?
        g_transition_v2f32_start(&entity->move, ent_pos, dst_pos, 
                                 tr_speed, tr_curve);
        
        map->tokens[ent_idx] = src_tile;
        map->tokens[dst_idx] = (G_TileToken)((U32)map->tokens[dst_idx] | (U32)ent_tile);
        entity->tilemap_idx = dst_idx;
      }
    }
  }
}

function G_EntityGravityResult
g_entity_apply_gravity(G_Entity *entity, G_Level *level)
{
  G_EntityGravityResult result = {0};
  
  B32 entity_moving = entity->move.active;
  if (!entity_moving) {
    G_TileMap *map = level->map;
    
    V2F32 delta = v2f32(0, 1);
    
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
      G_TileToken src_tile = G_TileToken_Empty; 
      G_TileToken ent_tile = map->tokens[ent_idx];
      
      // @Todo: This is garbage
      U32 dst_idx = ent_idx;
      G_TileToken dst_tile = map->tokens[dst_idx];
      S32 drop_count = 0;
      for (;;) {
        dst_idx += map_width;
        dst_tile = map->tokens[dst_idx];
        if (dst_tile != G_TileToken_Empty) {
          break;
        }
        drop_count += 1;
        dst_pos.y += 1;
      }
      
      // @Todo: Decrement drop count?
      dst_idx -= map_width;
      dst_tile = map->tokens[dst_idx];
      dst_pos.y -= 1;
      
      U32 landed_on_idx = dst_idx + map_width; // @Note: Probably need to clamp.
      G_TileToken landed_on_tile = map->tokens[landed_on_idx];
      
      F32 tr_speed = 4.25f;
      G_TransitionCurve tr_curve = G_TransitionCurve_QuinticEaseInOut;
      
      if (drop_count > 0) {
        g_transition_v2f32_start(&entity->move, ent_pos, dst_pos, 
                                 tr_speed, tr_curve);
        
        map->tokens[ent_idx] = src_tile;
        map->tokens[dst_idx] = ent_tile;
        entity->tilemap_idx = dst_idx;
      }
      
      result.drop_count = drop_count;
      result.ent_idx = ent_idx;
      result.dst_idx = dst_idx;
      result.landed_on_idx = landed_on_idx;
      result.dst_token = dst_tile;
      result.landed_on_token = landed_on_tile;
    }
  }
  
  return result;
}

function G_EntityMineResult
g_entity_mine(G_Entity *entity, G_Level *level, G_MineState *mine, S32 dx, S32 dy)
{
  G_EntityMineResult result = {0};
  
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
      
      
      G_TileToken ent_tile = map->tokens[ent_idx];
      G_TileToken dst_tile = map->tokens[dst_idx];
      
      B32 mined = 0;
      
      if (dst_tile & G_TileToken_Dirt) {
        map->tokens[dst_idx] = G_TileToken_Empty;
        mine->v -= 1;
        mined = 1;
      }
      else if (dst_tile & G_TileToken_StoneT1) {
        map->tokens[dst_idx] = (G_TileToken)((U32)map->tokens[dst_idx] >> 1);
        mine->v -= 1;
        mined = 1;
      }
      else if (dst_tile & G_TileToken_StoneT0) {
        map->tokens[dst_idx] = G_TileToken_Empty;
        mine->v -= 1;
        mined = 1;
      }
      else if (dst_tile & G_TileToken_OreT2) {
        map->tokens[dst_idx] = (G_TileToken)((U32)map->tokens[dst_idx] >> 1);
        mine->v -= 1;
        mined = 1;
      }
      else if (dst_tile & G_TileToken_OreT1) {
        map->tokens[dst_idx] = (G_TileToken)((U32)map->tokens[dst_idx] >> 1);
        mine->v -= 1;
        mined = 1;
      }
      else if (dst_tile & G_TileToken_OreT0) {
        map->tokens[dst_idx] = G_TileToken_Empty;
        g_mine_collect(mine);
        mined = 1;
      }
      
      if (ent_tile & G_TileToken_Player) {
        G_Entity *dst_ent = 0;
        if (dst_tile & G_TileToken_EnemyGround) {
          dst_ent = g_ground_enemy_from_tilemap_idx(level, dst_idx);
          mine->v = Max(0, mine->v - 1);
        }
        else if (dst_tile & G_TileToken_EnemyAir) {
          dst_ent = g_air_enemy_from_tilemap_idx(level, dst_idx);
          mine->v = Max(0, mine->v - 1);
        }
        else if (dst_tile & G_TileToken_Resource) {
          dst_ent = g_rsrc_tile_from_tilemap_idx(level, dst_idx);
          mine->v = Min(G_MINE_MAX, mine->v + 1);
        }
        
        if (dst_ent) {
          if (mine->v > 0) {
            dst_ent->health -= 1;
            if (dst_ent->health <= 0) {
              mined = 1;
              map->tokens[dst_idx] = G_TileToken_Empty;
            }
          }
        }
      }
      
      result.mined = mined;
      result.dst_idx = dst_idx;
      result.dst_token = dst_tile;
    }
  }
  
  return result;
}

//
// Levels
//

function void
g_level_refresh_entities(G_Level *level)
{
  G_TileMap *map = level->map;
  
  level->player = g_player_from_tilemap(map);
  level->ground_enemies_count = g_ground_enemies_from_tilemap(level->ground_enemies, map);
  level->air_enemies_count = g_air_enemies_from_tilemap(level->air_enemies, map);
  level->rsrc_tiles_count = g_rsrc_tiles_from_tilemap(level->rsrc_tiles, map);
  
  G_LevelBoulderInfo boulders = g_boulders_from_tilemap(level->small_boulders, level->large_boulders, map);
  level->small_boulders_count = boulders.small_count;
  level->large_boulders_count = boulders.large_count;
}

function G_LevelSimResult
g_simulate_level(G_Level *level, G_UndoContext *undo, G_Input *input, G_MineState *mine, F32 delta_time)
{
  G_LevelSimResult result = {0};
  
  S32 dx = 0;
  S32 dy = 0;
  
  dy +=       (g_key_pressed(input, G_Key_S) || g_key_pressed(input, G_Key_Down));
  dy -=       (g_key_pressed(input, G_Key_W) || g_key_pressed(input, G_Key_Up));
  dx -= !dy * (g_key_pressed(input, G_Key_A) || g_key_pressed(input, G_Key_Left));
  dx += !dy * (g_key_pressed(input, G_Key_D) || g_key_pressed(input, G_Key_Right));
  
  dx = Clamp(dx, -1, 1);
  dy = Clamp(dy, -1, 1);
  
  // @Note: Maybe you can create a function that returns the data calculated in the beginning of used by both _mine() and _move(), that can then be passed to both.
  
  G_TileMap *map = level->map;
  G_Entity *player = &level->player;
  
  static B32 landed_on_lava = 0;
  G_EntityGravityResult gravity = g_entity_apply_gravity(player, level);
  if (gravity.landed_on_token == G_TileToken_Lava) {
    g_entity_sink(player, level);
    landed_on_lava = 1;
  }
  B32 landed_on_lava_done = g_transition_v2f32_update(&player->move, &player->pos, delta_time);
  
  // Death by lava! AAAA!
  if (landed_on_lava && landed_on_lava_done) {
    B32 player_on_last_row = (player->pos.y >= level->map->height-1);
    if (player_on_last_row) {
      player->health = 0;
      result.lost = 1;
      landed_on_lava = 0;
    }
  }
  
  U32 player_tile_idx = player->tilemap_idx;
  U32 below_tile_idx = Clamp(player_tile_idx+map->width, 0, map->width*map->height) ;
  G_TileToken below_tile = map->tokens[below_tile_idx];
  B32 below_tile_is_enemy = 
  (below_tile == G_TileToken_EnemyAir) || (below_tile == G_TileToken_EnemyGround);
  
  if (!player->move.active && below_tile_is_enemy) {
    result.lost = 1;
  }
  
  // Small boulders
  U32 small_boulders_count = level->small_boulders_count;
  G_Entity *small_boulders = level->small_boulders;
  for (U32 boulder_idx = 0; boulder_idx < small_boulders_count; boulder_idx += 1) {
    G_Entity *boulder = &small_boulders[boulder_idx];
    
    G_EntityGravityResult gravity = g_entity_apply_gravity(boulder, level);
    if (gravity.landed_on_token == G_TileToken_Lava) {
      g_entity_sink(boulder, level);
    }
    U32 landed_on_idx = gravity.landed_on_idx;
    G_TileToken landed_on_token = gravity.landed_on_token;
    
    B32 landed_on_enemy = landed_on_token & G_TileToken_EnemyAir;
    G_Entity *enemy = g_air_enemy_from_tilemap_idx(level, landed_on_idx);
    if (landed_on_enemy && absf32(enemy->pos.y - boulder->pos.y) <= 1.05f) {
      G_Entity *ent = g_air_enemy_from_tilemap_idx(level, landed_on_idx);
      gravity = g_entity_apply_gravity(ent, level);
      if (gravity.landed_on_token == G_TileToken_Lava) {
        g_entity_sink(ent, level);
      }
    }
    
    g_transition_v2f32_update(&boulder->move, &boulder->pos, delta_time);
  }
  
  // Large boulders
  static B32 should_crush = 0;
  U32 large_boulders_count = level->large_boulders_count;
  G_Entity *large_boulders = level->large_boulders;
  for (U32 boulder_idx = 0; boulder_idx < large_boulders_count; boulder_idx += 1) {
    G_Entity *boulder = &large_boulders[boulder_idx];
    
    G_EntityGravityResult gravity = g_entity_apply_gravity(boulder, level);
    if (gravity.landed_on_token == G_TileToken_Lava) {
      g_entity_sink(player, level);
    }
    if (gravity.drop_count >= 1) {
      should_crush = 1;
    }
    // Don't let the player move when a boulder is falling/crushing something;
    // if we do, they can dodge it by moving to an adjecent tile and "walk through"
    // a boulder falling next to them.
    if (boulder->move.active) {
      dx = dy = 0;
    }
    B32 done = g_transition_v2f32_update(&boulder->move, &boulder->pos, delta_time);
    if (done && should_crush) { 
      G_EntityCrushResult crush_result = g_entity_crush(boulder, level);
      G_TileToken crushed = crush_result.crushed_token;
      B32 crushed_ore = 
      (crushed & G_TileToken_OreT2) || 
      (crushed & G_TileToken_OreT1) || 
      (crushed & G_TileToken_OreT0);
      if (crushed_ore) {
        mine->ore_collected += 1;
      }
      should_crush = 0;
    }
    
    B32 boulder_at_player = 
    (absf32(boulder->pos.y-player->pos.y) <= 0.05f) && (absf32(boulder->pos.x-player->pos.x <= 0.05f));
    if ((player->health == 0) && boulder_at_player) {
      result.lost = 1;
    }
  }
  
  // Air enemies
  U32 air_enemies_count = level->air_enemies_count;
  G_Entity *air_enemies = level->air_enemies;
  for (U32 enemy_idx = 0; enemy_idx < air_enemies_count; enemy_idx += 1) {
    G_Entity *enemy = &air_enemies[enemy_idx];
    g_transition_v2f32_update(&enemy->move, &enemy->pos, delta_time);
  }
  
  // Ground enemies
  U32 ground_enemies_count = level->ground_enemies_count;
  G_Entity *ground_enemies = level->ground_enemies;
  for (U32 enemy_idx = 0; enemy_idx < ground_enemies_count; enemy_idx += 1) {
    G_Entity *enemy = &ground_enemies[enemy_idx];
    
    if (enemy->health) {
      G_EntityGravityResult gravity = g_entity_apply_gravity(enemy, level);
      if (gravity.landed_on_token == G_TileToken_Lava) {
        g_entity_sink(enemy, level);
      }
      
      g_transition_v2f32_update(&enemy->move, &enemy->pos, delta_time);
      
      B32 enemy_on_player = absf32(player->pos.y - enemy->pos.y) <= 1.05;
      if ((gravity.landed_on_token & G_TileToken_Player) && enemy_on_player) {
        if (!(gravity.landed_on_token & G_TileToken_Ladder)) {
          result.lost = 1;
          break;
        }
      }
    }
  }
  
  // Resource tiles
  U32 rsrc_tiles_count = level->rsrc_tiles_count;
  G_Entity *rsrc_tiles = level->rsrc_tiles;
  for (U32 rsrc_tile_idx = 0; rsrc_tile_idx < rsrc_tiles_count; rsrc_tile_idx += 1) {
    G_Entity *rsrc_tile = &rsrc_tiles[rsrc_tile_idx];
    g_entity_apply_gravity(rsrc_tile, level);
    
    g_transition_v2f32_update(&rsrc_tile->move, &rsrc_tile->pos, delta_time);
  }
  
  // Player
  if (dx || dy) {
    g_undo_push(undo, map);
    
    G_EntityMoveTransition tr = { 6.26f, G_TransitionCurve_ExponentialEaseIn };
    
    G_EntityMoveResult move_result = g_entity_move(player, level, dx, dy, tr);
    G_EntityMineResult mine_result = g_entity_mine(player, level, mine, dx, dy);
    
    // Move to the mined tile if there is not an entity sitting on the mined tile.
    U32 above_mined_tile_idx = mine_result.dst_idx - map->width;
    G_Entity *a = g_ground_enemy_from_tilemap_idx(level, above_mined_tile_idx);
    G_Entity *b = g_small_boulder_from_tilemap_idx(level, above_mined_tile_idx);
    G_Entity *c = g_large_boulder_from_tilemap_idx(level, above_mined_tile_idx);
    if (!(a || b || c) && mine_result.mined) {
      g_entity_move(player, level, dx, 0, tr);
    }
  }
  
  return result;
}

function G_Entity *
g_air_enemy_from_tilemap_idx(G_Level *level, U32 idx)
{
  G_Entity *result = 0;
  
  U32 enemies_count = level->air_enemies_count;
  G_Entity *enemies = level->air_enemies;
  
  for (U32 enemy_idx = 0; enemy_idx < enemies_count; enemy_idx += 1) {
    G_Entity *ent = &enemies[enemy_idx];
    if (ent->tilemap_idx == idx) {
      result = ent;
      break;
    }
  }
  
  return result;
}

function G_Entity *
g_ground_enemy_from_tilemap_idx(G_Level *level, U32 idx)
{
  G_Entity *result = 0;
  
  U32 enemies_count = level->ground_enemies_count;
  G_Entity *enemies = level->ground_enemies;
  
  for (U32 enemy_idx = 0; enemy_idx < enemies_count; enemy_idx += 1) {
    G_Entity *ent = &enemies[enemy_idx];
    if (ent->tilemap_idx == idx) {
      result = ent;
      break;
    }
  }
  
  return result;
}

function G_Entity *
g_small_boulder_from_tilemap_idx(G_Level *level, U32 idx)
{
  G_Entity *result = 0;
  
  U32 small_boulders_count = level->small_boulders_count;
  G_Entity *small_boulders = level->small_boulders;
  
  for (U32 boulder_idx = 0; boulder_idx < small_boulders_count; boulder_idx += 1) {
    G_Entity *ent = &small_boulders[boulder_idx];
    if (ent->tilemap_idx == idx) {
      result = ent;
      break;
    }
  }
  
  return result;
}

function G_Entity *
g_rsrc_tile_from_tilemap_idx(G_Level *level, U32 idx)
{
  G_Entity *result = 0;
  
  U32 rsrc_tiles_count = level->rsrc_tiles_count;
  G_Entity *rsrc_tiles = level->rsrc_tiles;
  
  for (U32 rsrc_tile_idx = 0; rsrc_tile_idx < rsrc_tiles_count; rsrc_tile_idx += 1) {
    G_Entity *ent = &rsrc_tiles[rsrc_tile_idx];
    if (ent->tilemap_idx == idx) {
      result = ent;
      break;
    }
  }
  
  return result;
}

function G_Entity *
g_large_boulder_from_tilemap_idx(G_Level *level, U32 idx)
{
  G_Entity *result = 0;
  
  U32 large_boulders_count = level->large_boulders_count;
  G_Entity *large_boulders = level->large_boulders;
  
  for (U32 boulder_idx = 0; boulder_idx < large_boulders_count; boulder_idx += 1) {
    G_Entity *ent = &large_boulders[boulder_idx];
    if (ent->tilemap_idx == idx) {
      result = ent;
      break;
    }
  }
  
  return result;
}

//
// Rendering
//

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
g_render_lava(R_Context *renderer, G_AssetContext *assets, G_TileMap *map)
{
  U32 map_width = map->width;
  U32 map_height = map->height;
  G_TileToken *tokens = map->tokens;
  U32 token_count = map_width*map_height;
  
  F32 spr_x;
  F32 spr_y;
  F32 scale = 64; // @Note: Temporary
  G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/lava.png"));
  
  // All other tiles
  for (U32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    spr_x = scale * (token_idx % map_width);
    spr_y = scale * (token_idx / map_width);
    
    G_TileToken token = tokens[token_idx];
    if (token & G_TileToken_Lava) {
      g_render_sprite(renderer, sprite, spr_x, spr_y, scale);
    }
  }
}

function void
g_render_tilemap(R_Context *renderer, G_AssetContext *assets, G_TileMap *map)
{
  S32 map_width = map->width;
  S32 map_height = map->height;
  G_TileToken *tokens = map->tokens;
  S32 token_count = map_width*map_height;
  
  F32 spr_x;
  F32 spr_y;
  F32 scale = 64; // @Note: Temporary
  G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/empty.png"));
  
  // Background tiles
  for (S32 token_idx = 0; token_idx < token_count; token_idx += 1) {
    spr_x = scale * (token_idx % map_width);
    spr_y = scale * (token_idx / map_width);
    
    g_render_sprite(renderer, sprite, spr_x, spr_y, scale);
  }
  
  // All other tiles
  for (S32 token_idx = 0; token_idx < token_count; token_idx += 1) {
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
      case G_TileToken_Ladder: {
        sprite = g_assets_get_sprite(assets, S8("sprites/ladder.png"));
      }break;
      case G_TileToken_Bedrock: {
        sprite = g_assets_get_sprite(assets, S8("sprites/bedrock.png"));
      }break;
    }
    
    if (token & G_TileToken_Ladder) {
      sprite = g_assets_get_sprite(assets, S8("sprites/ladder.png"));
    }
    
    g_render_sprite(renderer, sprite, spr_x, spr_y, scale);
  }
}

function void
g_render_level(R_Context *renderer, G_AssetContext *assets, G_Level *level)
{
  F32 scale = 64; // @Note: Temporary
  
  g_render_tilemap(renderer, assets, level->map);
  
  {
    G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/player.png"));
    G_Entity *player = &level->player;
    F32 scaled_spr_x = scale * player->pos.x;
    F32 scaled_spr_y = scale * player->pos.y;
    
    g_render_sprite(renderer, sprite, scaled_spr_x, scaled_spr_y, scale);
  }
  
  // Air enemies
  {
    G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/enemy_air.png"));
    
    U32 air_enemies_count = level->air_enemies_count;
    G_Entity *air_enemies = level->air_enemies;
    for (U32 enemy_idx = 0; enemy_idx < air_enemies_count; enemy_idx += 1) {
      G_Entity *enemy = &air_enemies[enemy_idx];
      
      if (enemy->health > 0) { 
        F32 scaled_spr_x = scale * enemy->pos.x;
        F32 scaled_spr_y = scale * enemy->pos.y;
        
        g_render_sprite(renderer, sprite, scaled_spr_x, scaled_spr_y, scale);
      }
    }
  }
  
  // Ground enemies
  {
    G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/enemy_ground.png"));
    
    U32 ground_enemies_count = level->ground_enemies_count;
    G_Entity *ground_enemies = level->ground_enemies;
    for (U32 enemy_idx = 0; enemy_idx < ground_enemies_count; enemy_idx += 1) {
      G_Entity *enemy = &ground_enemies[enemy_idx];
      
      if (enemy->health > 0) { 
        F32 scaled_spr_x = scale * enemy->pos.x;
        F32 scaled_spr_y = scale * enemy->pos.y;
        
        g_render_sprite(renderer, sprite, scaled_spr_x, scaled_spr_y, scale);
      }
    }
  }
  
  // Small boulders
  {
    G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/boulder_small.png"));
    
    U32 small_boulders_count = level->small_boulders_count;
    G_Entity *small_boulders = level->small_boulders;
    
    for (U32 boulder_idx = 0; boulder_idx < small_boulders_count; boulder_idx += 1) {
      G_Entity *boulder = &small_boulders[boulder_idx];
      F32 scaled_spr_x = scale * boulder->pos.x;
      F32 scaled_spr_y = scale * boulder->pos.y;
      g_render_sprite(renderer, sprite, scaled_spr_x, scaled_spr_y, scale);
    }
  }
  
  // Large boulders
  {
    G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/boulder_large.png"));
    
    U32 large_boulders_count = level->large_boulders_count;
    G_Entity *large_boulders = level->large_boulders;
    
    for (U32 boulder_idx = 0; boulder_idx < large_boulders_count; boulder_idx += 1) {
      G_Entity *boulder = &large_boulders[boulder_idx];
      F32 scaled_spr_x = scale * boulder->pos.x;
      F32 scaled_spr_y = scale * boulder->pos.y;
      g_render_sprite(renderer, sprite, scaled_spr_x, scaled_spr_y, scale);
    }
  }
  
  // Resource tiles
  {
    G_Sprite *sprite = g_assets_get_sprite(assets, S8("sprites/resource.png"));
    
    U32 rsrc_tiles_count = level->rsrc_tiles_count;
    G_Entity *rsrc_tiles = level->rsrc_tiles;
    
    for (U32 rsrc_tile_idx = 0; rsrc_tile_idx < rsrc_tiles_count; rsrc_tile_idx += 1) {
      G_Entity *rsrc_tile = &rsrc_tiles[rsrc_tile_idx];
      if (rsrc_tile->health > 0) {
        F32 scaled_spr_x = scale * rsrc_tile->pos.x;
        F32 scaled_spr_y = scale * rsrc_tile->pos.y;
        g_render_sprite(renderer, sprite, scaled_spr_x, scaled_spr_y, scale);
      }
    }
  }
  
  // Lava
  g_render_lava(renderer, assets, level->map);
}

//
// Map storage and serialization
//

/*

@Todo: Pack
Maps file format (temp):

last_played_idx: 4bytes
maps_count: 4bytes

map0_flags: 4bytes
map0_width: 4bytes
map0_height: 4bytes
map0_tokens: 4bytes[G_TILEMAP_TOKENS_MAX]

...

*/

function G_MapStorage 
g_maps_read(String8 path) 
{
  G_MapStorage result = {0};
  
  TempArena scratch = arena_scratch_begin(0,0);
  {
    String8 str = os_file_read(scratch.arena, path);
    if (str.count > 0) {
      U8 *pos = str.data;
      
      MemoryCopy(&result.last_played_map_idx, pos, sizeof(result.last_played_map_idx));
      pos += sizeof(result.last_played_map_idx);
      
      MemoryCopy(&result.count, pos, sizeof(result.count));
      pos += sizeof(result.count);
      
      U32 maps_count = result.count;
      for (U32 map_idx = 0; map_idx < maps_count; map_idx += 1) {
        G_TileMap *map = &result.maps[map_idx];
        
        MemoryCopy(&map->width, pos, sizeof(map->width));
        pos += sizeof(map->width);
        
        MemoryCopy(&map->height, pos, sizeof(map->height));
        pos += sizeof(map->height);
        
        MemoryCopy(&map->flags, pos, sizeof(map->flags));
        pos += sizeof(map->flags);
        
        U32 tokens_size = map->width*map->height*sizeof(G_TileToken);
        MemoryCopy(map->tokens, pos, tokens_size);
        pos += tokens_size;
      }
    }
  }
  arena_scratch_end(scratch);
  
  return result;
}

function void 
g_maps_write(G_MapStorage *maps, String8 path) 
{
  FILE *f = fopen((char *)path.data, "wb");
  if (f) {
    U32 last_played_map_idx = maps->last_played_map_idx;
    fwrite(&last_played_map_idx, sizeof(last_played_map_idx), 1, f);
    
    U32 maps_count = maps->count;
    fwrite(&maps_count, sizeof(maps_count), 1, f);
    
    for (U32 map_idx = 0; map_idx < maps_count; map_idx += 1) {
      G_TileMap *map = &maps->maps[map_idx];
      
      U32 width = map->width;
      U32 height = map->height;
      
      fwrite(&width, sizeof(width), 1, f);
      fwrite(&height, sizeof(height), 1, f);
      fwrite(&map->flags, sizeof(map->flags), 1, f);
      
      U32 tokens_size = width*height*sizeof(G_TileToken);
      fwrite(map->tokens, tokens_size, 1, f);
    }
    
    fclose(f);
  }
}

function G_TileMap 
g_maps_get_copy(G_MapStorage *maps, U32 map_idx) 
{
  G_TileMap result = {0};
  if (map_idx >= 0 && map_idx < G_MAPS_MAX) {
    MemoryCopy(&result, &maps->maps[map_idx], sizeof(G_TileMap));
  }
  return result;
}

//
// Mining state
//

function G_MineState
g_mine_make(S32 max)
{
  G_MineState result = {0};
  result.max = max;
  result.v = max;
  return result;
}

function void 
g_mine_decollect(G_MineState *mine) 
{
  mine->v += 1;
  mine->v = Min(mine->v, mine->max);
  mine->ore_collected -= 1;
  mine->ore_collected = Max(mine->ore_collected, 0);
}

function void
g_mine_collect(G_MineState *mine)
{
  mine->ore_collected += 1;
  mine->v -= 1;
}

function void
g_mine_reset(G_MineState *mine)
{
  mine->v = mine->max;
  mine->ore_collected = 0;
}
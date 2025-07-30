#pragma once

//
// Constants 
//

#define G_TILEMAP_TOKENS_MAX 256
#define G_LEVEL_GROUND_ENEMIES_MAX 32
#define G_LEVEL_BOULDERS_MAX 16

//
// Input
//

enum G_Key {
  G_Key_Null,
  G_Key_Esc,
  G_Key_Space,
  G_Key_Enter,
  G_Key_Up,
  G_Key_Left,
  G_Key_Down,
  G_Key_Right,
  G_Key_W,
  G_Key_A,
  G_Key_S,
  G_Key_D,
  G_Key_Q,
  G_Key_E,
  G_Key_R,
  G_Key_Z,
  G_Key_0,
  G_Key_1,
  G_Key_2,
  G_Key_3,
  G_Key_4,
  G_Key_5,
  G_Key_6,
  G_Key_7,
  G_Key_8,
  G_Key_9,
  G_Key_F1,
  G_Key_F2,
  G_Key_F3,
  G_Key_F4,
  G_Key_F5,
  G_Key_F6,
  G_Key_F7,
  G_Key_F8,
  G_Key_F9,
  G_Key_F10,
  G_Key_F11,
  G_Key_F12,
  G_Key_Minus,
  G_Key_COUNT,
};

enum G_MouseButton {
  G_MouseButton_Null,
  G_MouseButton_Left,
  G_MouseButton_Middle,
  G_MouseButton_Right,
  G_MouseButton_COUNT,
};

struct G_Input {
  B32 keys[G_Key_COUNT];
  struct {
    S32 x;
    S32 y;
    B32 buttons[G_MouseButton_COUNT];
  }mouse;
};

//
// Transitions
//

enum G_TransitionCurve {
  G_TransitionCurve_Linear,
  G_TransitionCurve_ExponentialEaseIn,
  G_TransitionCurve_QuinticEaseInOut,
};

struct G_TransitionF32 {
  F32 src;
  F32 dst;
  
  F32 t;
  B32 active;
  F32 speed;
  
  G_TransitionCurve curve;
};

struct G_TransitionV2F32 {
  V2F32 src;
  V2F32 dst;
  
  F32 t;
  B32 active;
  F32 speed;
  
  G_TransitionCurve curve;
};

//
// Tiles & Tilemaps
//

// @Todo: Stages of stone and ore as they take several actions to mine
enum G_TileToken {
  G_TileToken_Null,
  
  G_TileToken_Empty        = (1 << 0),
  G_TileToken_Dirt         = (1 << 1),
  G_TileToken_StoneT0      = (1 << 2),
  G_TileToken_StoneT1      = (1 << 3),
  G_TileToken_OreT0        = (1 << 4),
  G_TileToken_OreT1        = (1 << 5),
  G_TileToken_OreT2        = (1 << 6),
  G_TileToken_BoulderSmall = (1 << 7),
  G_TileToken_BoulderLarge = (1 << 8),
  G_TileToken_EnemyGround  = (1 << 9),
  G_TileToken_EnemyAir     = (1 << 10),
  G_TileToken_Player       = (1 << 11),
};

enum G_TileMapFlags {
  G_TileMapFlag_Unlocked  = (1 << 0),
  G_TileMapFlag_Completed = (1 << 1),
};

struct G_TileMap {
  U32 width;
  U32 height;
  G_TileToken tokens[G_TILEMAP_TOKENS_MAX];
  G_TileMapFlags flags;
};

//
// Entities
//

struct G_Entity {
  V2F32 pos; // @Note: Unscaled
  G_TransitionV2F32 move;
  U32 tilemap_idx;
  B32 dead;
};

struct G_EntityMoveResult {
  G_TileToken dst_token;
  U32 dst_idx;
};

struct G_EntityMineResult {
  G_TileToken dst_token;
  U32 dst_idx;
};

struct G_EntityGravityResult {
  S32 drop_count;
  U32 ent_idx;
  U32 dst_idx;
  G_TileToken dst_token;
};

struct G_EntityMoveTransition {
  F32 speed;
  G_TransitionCurve curve;
};

//
// Levels
//

struct G_Level {
  G_TileMap *map;
  G_Entity player;
  U32 ore_count;
  
  U32 ground_enemies_count;
  G_Entity ground_enemies[G_LEVEL_GROUND_ENEMIES_MAX];
  
  U32 small_boulders_count;
  G_Entity small_boulders[G_LEVEL_BOULDERS_MAX];
  
  U32 large_boulders_count;
  G_Entity large_boulders[G_LEVEL_BOULDERS_MAX];
};

struct G_LevelSimResult {
  B32 lost;
};

struct G_LevelBoulderInfo {
  U32 small_count;
  U32 large_count;
};

//
// Undo System
//

struct G_UndoNode {
  G_UndoNode *stack_next;
  G_UndoNode *free_next;
  
  G_TileToken tokens[G_TILEMAP_TOKENS_MAX];
};

struct G_UndoContext {
  Arena *arena;
  
  G_UndoNode *first; // Most recent undo state
  G_UndoNode *last;  // Least recent undo state
  U32 count;
  
  G_UndoNode *free;
};

//
// Mining state
//

struct G_MineState {
  S32 v;
  S32 max;
  S32 ore_collected;
};

//
// Misc.
//

// Input
function void g_get_input(OS_Handle window, G_Input *input, OS_EventList *events);
function B32 g_key_pressed(G_Input *input, G_Key key);
function B32 g_key_down(G_Input *input, G_Key key);
function B32 g_mouse_pressed(G_Input *input, G_MouseButton btn);
function B32 g_mouse_down(G_Input *input, G_MouseButton btn);

// Transitions
function void g_transition_f32_start(G_TransitionF32 *tr, F32 src, F32 dst, F32 speed, G_TransitionCurve curve);
function B32 g_transition_f32_update(G_TransitionF32 *tr, F32 *out, F32 delta_time);
function void g_transition_v2f32_start(G_TransitionV2F32 *tr, V2F32 src, V2F32 dst, F32 speed, G_TransitionCurve curve);
function B32 g_transition_v2f32_update(G_TransitionV2F32 *tr, V2F32 *out, F32 delta_time);

// Undo System
function G_UndoContext g_undo_make(void);
function void g_undo_release(G_UndoContext *undo);
function void g_undo_clear(G_UndoContext *undo);

// Tilemaps
function G_Entity g_player_from_tilemap(G_TileMap *map);
function U32 g_ground_enemies_from_tilemap(G_Entity *enemies, G_TileMap *map);
function U32 g_ore_count_from_tilemap(G_TileMap *map);
function G_LevelBoulderInfo g_boulders_from_tilemap(G_Entity *small_boulders, G_Entity *large_boulders, G_TileMap *map);
function G_Level g_level_from_tilemap(G_TileMap *map);

// Entities
function G_EntityMoveResult g_entity_move(G_Entity *entity, G_Level *level, S32 dx, S32 dy, G_EntityMoveTransition move_transition);
function G_EntityMineResult g_entity_mine(G_Entity *entity, G_Level *level, G_MineState *mine, S32 dx, S32 dy);
function void g_entity_crush(G_Entity *entity, G_Level *level);
function G_EntityGravityResult g_entity_apply_gravity(G_Entity *entity, G_Level *level);

// Levels
function G_LevelSimResult g_simulate_level(G_Level *level, G_UndoContext *undo, G_Input *input, G_MineState *mine, F32 delta_time);
function G_Entity *g_small_boulder_from_tilemap_idx(G_Level *level, U32 idx);
function G_Entity *g_large_boulder_from_tilemap_idx(G_Level *level, U32 idx);
function G_Entity *g_ground_enemy_from_tilemap_idx(G_Level *level, U32 idx);

// Rendering
function void g_render_sprite(R_Context *renderer, G_Sprite *sprite, F32 x, F32 y, F32 scale);
function void g_render_tilemap(R_Context *renderer, G_AssetContext *assets, G_TileMap *map);

function void g_render_level(R_Context *renderer, G_AssetContext *assets, G_Level *level);
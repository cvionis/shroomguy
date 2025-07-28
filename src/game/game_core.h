#pragma once

//
// Constants 
//

#define G_TILEMAP_TOKENS_MAX 256

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
// Tiles
//

enum G_TileToken {
  G_TileToken_Empty  = (0 << 0),
  G_TileToken_Dirt   = (1 << 0),
  G_TileToken_Stone  = (1 << 1),
  G_TileToken_Ore    = (1 << 2),
  G_TileToken_Hazard = (1 << 3),
  G_TileToken_Enemy  = (1 << 4),
  G_TileToken_Player = (1 << 5),
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
// Gameplay API
//

// Input
function void g_get_input(OS_Handle window, G_Input *input, OS_EventList *events);
function B32 g_key_pressed(G_Input *input, G_Key key);
function B32 g_key_down(G_Input *input, G_Key key);
function B32 g_mouse_pressed(G_Input *input, G_MouseButton btn);
function B32 g_mouse_down(G_Input *input, G_MouseButton btn);

// Undo System
function G_UndoContext g_undo_make(void);
function void g_undo_release(G_UndoContext *undo);
function void g_undo_clear(G_UndoContext *undo);

// Transitions
function void g_transition_f32_start(G_TransitionF32 *tr, F32 src, F32 dst, F32 speed, G_TransitionCurve curve);
function void g_transition_v2f32_start(G_TransitionV2F32 *tr, V2F32 src, V2F32 dst, F32 speed, G_TransitionCurve curve);
function B32 g_transition_f32_update(G_TransitionF32 *tr, F32 *out, F32 delta_time);
function B32 g_transition_v2f32_update(G_TransitionV2F32 *tr, V2F32 *out, F32 delta_time);
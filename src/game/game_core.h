#pragma once

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

function void g_get_input(OS_Handle window, G_Input *input, OS_EventList *events);
function B32 g_key_pressed(G_Input *input, G_Key key);
function B32 g_key_down(G_Input *input, G_Key key);
function B32 g_mouse_pressed(G_Input *input, G_MouseButton btn);
function B32 g_mouse_down(G_Input *input, G_MouseButton btn);
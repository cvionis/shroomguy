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

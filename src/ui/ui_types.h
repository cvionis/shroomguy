#ifndef UI_TYPES_H
#define UI_TYPES_H

//
// Types and enums used by helper and core UI code
//

struct UI_Key {
  U64 v;
};

enum UI_BoxFlags {
  UI_BoxFlag_Disabled        = (1 << 0), 
  UI_BoxFlag_MouseClickable  = (1 << 1),
  UI_BoxFlag_DrawBorder      = (1 << 2),
  UI_BoxFlag_DrawText        = (1 << 3),
  UI_BoxFlag_DrawBackground  = (1 << 4),
  UI_BoxFlag_DrawDropShadow  = (1 << 5),
  UI_BoxFlag_Clickable       = (1 << 6),
};

enum UI_SizeKind {
  UI_SizeKind_Null,
  UI_SizeKind_Pixels,
  UI_SizeKind_FitText,
  UI_SizeKind_PctOfParent,
  UI_SizeKind_ChildrenSum,
};

enum UI_MouseButtonSlot {
  UI_MouseButtonSlot_Left,
  UI_MouseButtonSlot_Middle,
  UI_MouseButtonSlot_Right,
  UI_MouseButtonSlot_COUNT,
};

enum UI_TextAlignment {
  UI_TextAlignment_Left,
  UI_TextAlignment_Center,
  UI_TextAlignment_Right,
};

struct UI_Size {
  UI_SizeKind kind;
  F32 value;
  F32 strictness;
};

struct UI_Box {
  //
  // Tree links 
  //
  
  UI_Box *first;
  UI_Box *last;
  UI_Box *next;
  UI_Box *prev;
  UI_Box *parent;
  
  //
  // Cross-frame box caching
  //
  
  UI_Key key;
  UI_Box *hash_prev;
  UI_Box *hash_next;
  U64 first_frame_touched;
  U64 last_frame_touched;  
  
  //
  // Per-frame data provided by UI-building code
  //
  
  UI_BoxFlags flags;
  UI_Size semantic_size[Axis2D_COUNT];
  Axis2D child_layout_axis;
  V4F32 bg_color;
  V4F32 border_color;
  F32 border_thickness;
  F32 corner_radius;
  OS_CursorKind hover_cursor;
  
  // Text
  R_Font *font;
  F32 font_size;
  String text;
  V4F32 text_color;
  V2S32 text_dim; 
  UI_TextAlignment text_align;
  
  //
  // Data computed each frame 
  //
  
  F32 computed_rel_pos[Axis2D_COUNT]; 
  F32 computed_size[Axis2D_COUNT];
  RectF32 rel_rect;
  RectF32 computed_rect; 
  
  //
  // Persistent animation data 
  //
  
  F32 hot_a; 
  F32 active_a; 
};

struct UI_BoxSlot {
  UI_Box *first;
  UI_Box *last; 
};

struct UI_Interaction {
  V2S32 cursor_pos; 
  OS_Key key; 
  OS_Modifiers modifiers;
  B32 hovered;    
  B32 pressed[UI_MouseButtonSlot_COUNT];
  B32 released[UI_MouseButtonSlot_COUNT];
  B32 clicked[UI_MouseButtonSlot_COUNT];
};

#endif // UI_TYPES_H

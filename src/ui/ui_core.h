#ifndef UI_CORE_H
#define UI_CORE_H

#define UI_TABLE_SLOTS_MAX 1024
#define UI_KeyZero() (UI_Key){0} // TODO: Use nil struct

//
// UI state
//

struct UI_Context {
  // Persistent box state
  Arena *arena; 
  UI_BoxSlot *box_table; 
  
  // Per-frame transient state
  Arena *frame_arena; 
  UI_Box *box_root; 
  V2S32 cursor_pos; 
  
  // NOTE: We create a copy of the passed event list because the
  // ui layer will needs to remove events from the list; removing
  // events directly from the os event list would be problematic
  // for other layers that need to read os events.
  OS_EventList *event_list;
  
  // Hot/active box keys
  // Note: Three boxes can be active at a time -- one for each 
  // mouse button. This is because if we right click something, i.e.
  // a menu that becomes active/opens on right clicking, that menu 
  // shouldnt become inactive/close when something is left clicked.
  UI_Key key_hot;
  UI_Key key_active[UI_MouseButtonSlot_COUNT];
  
  // Persistent state
  R_Font *default_font;
  F32 delta_time; 
  U64 frame_counter; 
  
  // Stacks 
  // Note: Should we allocate a separate arena for stacks?
  // Note: We never free popped nodes from stacks. Maybe this is 
  // where Ryan's apparent use of a free list comes into play? 
  UI_ParentNode *parent_first;
  UI_WidthNode  *width_first;
  UI_HeightNode *height_first;
  UI_ChildLayoutAxisNode *child_layout_axis_first;
  UI_BackgroundColorNode *bg_color_first;
  UI_TextColorNode *text_color_first;
  UI_BorderColorNode *border_color_first;
  UI_BorderThicknessNode *border_thickness_first;
  UI_CornerRadiusNode *corner_radius_first;
  UI_FontNode *font_first;
  UI_FontSizeNode *font_size_first;
  UI_TextAlignmentNode *text_align_first;
  UI_HoverCursorNode *hover_cursor_first;
  UI_BoxFlagsNode *box_flags_first;
};

global UI_Context *ui_ctx; // TODO: Consider making ui context local (definitely not on stack though) like rendering contexts.

//
// Initialization and per-frame operations
//

function void UI_Init(void);
function void UI_GetEvents(OS_EventList *events);
function void UI_Begin(OS_Handle window, F32 delta_time);
function void UI_End(void); 
function void UI_Draw(R_Context *render_context);

//
// More granular API wrapped by UI_Begin() and UI_End()
//

function void UI_BeginFrame(void);
function void UI_EndFrame(void);
function void UI_BeginBuild(OS_Handle window);
function void UI_EndBuild(void);
function void UI_Layout(void);
function void UI_Animate(F32 delta_time);

//
// UI Boxes 
//

function UI_Box *UI_BoxMake(UI_BoxFlags flags, String text);
function UI_Interaction UI_InteractionFromBox(UI_Box *box);

#endif // UI_CORE_H

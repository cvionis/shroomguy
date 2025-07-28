
//
// Miscellaneous internal helpers
//

#define UI_PersistentArena() ui_ctx->arena
#define UI_FrameArena() ui_ctx->frame_arena
#define UI_DeltaTime() ui_ctx->delta_time
#define UI_DefaultFont() ui_ctx->default_font

function UI_MouseButtonSlot
UI_MouseButtonSlotFromOSKey(OS_Key key)
{
  UI_MouseButtonSlot result = 0;
  switch (key) {
  case OS_Key_MouseLeft: {
    result = UI_MouseButtonSlot_Left;
  }break;
  case OS_Key_MouseMiddle: {
    result = UI_MouseButtonSlot_Middle;
  }break;
  case OS_Key_MouseRight: {
    result = UI_MouseButtonSlot_Right;
  }break;
  }
  return result;
}

// 
// Internal box key helpers
//

function UI_Key
UI_KeyFromString(String str)
{
  UI_Key key = {0};

  U32 hash = HashFromString(str);
  key.v = (U64)hash;

  return key; 
}

function B32 
UI_KeyEqual(UI_Key a, UI_Key b)
{
  B32 result = (a.v == b.v); 
  return result;
}

function UI_Box *
UI_BoxFromKey(UI_Key key)
{
  UI_Box *result = 0;
  U64 idx = key.v % UI_TABLE_SLOTS_MAX; 

  UI_BoxSlot *slot = &ui_ctx->box_table[idx]; 
  for (UI_Box *box = slot->first; box != 0; box = box->hash_next) {
    // TODO: Should probably make sure key is not the 'zero key'
    if (UI_KeyEqual(box->key, key)) {
      result = box;
      break;
    }
  }

  return result; 
}

//
// Initialization and per-frame operations
//

function void 
UI_Init(void)
{
  Arena *arena = ArenaAllocDefault();
  ui_ctx = ArenaPushStruct(arena, UI_Context);
  ui_ctx->arena = arena;

  ui_ctx->frame_arena = ArenaAllocDefault();

  ui_ctx->default_font = r_font_ttf_parse_and_bake(arena, StrLit("C:\\Windows\\Fonts\\Arial.ttf")); // TODO: Better font
  ui_ctx->box_table = ArenaPushArray(arena, UI_BoxSlot, UI_TABLE_SLOTS_MAX);
}

function void
UI_GetEvents(OS_EventList *events)
{
  ui_ctx->event_list = ArenaPushStruct(UI_FrameArena(), OS_EventList);
  for (OS_Event *event = events->first; event != 0; event = event->next) {
    OS_Event *copy = ArenaPushStruct(UI_FrameArena(), OS_Event);
    MemoryCopyStruct(copy, event);
    DLLPushBack(ui_ctx->event_list->first, ui_ctx->event_list->last, copy);
  }
}

function void 
UI_Begin(OS_Handle window, F32 delta_time)
{
  // TODO: Move this somewhere else since UI_Begin()/UI_End() 
  // are just wrappers around a more granular API.
  ui_ctx->delta_time = delta_time;

  UI_BeginFrame();
  UI_BeginBuild(window);
}

function void 
UI_End(void)
{
  UI_EndBuild();
  UI_Layout();
  UI_Animate(UI_DeltaTime());
  UI_EndFrame();
}

function void 
UI_BeginFrame(void)
{
}

function void 
UI_EndFrame(void)
{
  ui_ctx->frame_counter += 1;
}

function void 
UI_BeginBuild(OS_Handle window)
{
  // Reset per-frame state
  ArenaClear(UI_FrameArena());
  MemoryZeroStruct(&ui_ctx->key_hot);
  ui_ctx->cursor_pos = OS_WindowCursorPosition(window);

  U64 frame_counter = ui_ctx->frame_counter;

  // Remove stale boxes from the box table
  for (U32 slot_idx = 0; slot_idx < UI_TABLE_SLOTS_MAX; slot_idx += 1) {
    UI_BoxSlot *slot = &ui_ctx->box_table[slot_idx]; 

    for (UI_Box *box = slot->first; box != 0; box = box->hash_next) {
      if (UI_KeyEqual(box->key, UI_KeyZero()) || box->last_frame_touched+1 < frame_counter) {
        DLLRemoveNP(slot->first, slot->last, box, hash_next, hash_prev);
      }
    }
  }

  // Build a window-sized root box

  RectU32 client_rect = OS_WindowClientRect(window);
  V2U32 client_size = rect_u32_dim(client_rect);

  UI_PushWidth(UI_Pixels((F32)client_size.x, 1.f)); 
  UI_PushHeight(UI_Pixels((F32)client_size.y, 1.f)); 
  UI_PushCornerRadius(0.f);
  UI_PushBackgroundColor(v4f32(0.28f,0.28f,0.28f,1.f));
  UI_PushBorderThickness(0.5f);
  UI_PushBorderColor(v4f32(0.48f,0.48f,0.48f,1.f));
  UI_PushTextColor(v4f32(0.654f,0.654f,0.654f,1.f));
  UI_PushFontSize(12.f);
  UI_PushFont(UI_DefaultFont());
  UI_PushChildLayoutAxis(Axis2D_X);
  UI_PushTextAlignment(UI_TextAlignment_Center);
  UI_PushHoverCursor(OS_CursorKind_Arrow);
  UI_PushBoxFlags(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder);

  UI_Box *root = UI_BoxMake(0, StrLit("window_root"));

  // TODO: This isn't working properly
  UI_PopBoxFlags();
  UI_PopBorderColor();
  UI_PopBorderThickness();

  ui_ctx->box_root = root;
  UI_PushParent(root);
}

function void 
UI_EndBuild(void)
{
  UI_PopParent();

  UI_Box *box = UI_BoxFromKey(ui_ctx->key_hot);
  if (box) {
    OS_SetCursor(box->hover_cursor);
  }
  else {
    OS_SetCursor(OS_CursorKind_Arrow);
  }
}

function void UI_ComputeExplicitSizes(UI_Box *root, Axis2D axis);
function void UI_ComputeParentDependentSizes(UI_Box *root, Axis2D axis);
function void UI_ComputeChildDependentSizes(UI_Box *root, Axis2D axis);

function void 
UI_ComputeExplicitSizes(UI_Box *root, Axis2D axis)
{
  UI_Size semantic_axis_size = root->semantic_size[axis];

  switch (semantic_axis_size.kind) {
  case UI_SizeKind_Pixels: {
    root->computed_size[axis] = root->semantic_size[axis].value;
  }break;
  case UI_SizeKind_FitText: { 
    String text = root->text;
    (void)text;
    V2S32 text_dim = root->text_dim;

    switch (axis) {
    case Axis2D_X: {
      root->computed_size[axis] = (F32)text_dim.x;
    }break;
    case Axis2D_Y: {
      root->computed_size[axis] = (F32)text_dim.y;
    }break;
    };
  }break;
  };

  for (UI_Box *child = root->first; child != 0; child = child->next) {
    UI_ComputeExplicitSizes(child, axis);
  }
}

function void 
UI_ComputeParentDependentSizes(UI_Box *root, Axis2D axis)
{
  UI_Size semantic_axis_size = root->semantic_size[axis];

  switch (semantic_axis_size.kind) {
  case UI_SizeKind_PctOfParent: {
    UI_Box *closest_valid_parent = 0;
    for (UI_Box *current = root->parent; current != 0; current = current->parent) {
      if (current->semantic_size[axis].kind != UI_SizeKind_ChildrenSum) {
        closest_valid_parent = current;
        break;
      }
    }
    if (closest_valid_parent) {
      F32 pct = semantic_axis_size.value;
      F32 result = pct * closest_valid_parent->computed_size[axis];
      root->computed_size[axis] = floorf32(result);
    }
  }break;
  };

  for (UI_Box *child = root->first; child != 0; child = child->next) {
    UI_ComputeParentDependentSizes(child, axis);
  }
}

function void 
UI_ComputeChildDependentSizes(UI_Box *root, Axis2D axis)
{
  UI_Size semantic_axis_size = root->semantic_size[axis];

  switch (semantic_axis_size.kind) {
  case UI_SizeKind_ChildrenSum: {
    F32 children_sum = 0; 
    if (axis == root->child_layout_axis) {
      for (UI_Box *child = root->first; child != 0; child = child->next) {
        children_sum += child->computed_size[axis];
      }
    }
    else {
      for (UI_Box *child = root->first; child != 0; child = child->next) {
        children_sum = Max(children_sum, child->computed_size[axis]);
      }
    }
    root->computed_size[axis] = floorf32(children_sum);
  }break;
  };

  for (UI_Box *child = root->first; child != 0; child = child->next) {
    UI_ComputeChildDependentSizes(child, axis);
  }
}

function void 
UI_ResolveSizeViolations(UI_Box *root, Axis2D axis)
{
  F32 available_space = root->computed_size[axis];
  F32 children_taken_space = 0.f;
  F32 space_removal_budget = 0.f;

  // Determine our budget for correcting sizes of children given the space
  // that their preferred sizes take up and the strictness given to each those 
  // preferred sizes when they were built.

  for (UI_Box *child = root->first; child != 0; child = child->next) {
    if (axis == root->child_layout_axis) {
      children_taken_space += child->computed_size[axis];
    }
    else {
      children_taken_space = Max(children_taken_space, child->computed_size[axis]);
    }
    F32 child_space_removal_budget = child->computed_size[axis] * (1.f - child->semantic_size[axis].strictness);
    space_removal_budget += child_space_removal_budget;
  }

  // Adjust size of children given the total space violation and the available space 
  // adjustment budget.

  F32 violation = children_taken_space - available_space;
  if (violation > 0.f && space_removal_budget > 0.f) {
    for (UI_Box *child = root->first; child != 0; child = child->next) {
      F32 child_space_removal_budget = child->computed_size[axis] * (1.f - child->semantic_size[axis].strictness);
      F32 child_size_adjustment = 0.f;

      if (axis == root->child_layout_axis) {
        child_size_adjustment = child_space_removal_budget * (violation / space_removal_budget);
      }
      else {
        child_size_adjustment = child->computed_size[axis] - available_space;
      }
      child_size_adjustment = Clamp(child_size_adjustment, 0.f, child_space_removal_budget);
      child->computed_size[axis] -= child_size_adjustment;
      child->computed_size[axis] = floorf32(child->computed_size[axis]);
    }
  }

  // Calculate the relative positions of this box's children
  
  if (axis == root->child_layout_axis) {
    F32 pos = 0.f;
    for (UI_Box *child = root->first; child != 0; child = child->next) {
      child->computed_rel_pos[axis] = pos;
      pos += child->computed_size[axis];
    }
  }
  else {
    for (UI_Box *child = root->first; child != 0; child = child->next) {
      child->computed_rel_pos[axis] = 0.f;
    }
  }

  // Calculate the final rects for each of this box's children

  for (UI_Box *child = root->first; child != 0; child = child->next) {
    child->rel_rect.p0.e[axis] = child->computed_rel_pos[axis];
    child->rel_rect.p1.e[axis] = child->rel_rect.p0.e[axis] + child->computed_size[axis];

    child->computed_rect.p0.e[axis] = root->computed_rect.p0.e[axis] + child->rel_rect.p0.e[axis];
    child->computed_rect.p1.e[axis] = child->computed_rect.p0.e[axis] + child->computed_size[axis];

    child->computed_rect.p0.e[axis] = floorf32(child->computed_rect.p0.e[axis]);
    child->computed_rect.p1.e[axis] = floorf32(child->computed_rect.p1.e[axis]);
  }

  // Recurse

  for (UI_Box *child = root->first; child != 0; child = child->next) {
    UI_ResolveSizeViolations(child, axis);
  }
}

function void 
UI_Layout(void)
{
  UI_Box *root = ui_ctx->box_root;
  for (Axis2D axis = 0; axis < Axis2D_COUNT; axis += 1) {
    UI_ComputeExplicitSizes(root, axis);
    UI_ComputeParentDependentSizes(root, axis);
    UI_ComputeChildDependentSizes(root, axis);
    UI_ResolveSizeViolations(root, axis);
  }
}

// TODO: Use a nonlinear curve that "ramps up"
function void 
UI_Animate(F32 delta_time)
{
  F32 hot_speed_in = 0.65f; 
  F32 hot_speed_out = 0.55f;

  F32 active_speed_in = 0.65f; 
  F32 active_speed_out = 0.55f;

  for (U32 slot_idx = 0; slot_idx < UI_TABLE_SLOTS_MAX; slot_idx += 1) {
    UI_BoxSlot *slot = &ui_ctx->box_table[slot_idx]; 

    for (UI_Box *box = slot->first; box != 0; box = box->hash_next) {
      if (UI_KeyEqual(box->key, ui_ctx->key_hot)) {
        box->hot_a += delta_time * hot_speed_in;
        box->hot_a = Clamp(box->hot_a, 0.f, 1.f);
      }
      else {
        box->hot_a -= delta_time * hot_speed_out;
        box->hot_a = Clamp(box->hot_a, 0.f, 1.f);
      }
      if (UI_KeyEqual(box->key, ui_ctx->key_active[UI_MouseButtonSlot_Left])) {
        box->active_a += delta_time * active_speed_in;
        box->active_a = Clamp(box->active_a, 0.f, 1.f);
      }
      else {
        box->active_a -= delta_time * active_speed_out;
        box->active_a = Clamp(box->active_a, 0.f, 1.f);
      }
    }
  }
}

typedef struct UI_BoxRecurse UI_BoxRecurse;
struct UI_BoxRecurse
{
  UI_Box *next;
  S32 push_count;
  S32 pop_count; 
};

function UI_BoxRecurse
UI_BoxDepthFirstPostStep(UI_Box *box, UI_Box *stopper)
{
  UI_BoxRecurse result = {0};

  // One step in descending the tree?
  if (box->last) {
    result.next = box->last; 
    result.push_count = 1; 
  }
  // One step in returning upwards in the tree, after we've exhausted all
  // children (hence why this is executed when the box doesn't have a `last` child)?
  else for (UI_Box *b = box; (b != 0) && (b != stopper); b = b->parent) {
    if (b->prev) {
      result.next = b->prev; 
      break;
    }
    result.pop_count += 1; 
  }

  return result; 
}

function void 
UI_Draw(R_Context *render_context)
{
  for (UI_Box *box = ui_ctx->box_root, *next_box = 0; box != 0; box = next_box) {
    // Grab the next box from our depth-first traversal
    UI_BoxRecurse rec = UI_BoxDepthFirstPostStep(box, 0); 
    next_box = rec.next;

    // 
    // Drop shadow (TODO)
    //

    if (box->flags & UI_BoxFlag_DrawDropShadow) {
    }
    
    R_Quad quad = {0};

    quad.rect.x0 = box->computed_rect.p0.x;
    quad.rect.x1 = box->computed_rect.p1.x;
    quad.rect.y0 = box->computed_rect.p0.y;
    quad.rect.y1 = box->computed_rect.p1.y;
    quad.uv_rect = rect_f32(0,0,1,1);

    quad.radius = box->corner_radius;
    quad.corner_softness = 1.f;

    //
    // Background 
    //

    if (box->flags & UI_BoxFlag_DrawBackground) {
      V4F32 hover_color = v4f32_add(box->bg_color, v4f32(0.052f,0.052f,0.052f,0.f));
      V4F32 color = v4f32_lerp(box->bg_color, hover_color, box->hot_a);

      V4F32 active_color = v4f32_sub(box->bg_color, v4f32(0.052f,0.052f,0.052f,0.f));
      color = v4f32_lerp(color, active_color, box->active_a);

      quad.colors[0] = color;
      quad.colors[1] = color;
      quad.colors[2] = color;
      quad.colors[3] = color;
      quad.corner_softness= 0.1f;

      r_quad(render_context, &quad, 0); 
    }

    //
    // Border
    //

    if (box->flags & UI_BoxFlag_DrawBorder) {
      V4F32 hover_color = v4f32_add(box->border_color, v4f32(0.06f,0.06f,0.06f,0.f));
      V4F32 color = v4f32_lerp(box->border_color, hover_color, Max(box->active_a, box->hot_a));

      quad.colors[0] = color;
      quad.colors[1] = color;
      quad.colors[2] = color;
      quad.colors[3] = color;

      F32 hover_thickness = 2.2f;
      F32 thickness = lerp(box->border_thickness, hover_thickness, Max(box->active_a, box->hot_a));
      quad.border_thickness = thickness;
      quad.corner_softness= 0.1f;

      r_quad(render_context, &quad, 0); 
    }

    // 
    // Text string
    // 

    if (box->flags & UI_BoxFlag_DrawText) {
      V2S32 text_dim = box->text_dim;
      V2F32 text_pos = {0};

      switch (box->text_align) {
      case UI_TextAlignment_Left: {
        text_pos.x = quad.rect.x0; 
      }break;
      case UI_TextAlignment_Right: {
        text_pos.x = quad.rect.x1 - text_dim.x; 
      }break;
      case UI_TextAlignment_Center: {
        text_pos.x = quad.rect.x0+(quad.rect.x1-quad.rect.x0)/2-text_dim.x/2;
        text_pos.x = floorf32(text_pos.x);
      }break;
      };

      //text_pos.y = quad.rect.y0+(quad.rect.y1-quad.rect.y0)/2-text_dim.y/2;
      text_pos.y = quad.rect.y0+(quad.rect.y1-quad.rect.y0)/2-text_dim.y/2;
      text_pos.y = floorf32(text_pos.y);

      R_Font *font = box->font;
      F32 font_pt = box->font_size;
      V4F32 text_color = box->text_color;

      r_text(render_context, box->text, font, font_pt, text_pos, 
        text_color, &quad.rect);
    }
  }
}

// UI boxes

function UI_Interaction
UI_InteractionFromBox(UI_Box *box)
{
  UI_Interaction result = {0};

  RectF32 rect = box->computed_rect;

  for (OS_Event *event = ui_ctx->event_list->first; event != 0; event = event->next) {
    B32 event_taken = 0;

    V2S32 cursor = event->pos;

    B32 key_is_mouse = (event->kind == OS_EventKind_MousePress || OS_EventKind_MouseRelease);
    B32 cursor_inside_box = 
        (cursor.x >=  rect.p0.x) && 
        (cursor.y >=  rect.p0.y) &&
        (cursor.x <= rect.p1.x) && 
        (cursor.y <= rect.p1.y); 

    // Mouse button events
    if ((box->first_frame_touched != box->last_frame_touched) && (box->flags & UI_BoxFlag_Clickable) && key_is_mouse) {
      UI_MouseButtonSlot btn_slot = UI_MouseButtonSlotFromOSKey(event->key);

      if (cursor_inside_box && (event->kind == OS_EventKind_MousePress)) {
        ui_ctx->key_hot = box->key;
        ui_ctx->key_active[btn_slot] = box->key; 
        result.pressed[btn_slot] = 1;
        event_taken = 1;
      }
      if ((event->kind == OS_EventKind_MouseRelease) && UI_KeyEqual(ui_ctx->key_active[btn_slot], box->key)) {
        result.released[btn_slot] = 1;
        if (cursor_inside_box) {
          result.clicked[btn_slot] = 1;
        }
        ui_ctx->key_active[btn_slot] = UI_KeyZero();
        event_taken = 1;
      }
    }

    // TODO: Keyboard-clickable box flag
    // Note: Not sure how to go about this yet.
    if (event->kind == OS_EventKind_KeyRelease) {
      result.key = event->key;
      result.modifiers = event->modifiers;
    }

    // Event belongs to this box and none else; remove it from the list
    if (event_taken) {
      DLLRemove(ui_ctx->event_list->first, ui_ctx->event_list->last, event);
    }
  }

  // Check mouse hover 

  V2S32 cursor = ui_ctx->cursor_pos;
  result.cursor_pos = cursor;

  B32 cursor_inside_box = 
    (cursor.x >= rect.p0.x) &&
    (cursor.y >= rect.p0.y) &&
    (cursor.x <= rect.p1.x) &&
    (cursor.y <= rect.p1.y); 

  if (cursor_inside_box) {
    if (UI_KeyEqual(ui_ctx->key_hot, UI_KeyZero())) {
      result.hovered = 1;
      ui_ctx->key_hot = box->key;
    }
    else if (UI_KeyEqual(box->key, ui_ctx->key_hot)) {
      ui_ctx->key_hot = UI_KeyZero();
    }
  }

  return result;
}

function UI_Box *
UI_BoxMake(UI_BoxFlags flags, String text)
{
  // Allocate a new box if it doesn't already exist in the cache
  UI_Key key = UI_KeyFromString(text);
  UI_Box *box = UI_BoxFromKey(key);
  if (!box) {
    box = ArenaPushStruct(UI_PersistentArena(), UI_Box);

    // TODO: Don't calculate when flags don't include DrawText
    // TODO: This can't really be cached unconditionally, because some boxes have dynamic text. 
    box->text_dim = F_TextStringDimPx(text, UI_TopFont()->cache, UI_TopFontSize());
    box->first_frame_touched = ui_ctx->frame_counter;

    U64 slot = key.v % UI_TABLE_SLOTS_MAX;
    DLLPushBackNP(ui_ctx->box_table[slot].first, ui_ctx->box_table[slot].last, box, hash_next, hash_prev);
  }

  // Set the box's parent
  UI_Box *parent = UI_TopParent();
  if (parent) {
    DLLPushBackNP(parent->first, parent->last, box, next, prev);
    box->parent = parent;
  }

  // Set per-frame state
  box->last_frame_touched = ui_ctx->frame_counter;
  box->first = box->last = 0; 
  box->flags = flags | UI_TopBoxFlags();
  box->text = text;
  box->key = key;
  box->semantic_size[Axis2D_X] = UI_TopWidth();
  box->semantic_size[Axis2D_Y] = UI_TopHeight();
  box->child_layout_axis = UI_TopChildLayoutAxis();
  box->bg_color = UI_TopBackgroundColor();
  box->text_color = UI_TopTextColor();
  box->border_color = UI_TopBorderColor();
  box->border_thickness = UI_TopBorderThickness();
  box->corner_radius = UI_TopCornerRadius();
  box->font = UI_TopFont();
  box->font_size = UI_TopFontSize();
  box->text_align = UI_TopTextAlignment();
  box->hover_cursor = UI_TopHoverCursor();

  return box;
}

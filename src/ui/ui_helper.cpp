//
// UI Box style stack helpers
//

function UI_BoxFlags
UI_TopBoxFlags(void)
{
  UI_BoxFlags result = 0;
  UI_BoxFlagsNode *first = ui_ctx->box_flags_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void
UI_PushBoxFlags(UI_BoxFlags flags)
{
  UI_BoxFlagsNode *node = ArenaPushStruct(UI_FrameArena(), UI_BoxFlagsNode);
  node->v = flags; 
  SLLStackPush(ui_ctx->box_flags_first, node);
}

function void
UI_PopBoxFlags(void)
{
  SLLStackPop(ui_ctx->box_flags_first);
}

function OS_CursorKind 
UI_TopHoverCursor(void)
{
  OS_CursorKind result = OS_CursorKind_Null;
  UI_HoverCursorNode *first = ui_ctx->hover_cursor_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushHoverCursor(OS_CursorKind cursor)
{
  UI_HoverCursorNode *node = ArenaPushStruct(UI_FrameArena(), UI_HoverCursorNode);
  node->v = cursor; 
  SLLStackPush(ui_ctx->hover_cursor_first, node);
}

function void
UI_PopHoverCursor(void)
{
  SLLStackPop(ui_ctx->hover_cursor_first);
}

function UI_TextAlignment 
UI_TopTextAlignment(void)
{
  UI_TextAlignment result = 0;
  UI_TextAlignmentNode *first = ui_ctx->text_align_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushTextAlignment(UI_TextAlignment align)
{
  UI_TextAlignmentNode *node = ArenaPushStruct(UI_FrameArena(), UI_TextAlignmentNode);
  node->v = align; 
  SLLStackPush(ui_ctx->text_align_first, node);
}

function void
UI_PopTextAlignment(void)
{
  SLLStackPop(ui_ctx->text_align_first);
}

function R_Font *
UI_TopFont(void)
{
  R_Font *result = {0};
  UI_FontNode *first = ui_ctx->font_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushFont(R_Font *font)
{
  UI_FontNode *node = ArenaPushStruct(UI_FrameArena(), UI_FontNode);
  node->v = font; 
  SLLStackPush(ui_ctx->font_first, node);
}

function void
UI_PopFont(void)
{
  SLLStackPop(ui_ctx->font_first);
}

function F32
UI_TopFontSize(void)
{
  F32 result = {0};
  UI_FontSizeNode *first = ui_ctx->font_size_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushFontSize(F32 pt)
{
  UI_FontSizeNode *node = ArenaPushStruct(UI_FrameArena(), UI_FontSizeNode);
  node->v = pt; 
  SLLStackPush(ui_ctx->font_size_first, node);
}

function void
UI_PopFontSize(void)
{
  SLLStackPop(ui_ctx->font_size_first);
}

function F32
UI_TopBorderThickness(void)
{
  F32 result = {0};
  UI_BorderThicknessNode *first = ui_ctx->border_thickness_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushBorderThickness(F32 thickness)
{
  UI_BorderThicknessNode *node = ArenaPushStruct(UI_FrameArena(), UI_BorderThicknessNode);
  node->v = thickness; 
  SLLStackPush(ui_ctx->border_thickness_first, node);
}

function void
UI_PopBorderThickness(void)
{
  SLLStackPop(ui_ctx->border_thickness_first);
}

function F32
UI_TopCornerRadius(void)
{
  F32 result = {0};
  UI_CornerRadiusNode *first = ui_ctx->corner_radius_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushCornerRadius(F32 color)
{
  UI_CornerRadiusNode *node = ArenaPushStruct(UI_FrameArena(), UI_CornerRadiusNode);
  node->v = color; 
  SLLStackPush(ui_ctx->corner_radius_first, node);
}

function void
UI_PopCornerRadius(void)
{
  SLLStackPop(ui_ctx->corner_radius_first);
}

function V4F32 
UI_TopBorderColor(void)
{
  V4F32 result = {0};
  UI_BorderColorNode *first = ui_ctx->border_color_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushBorderColor(V4F32 color)
{
  UI_BorderColorNode *node = ArenaPushStruct(UI_FrameArena(), UI_BorderColorNode);
  node->v = color; 
  SLLStackPush(ui_ctx->border_color_first, node);
}

function void
UI_PopBorderColor(void)
{
  SLLStackPop(ui_ctx->border_color_first);
}

function V4F32
UI_TopTextColor(void)
{
  V4F32 result = {0};
  UI_TextColorNode *first = ui_ctx->text_color_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushTextColor(V4F32 color)
{
  UI_TextColorNode *node = ArenaPushStruct(UI_FrameArena(), UI_TextColorNode);
  node->v = color; 
  SLLStackPush(ui_ctx->text_color_first, node);
}

function void
UI_PopTextColor(void)
{
  SLLStackPop(ui_ctx->text_color_first);
}

function V4F32
UI_TopBackgroundColor(void)
{
  V4F32 result = {0};
  UI_BackgroundColorNode *first = ui_ctx->bg_color_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushBackgroundColor(V4F32 color)
{
  UI_BackgroundColorNode *node = ArenaPushStruct(UI_FrameArena(), UI_BackgroundColorNode);
  node->v = color; 
  SLLStackPush(ui_ctx->bg_color_first, node);
}

function void
UI_PopBackgroundColor(void)
{
  SLLStackPop(ui_ctx->bg_color_first);
}

function Axis2D
UI_TopChildLayoutAxis(void)
{
  Axis2D result = Axis2D_X;
  UI_ChildLayoutAxisNode *first = ui_ctx->child_layout_axis_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushChildLayoutAxis(Axis2D axis)
{
  UI_ChildLayoutAxisNode *node = ArenaPushStruct(UI_FrameArena(), UI_ChildLayoutAxisNode);
  node->v = axis; 
  SLLStackPush(ui_ctx->child_layout_axis_first, node);
}

function void
UI_PopChildLayoutAxis(void)
{
  SLLStackPop(ui_ctx->child_layout_axis_first);
}

function UI_Box *
UI_TopParent(void)
{
  UI_Box *result = 0;
  UI_ParentNode *first = ui_ctx->parent_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void 
UI_PushParent(UI_Box *box)
{
  UI_ParentNode *node = ArenaPushStruct(UI_FrameArena(), UI_ParentNode);
  node->v = box; 
  SLLStackPush(ui_ctx->parent_first, node);
}

function void 
UI_PopParent(void)
{
  SLLStackPop(ui_ctx->parent_first);
}

function UI_Size
UI_TopWidth(void)
{
  UI_Size result = {0};
  UI_WidthNode *first = ui_ctx->width_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void
UI_PushWidth(UI_Size width)
{
  UI_WidthNode *node = ArenaPushStruct(UI_FrameArena(), UI_WidthNode);
  node->v = width;
  SLLStackPush(ui_ctx->width_first, node);
}

function void
UI_PopWidth(void)
{
  SLLStackPop(ui_ctx->width_first);
}

function UI_Size
UI_TopHeight(void)
{
  UI_Size result = {0};
  UI_HeightNode *first = ui_ctx->height_first;
  if (first) {
    result = first->v;
  }
  return result;
}

function void
UI_PushHeight(UI_Size height)
{
  UI_HeightNode *node = ArenaPushStruct(UI_FrameArena(), UI_HeightNode);
  node->v = height;
  SLLStackPush(ui_ctx->height_first, node);
}

function void
UI_PopHeight(void)
{
  SLLStackPop(ui_ctx->height_first);
}

//
// Semantic size helpers
//

function UI_Size
UI_FitText(F32 strictness)
{
  UI_Size size = {
    .kind = UI_SizeKind_FitText,
    .value = 0,
    .strictness = strictness,
  };
  return size; 
}

function UI_Size
UI_Pixels(F32 value, F32 strictness)
{
  UI_Size size = {
    .kind = UI_SizeKind_Pixels,
    .value = value,
    .strictness = strictness,
  };
  return size; 
}

function UI_Size
UI_PctOfParent(F32 value, F32 strictness)
{
  UI_Size size = {
    .kind = UI_SizeKind_PctOfParent,
    .value = value,
    .strictness = strictness,
  };
  return size; 
}

function UI_Size
UI_ChildrenSum(F32 strictness)
{
  UI_Size size = {
    .kind = UI_SizeKind_ChildrenSum,
    .value = 0,
    .strictness = strictness,
  };
  return size;
}


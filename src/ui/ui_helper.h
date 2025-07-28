#ifndef UI_HELPER_H
#define UI_HELPER_H

//
// UI Box style stack types
//

struct UI_BoxFlagsNode {
  UI_BoxFlagsNode *next;
  UI_BoxFlags v;
};

struct UI_HoverCursorNode {
  UI_HoverCursorNode *next;
  OS_CursorKind v;
};

struct UI_TextAlignmentNode {
  UI_TextAlignmentNode *next;
  UI_TextAlignment v; 
};

struct UI_FontNode {
  UI_FontNode *next;
  R_Font *v; 
};

struct UI_FontSizeNode {
  UI_FontSizeNode *next;
  F32 v; 
};

struct UI_BackgroundColorNode {
  UI_BackgroundColorNode *next;
  V4F32 v;
};

struct UI_TextColorNode {
  UI_TextColorNode *next;
  V4F32 v;
};

struct UI_BorderColorNode {
  UI_BorderColorNode *next;
  V4F32 v;
};

struct UI_BorderThicknessNode {
  UI_BorderThicknessNode *next;
  F32 v;
};

struct UI_CornerRadiusNode  {
  UI_CornerRadiusNode *next;
  F32 v;
};

struct UI_ChildLayoutAxisNode {
  UI_ChildLayoutAxisNode *next; 
  Axis2D v;
};

struct UI_ParentNode {
  UI_ParentNode *next;
  UI_Box *v; 
};

struct UI_WidthNode {
  UI_WidthNode *next;
  UI_Size v; 
};

struct UI_HeightNode {
  UI_HeightNode *next;
  UI_Size v; 
};

//
// UI Box style stack helpers
//

// Box flags
function UI_BoxFlags UI_TopBoxFlags(void);
function void UI_PushBoxFlags(UI_BoxFlags flags);
function void UI_PopBoxFlags(void);
#define UI_BoxFlags(v) DeferLoop(UI_PushBoxFlags(v),UI_PopBoxFlags())

// Hover cursor
function OS_CursorKind UI_TopHoverCursor(void);
function void UI_PushHoverCursor(OS_CursorKind cursor);
function void UI_PopHoverCursor(void);
#define UI_HoverCursor(v) DeferLoop(UI_PushHoverCursor(v),UI_PopHoverCursor())

// Text alignment
function UI_TextAlignment UI_TopTextAlignment(void);
function void UI_PushTextAlignment(UI_TextAlignment align);
function void UI_PopTextAlignment(void);
#define UI_TextAlignment(v) DeferLoop(UI_PushTextAlignment(v),UI_PopTextAlignment())

// Font
function R_Font *UI_TopFont(void);
function void UI_PushFont(R_Font *font);
function void UI_PopFont(void);
#define UI_Font(v) DeferLoop(UI_PushFont(v),UI_PopFont())

// Font size
function F32 UI_TopFontSize(void);
function void UI_PushFontSize(F32 pt);
function void UI_PopFontSize(void);
#define UI_FontSize(v) DeferLoop(UI_PushFontSize(v),UI_PopFontSize())

// Border thickness
function F32 UI_TopBorderThickness(void);
function void UI_PushBorderThickness(F32 thickness);
function void UI_PopBorderThickness(void);
#define UI_BorderThickness(v) DeferLoop(UI_PushBorderThickness(v),UI_PopBorderThickness())

// Corner radius
function F32 UI_TopCornerRadius(void);
function void UI_PushCornerRadius(F32 color);
function void UI_PopCornerRadius(void);
#define UI_CornerRadius(v) DeferLoop(UI_PushCornerRadius(v),UI_PopCornerRadius())

// Border color
function V4F32 UI_TopBorderColor(void);
function void UI_PushBorderColor(V4F32 color);
function void UI_PopBorderColor(void);
#define UI_BorderColor(v) DeferLoop(UI_PushBorderColor(v),UI_PopBorderColor())

// Text color
function V4F32 UI_TopTextColor(void);
function void UI_PushTextColor(V4F32 color);
function void UI_PopTextColor(void);
#define UI_TextColor(v) DeferLoop(UI_PushTextColor(v),UI_PopTextColor())

// Background color
function V4F32 UI_TopBackgroundColor(void);
function void UI_PushBackgroundColor(V4F32 color);
function void UI_PopBackgroundColor(void);
#define UI_BackgroundColor(v) DeferLoop(UI_PushBackgroundColor(v),UI_PopBackgroundColor())

// Child layout axis
function Axis2D UI_TopChildLayoutAxis(void);
function void UI_PushChildLayoutAxis(Axis2D axis);
function void UI_PopChildLayoutAxis(void);
#define UI_ChildLayoutAxis(v) DeferLoop(UI_PushChildLayoutAxis(v),UI_PopChildLayoutAxis())

// Parent box
function UI_Box *UI_TopParent(void);
function void UI_PushParent(UI_Box *box);
function void UI_PopParent(void);
#define UI_Parent(v) DeferLoop(UI_PushParent(v),UI_PopParent())

// Width
function UI_Size UI_TopWidth(void);
function void UI_PushWidth(UI_Size width);
function void UI_PopWidth(void);
#define UI_Width(v) DeferLoop(UI_PushWidth(v),UI_PopWidth())

// Height
function UI_Size UI_TopHeight(void);
function void UI_PushHeight(UI_Size height);
function void UI_PopHeight(void);
#define UI_Height(v) DeferLoop(UI_PushHeight(v),UI_PopHeight())

//
// Semantic size helpers
//

function UI_Size UI_FitText(F32 strictness);
function UI_Size UI_Pixels(F32 value, F32 strictness);
function UI_Size UI_PctOfParent(F32 value, F32 strictness);
function UI_Size UI_ChildrenSum(F32 strictness);

#endif // UI_HELPER_H 

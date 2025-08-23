#include "base/base_inc.h"
#include "os/os_inc.h"
#include "font/font_inc.h"
#include "render/render_inc.h"
#include "sound/sound_inc.h"
#include "async/async_inc.h"
#include "game/game_inc.h"

#include "base/base_inc.cpp"
#include "os/os_inc.cpp"
#include "font/font_inc.cpp"
#include "render/render_inc.cpp"
#include "sound/sound_inc.cpp"
#include "async/async_inc.cpp"
#include "game/game_inc.cpp"

#define ED_DEFAULT_MAPS_PATH S8("../data/maps/default.maps")

struct ED_Context {
  Arena *arena;
  OS_Handle window;
  R_Context *renderer;
  R_Font *font;
  
  B32 quit;
  
  G_MapStorage maps;
};

global ED_Context editor;

function void
ed_update(ED_Context *editor)
{
  OS_EventList *events = os_get_events();
  for (OS_Event *e = events->first; e != 0; e = e->next) {
    if (e->kind == OS_EventKind_WindowClose) {
      editor->quit = 1;
    }
  }
}

function void
ed_render(ED_Context *editor) 
{
  OS_Handle window = editor->window;
  R_Context *renderer = editor->renderer;
  R_Font *font = editor->font;
  
  r_set_clear_color(v4f32(0,0,0,1));
  r_set_viewport(os_window_client_rect(window));
  r_begin_frame(renderer); 
  
  V4F32 col = v4f32(0.4f,0.4f,0.4f,1.f);
  r_text(renderer, ED_DEFAULT_MAPS_PATH, font, 32, v2f32(16,8), col, 0);
  
  r_flush(renderer);
  r_end_frame(renderer);
}

void 
entry_point(void) 
{
  os_init();
  os_gfx_init();
  r_init(); 
  
  editor.arena = arena_alloc_default();
  editor.window = os_window_open(S8("Editor"), 1280, 720);
  
  r_equip_window(editor.window);
  editor.renderer = r_context_alloc();
  editor.font = r_font_ttf_parse_and_bake(editor.arena, S8("../data/fonts/Ithaca.ttf"));
  
  while (!editor.quit) {
    ed_update(&editor);
    ed_render(&editor);
  }
  
  r_context_release(editor.renderer);
  os_window_close(editor.window);
  arena_release(editor.arena);
}

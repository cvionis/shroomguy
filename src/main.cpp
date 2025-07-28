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

struct GameState {
  Arena *arena;
  OS_Handle window;
  R_Context *renderer;
  R_Font *font;
  B32 quit;
};

global GameState game;

function void 
update(void)
{
  OS_Handle window = game.window;
  
  OS_EventList *events = os_get_events();
  
  for (OS_Event *e = events->first; e != 0; e = e->next) {
    if (e->kind == OS_EventKind_WindowClose) {
      game.quit = 1;
    }
  }
}

function void 
render(void)
{
  OS_Handle window = game.window;
  R_Context *renderer = game.renderer;
  
  r_set_clear_color(v4f32(0,0,0,1));
  r_set_viewport(os_window_client_rect(window));
  r_begin_frame(renderer); 
  
#if 0
  {
    R_Quad quad = {0};
    quad.uv_rect = rect_f32(0,0,1,1);
    quad.rect = rect_f32(200,200,400,400);
    
    quad.colors[0] = v4f32(0.4f,0.5f,0.9f,1.f);
    quad.colors[1] = v4f32(0.2f,0.6f,0.9f,1.f);
    quad.colors[2] = v4f32(0.1f,0.8f,0.9f,1.f);
    quad.colors[3] = v4f32(0.0f,1.0f,0.9f,1.f);
    
    quad.radius = 20.f;
    quad.corner_softness = 1.f;
    r_quad(renderer, &quad, 0);
  }
  
  r_segment(renderer, v2f32(640, 400), v2f32(840, 200), 2, v4f32(0,1,0,1));
  r_text(renderer, S8("Sample Text"), game.font, 32, v2f32(500, 440), v4f32(1,1,1,1), 0);
  r_circle(renderer, v2f32(600,600), 40, 2, v4f32(0.4f,0.5f,0.9f,1.f));
#endif
  
  r_flush(renderer);
  r_end_frame(renderer);
}

void 
entry_point(void) 
{
  os_init();
  os_gfx_init();
  r_init(); 
  
  game.arena = arena_alloc_default();
  game.window = os_window_open(S8("Game"), 1280, 720);
  
  r_equip_window(game.window);
  game.renderer = r_context_alloc();
  game.font = r_font_ttf_parse_and_bake(game.arena, S8("../data/fonts/Ithaca.ttf"));
  
  while (!game.quit) {
    update();
    render();
  }
  
  r_context_release(game.renderer);
  os_window_close(game.window);
  arena_release(game.arena);
}
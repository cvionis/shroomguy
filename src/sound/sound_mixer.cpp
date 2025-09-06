function M_Context *
m_ctx_alloc(void)
{
  M_Context *ctx = 0;
  
  Arena *arena = arena_alloc_default();
  ctx = ArenaPushStruct(arena, M_Context);
  ctx->arena = arena;
  
  return ctx;
}

function void
m_ctx_release(M_Context *ctx)
{
  if (ctx) {
    arena_release(ctx->arena);
  }
}

function M_Sound *
m_sound_from_file(M_Context *ctx, String8 path)
{
  M_Sound *result = 0;
  return result;
}

// @Todo: Pos can probably just belong to sound, not source?
function void 
m_sound_play(M_Sound *sound)
{
  sound->source.pos = 0;
}

function void
m_sound_pause(M_Sound *sound)
{
  sound->stopped = 1;
}

function void
m_sound_stop(M_Sound *sound)
{
  sound->source.pos = 0;
  sound->stopped = 1;
}

function void 
m_sound_set_loop(M_Sound *sound, B32 loop)
{
  sound->loop = loop;
}

function void
m_sound_set_pan(M_Sound *sound, F32 pan)
{
  sound->pan = pan;
}

function void 
m_sound_set_pitch(M_Sound *sound, F32 pitch)
{
  sound->pitch = pitch;
}

function void 
m_begin_frame(M_Context *ctx)
{
  s_lock_buffer();
}

function void
m_end_frame(M_Context *ctx)
{
  s_unlock_buffer();
  
  // @Todo: Make sure this write count will be the same as the one used in m_process --
  // that is, that write count does not change between m_process and this function..
  U64 write_count = s_get_write_count();
  s_advance_buffer(write_count);
}

function void 
m_process(M_Context *ctx)
{
  U64 play_count = s_get_play_count();
  U32 bytes_per_sample = s_win32_backend->sample_buff_fmt->nBlockAlign; 
  
  for (M_SoundNode *n = ctx->first; n != 0; n = n->next) {
    M_Sound *sound = &n->sound;
    S_Source *source = sound->source;
    
    // @Todo: set the source's stopped = 1 when it's done, and handle looping.
    // @Note: M_Sound's playback speed would modify play count.
    if (!sound->stopped) {
      s_source_update(source, play_count);
    }
  }
}

function void 
m_mix(M_Context *ctx)
{
  U64 write_count = s_get_write_count();
  F32 *buff = s_get_sample_buffer();
  
  MemorySet(buff, 0, write_count * bytes_per_sample);
  
  for (M_SoundNode *n = ctx->first; n != 0; n = n->next) {
    M_Sound *sound = &n->sound;
    S_Source *source = sound->source;
    
    // @Note: M_Sound's volume, pan, pitch would be passed to m_source_mix.
    s_source_mix(&game.source, buff, write_count, 1.f);
  }
}
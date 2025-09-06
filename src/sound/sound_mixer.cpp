function M_Context *
m_context_alloc(void)
{
  M_Context *ctx = 0;
  
  Arena *arena = arena_alloc_default();
  ctx = ArenaPushStruct(arena, M_Context);
  ctx->arena = arena;
  
  return ctx;
}

function void
m_context_release(M_Context *ctx)
{
  if (ctx) {
    arena_release(ctx->arena);
  }
}

// @Todo: Don't crash when the file isn't found...
function M_Sound *
m_sound_from_file(M_Context *ctx, String8 path)
{
  M_Sound *result = 0;
  
  // @Todo
  U32 sample_rate = 44100;
  
  S_Source source = s_source_load(path, sample_rate);
  if (source.count > 0) {
    M_SoundNode *node = ArenaPushStruct(ctx->arena, M_SoundNode);
    SLLQueuePushN(ctx->sounds_first, ctx->sounds_last, node, list_next);
    
    node->v.volume = 1;
    node->v.pan = 0;
    node->v.pitch = 1;
    MemoryCopyStruct(&node->v.source, &source);
    
    result = &node->v;
  }
  
  return result;
}

// @Todo: Pos can probably just belong to sound, not source?
function void 
m_sound_play(M_Sound *sound)
{
  sound->active = 1;
  sound->source.pos = 0;
}

function void
m_sound_pause(M_Sound *sound)
{
  sound->active = 0;
}

function void
m_sound_stop(M_Sound *sound)
{
  sound->source.pos = 0;
  sound->active = 0;
}

function void 
m_sound_set_loop(M_Sound *sound, B32 loop)
{
  sound->loop = loop;
}

function void
m_sound_set_volume(M_Sound *sound, F32 volume)
{
  sound->volume = volume;
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
  (void *)ctx;
  s_lock_buffer();
}

function void
m_end_frame(M_Context *ctx)
{
  (void *)ctx;
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
  
  for (M_SoundNode *n = ctx->sounds_first; n != 0; n = n->list_next) {
    M_Sound *sound = &n->v;
    S_Source *source = &sound->source;
    
    if (sound->active) {
      // @Note: M_Sound's playback speed would modify play count.
      s_source_update(source, play_count);
    }
    
    if (source->pos >= source->count) {
      if (sound->loop) {
        source->pos = 0;
      }
      else {
        sound->active = 0;
      }
    }
  }
}

function void 
m_mix(M_Context *ctx)
{
  U64 write_count = s_get_write_count();
  F32 *buff = s_get_sample_buffer();
  
  U32 bytes_per_sample = s_win32_backend->sample_buff_fmt->nBlockAlign; 
  MemorySet(buff, 0, write_count * bytes_per_sample);
  
  for (M_SoundNode *n = ctx->sounds_first; n != 0; n = n->list_next) {
    M_Sound *sound = &n->v;
    S_Source *source = &sound->source;
    
    s_source_mix(source, buff, write_count, sound->volume, sound->pan, 1);
  }
}
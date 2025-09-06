#pragma once

struct M_Sound {
  S_Source source;
  
  B32 loop;   // nonzero for loop; 0 for no loop
  B32 stopped;; 
  F32 volume; // [0, 1]
  
  F32 pan;    // [-1,1]
  F32 pitch; 
};

struct M_SoundNode {
  M_SoundNode *next;
  M_Sound v;
};

struct M_Context {
  Arena *arena;
  
  M_SoundNode *first;
  M_SoundNode *last;
  M_SoundNode *free; // @Note: Reserved when I want to be able to delete sounds
  S32 sounds_count;
};

// Note: sound backend needs to be initialized before calling any of these.

function M_Context *m_ctx_alloc(void);
function void m_ctx_release(M_Context *ctx);

// @Note: Sounds are not dynamically allocated -- we're just returning pointer to a 
// sound node's value.
function M_Sound *m_sound_from_file(M_Context *ctx, String8 path);
function void m_sound_play(M_Sound *sound);
function void m_sound_pause(M_Sound *sound);
function void m_sound_stop(M_Sound *sound);

function void m_sound_set_loop(M_Sound *sound, B32 loop);
function void m_sound_set_pan(M_Sound *sound, F32 pan);
function void m_sound_set_pitch(M_Sound *sound, F32 pitch);

function void m_begin_frame(M_Context *ctx);
function void m_end_frame(M_Context *ctx);
function void m_process(M_Context *ctx);
function void m_mix(M_Context *ctx);
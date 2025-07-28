#pragma once

enum S_SoundFormat {
};

struct S_Sound {
  S16 *samples;
  U64 pos;
  U64 count; 
  S_SoundFormat fmt; 
};

function void s_init(U32 sample_rate, U32 channel_count, U32 channel_mask);
function void s_release(void);

function F32 *s_get_sample_buffer(void);
function U64 s_get_write_count(void);
function U64 s_get_play_count(void);

function void s_lock_buffer(void);
function void s_unlock_buffer(void);
function void s_advance_buffer(U64 sample_count);

function S_Sound s_sound_load(String8 path, U32 sample_rate);
function void s_sound_update(S_Sound *sound, U64 sample_count);
function void s_sound_mix(S_Sound *sound, F32 *out_samples, U64 out_sample_count, F32 volume); 
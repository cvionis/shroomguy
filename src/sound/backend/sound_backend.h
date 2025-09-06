#pragma once

enum S_ChannelMask {
};

struct S_Source {
  S16 *samples;
  U64 pos;
  U64 count; 
  // @Todo: Sample rate?
};

function void s_init(U32 sample_rate, U32 channel_count, U32 channel_mask);
function void s_release(void);

function F32 *s_get_sample_buffer(void);
function U64 s_get_write_count(void);
function U64 s_get_play_count(void);

function void s_lock_buffer(void);
function void s_unlock_buffer(void);
function void s_advance_buffer(U64 sample_count);

function S_Source s_source_load(String8 path, U32 sample_rate);
function void s_source_update(S_Source *source, U64 sample_count);
function void s_source_mix(S_Source *source, F32 *out_samples, U64 out_sample_count, F32 volume, F32 pan, F32 pitch); 
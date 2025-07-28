#pragma once

struct S_Effect {
  S_Effect *table_next;
  S_Effect *list_next; 
  
  S_Sound *sound;
  
  U32 pos;
  B32 loop;
  B32 paused; 
  F32 volume;
  
  F32 pan;           // [-1,1]
  F32 pitch; 
  F32 playback_rate; 
  
  F32 fadein_speed;  // [0, 1]
  F32 fadein_start;  // [0, 1]
  F32 fadein_end;    // [0, 1]
  
  F32 fadeout_speed; // [0, 1]
  F32 fadeout_start; // [0, 1]
  F32 fadeout_end;   // [0, 1]
};

struct S_Key {
  U64 v; 
};

struct S_EffectSlot {
  S_Key key; 
  S_Effect *first;
  S_Effect *last; 
};

struct S_Player {
  Arena *arena; 
  
  S_EffectSlot *effect_table;
  
  S_Effect *effect_list_first;
  S_Effect *effect_list_last; 
};

//
// Sound player creation/destruction
//

function S_Player *s_player_alloc(Arena *arena); 
function void s_player_release(S_Player **player);

//
// Sound effect creation/destruction
//

function S_Effect *s_effect_create(S_Player *player, String8 name, void *data, U64 size);
function void s_effect_delete(S_Player *player, String8 name);
function S_Effect *s_effect_load(S_Player *player, String8 name);

//
// Sound effects: basic playback-state-setting functions
//

function void s_effect_play(S_Player *player, String8 name);
function void s_effect_pause(S_Player *player, String8 name);
function void s_effect_toggle(S_Player *player, String8 name);
function void s_effect_loop(S_Player *player, String8 name);
function void s_effect_set_volume(S_Player *player, String8 name, F32 volume);
function void s_effect_set_pos(S_Player *player, String8 name, F32 seconds);
function void s_effect_advance(S_Player *player, String8 name, U32 seconds);

//
// Sound effects: basic effects
//

function void s_effect_set_pan(S_Player *player, String8 name, F32 pan);
function void s_effect_set_playback_rate(S_Player *player, String8 name, F32 rate);
function void s_effect_set_fadein(S_Player *player, String8 name, 
                                  U32 start, U32 end, U32 speed);
function void s_effect_set_fadeout(S_Player *player, String8 name, 
                                   U32 start, U32 end, U32 speed);
function void s_effect_shift_pitch(S_Player *player, String8 name, F32 delta);

//
// Sound effects: temporal effects/transitions
//

function void s_effect_pan_transition(S_Player *player, String8 name, 
                                      U32 start_pan, U32 end_pan, U32 seconds);
function void s_effect_pitch_transition(S_Player *player, String8 name, 
                                        U32 start_delta, U32 end_delta, U32 seconds);
function void s_effect_volume_transition(S_Player *player, String8 name, 
                                         U32 start_volume, U32 end_volume, U32 seconds);

//
// Direct version of the sound effect API that uses a pointer to the effect instead 
// of taking a name and looking up the effect in a sound player's table. These 
// are internally used by the named effects version's API.
//

function void s_effect_play_impl(S_Effect *effect);
function void s_effect_pause_impl(S_Effect *effect);
function void s_effect_toggle_impl(S_Effect *effect);
function void s_effect_loop_impl(S_Effect *effect);
function void s_effect_set_volume_impl(S_Effect *effect, F32 volume);
function void s_effect_set_pos_impl(S_Effect *effect, F32 seconds);
function void s_effect_advance_impl(S_Effect *effect, U32 seconds);

function void s_effect_set_pan_impl(S_Effect *effect, F32 pan);
function void s_effect_set_playback_rate_impl(S_Effect *effect, F32 rate);
function void s_effect_set_fadein_impl(S_Effect *effect, U32 start, U32 end, U32 speed);
function void s_effect_set_fadeout_impl(S_Effect *effect, U32 start, U32 end, U32 speed);
function void s_effect_shift_pitch_impl(S_Effect *effect, F32 delta);

function void s_effect_pan_transition_impl(S_Effect *effect, 
                                           U32 start_pan, U32 end_pan, U32 seconds);
function void s_effect_pitch_transition_impl(S_Effect *effect, 
                                             U32 start_delta, U32 end_delta, U32 seconds);
function void s_effect_volume_transition_impl(S_Effect *effect, 
                                              U32 start_volume, U32 end_volume, U32 seconds);


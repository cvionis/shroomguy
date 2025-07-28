function void
s_sound_update(S_Sound *sound, U64 sample_count)
{
  sound->pos += sample_count;
  sound->pos = Min(sound->pos, sound->count);
}

function void
s_sound_mix(S_Sound *sound, F32 *out_samples, U64 out_sample_count, F32 volume)
{
  S16 *in_samples = sound->samples;
  U64 in_count = sound->count;
  U64 in_pos = sound->pos;
  
  for (U64 sample_idx = 0; sample_idx < out_sample_count; sample_idx += 1) {
    if (in_pos >= in_count) {
      break;
    }
    S16 sample_s16 = in_samples[in_pos];
    F32 sample_f32 = sample_s16 * (1.f / 32768.f);
    in_pos += 1;
    
    out_samples[0] += volume * sample_f32;
    out_samples[1] += volume * sample_f32;
    out_samples += 2;
  }
}
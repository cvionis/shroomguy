function void
s_source_update(S_Source *source, U64 sample_count)
{
  source->pos += sample_count;
  source->pos = Min(source->pos, source->count);
}

function void
s_source_mix(S_Source *source, F32 *out_samples, U64 out_sample_count, F32 volume,
             F32 pan, F32 pitch)
{
  (void)pitch;
  
  S16 *in_samples = source->samples;
  U64 in_count = source->count;
  U64 in_pos = source->pos;
  
  for (U64 sample_idx = 0; sample_idx < out_sample_count; sample_idx += 1) {
    if (in_pos >= in_count) {
      break;
    }
    S16 sample_s16 = in_samples[in_pos];
    F32 sample_f32 = sample_s16 * (1.f / 32768.f);
    in_pos += 1;
    
    F32 pan_theta = (pan + 1.f) * (PI_F32 * 0.25f);
    F32 pan_left  = cosf32(pan_theta);
    F32 pan_right = sinf32(pan_theta);
    
    out_samples[0] += volume * pan_left  * sample_f32;
    out_samples[1] += volume * pan_right * sample_f32;
    
    out_samples += 2;
  }
}
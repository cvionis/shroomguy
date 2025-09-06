function void
s_source_update(S_Source *source, U64 sample_count)
{
  source->pos += sample_count;
  source->pos = Min(source->pos, source->count);
}

function void
s_source_mix(S_Source *source, F32 *out_samples, U64 out_sample_count, F32 volume)
{
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
    
    // @Todo: Pan, which ranges from -1 to 1, would be applied here.
    out_samples[0] += volume * sample_f32;
    out_samples[1] += volume * sample_f32;
    out_samples += 2;
  }
}
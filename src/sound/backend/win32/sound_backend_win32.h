#pragma once

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <initguid.h>
#include <windows.h>
#include <objbase.h>
#include <uuids.h>
#include <avrt.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

struct S_Win32_Backend {
  Arena *arena; 
  
  //
  // Public state
  //
  
  // The amount by which to advance each source's playback position
  // every frame, determined by `thread_samples_submitted`
  U64 source_pos_adv; 
  
  // Sample buffer state
  void *sample_buff;
  U64 sample_buff_count; 
  WAVEFORMATEX *sample_buff_fmt; 
  
  //
  // Internal state
  //
  
  // Audio thread state & synchronization
  HANDLE thread;
  HANDLE thread_run_event;
  LONG thread_stop; 
  LONG lock;
  B32 first_lock;
  U32 thread_samples_submitted;
  
  // WASAPI state
  IAudioClient *client;
  U32 out_buff_size; // in bytes 
  
  // Ringbuffer state
  U8 *rb_view1;
  U8 *rb_view2; 
  U32 rb_size; 
  
  // RB data read by audio thread, written to by thread using the source API.
  // Safe read range:  [rb_read_off, rb_lock_off)
  // Safe write range: [write_off, read_off)
  
  volatile LONG rb_read_off;  // Offset from which to read from buffer 
  volatile LONG rb_lock_off;  // Offset up to where the buffer is being used
	volatile LONG rb_write_off; // Offset up to where the buffer is filled
};

global S_Win32_Backend *s_win32_backend;

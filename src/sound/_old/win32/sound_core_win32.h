#ifndef SOUND_CORE_WIN32_H
#define SOUND_CORE_WIN32_H

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 0)
#include <windows.h>
#include <audioclient.h>
#include <initguid.h>
#include <uuids.h>
#include <avrt.h>
#include <mmdeviceapi.h>
#include <objbase.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#pragma warning(pop)

#pragma comment(lib, "avrt")
#pragma comment(lib, "ole32")
#pragma comment(lib, "onecore")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfreadwrite")

// TODO: Most of this isn't really windows specific, except for 
// WAVEFORMATEX and IAudioClient. 

typedef struct S_Win32_State S_Win32_State; 
struct S_Win32_State
{
  Arena *arena; 

	// Describes the format of the sample buffer
	WAVEFORMATEX* sample_buff_fmt;

	// NOTE: These three values are to be written to only between 
  // lock_buffer()/unlock_buffer() calls

  // Ringbuffer for interleaved samples
	void *sample_buff;       
  // The size of the sample buffer in number of samples
	U32 samples_max; 
  // # of samples actually used for playback since previous lock_buffer() call
	U32 play_count;      

	IAudioClient* client;
	HANDLE event;
	HANDLE thread;
	U32 stop;
	U32 lock;
	U8 *buffer1;
	U8 *buffer2;
  // Output buffer size in bytes
	U32 out_size;            
  // Ringbuffer size, always power of 2
	U32 rb_size;             
  // How many samples are used from buffer
	U32 samples_used;        
  // True when BufferLock is used at least once
	B32 buffer_first_lock;     
  // Offset to read from buffer
	volatile S32 rb_read_off;  
  // Offset up to where the buffer is currently being used
	volatile S32 rb_lock_off;  
  // Offset up to where the buffer is filled
	volatile S32 rb_write_off; 
};

global S_Win32_State *s_win32_state; 

#endif // SOUND_CORE_WIN32_H

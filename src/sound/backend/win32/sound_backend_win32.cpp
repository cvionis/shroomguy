//
// Win32 implementation of the source backend API
//

#pragma comment(lib, "avrt")
#pragma comment(lib, "ole32")
#pragma comment(lib, "onecore")
#pragma comment (lib, "mfplat")
#pragma comment (lib, "mfreadwrite")

#define HR(stmt) Assert(SUCCEEDED(stmt))

// NOTE: Missing from Windows libs
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);
DEFINE_GUID(IID_IMMDeviceEnumerator,  0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
DEFINE_GUID(IID_IAudioClient,         0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
DEFINE_GUID(IID_IAudioClient3,        0x7ed4ee07, 0x8e67, 0x4cd4, 0x8c, 0x1a, 0x2b, 0x7a, 0x59, 0x87, 0xad, 0x42);
DEFINE_GUID(IID_IAudioRenderClient,   0xf294acfc, 0x3146, 0x4483, 0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2);

//
// Helpers
//

function void
s_win32_backend_lock(void)
{
	while (InterlockedCompareExchange(&s_win32_backend->lock, TRUE, FALSE) == TRUE) {
		LONG lock_must_differ_from = FALSE;
		WaitOnAddress(&s_win32_backend->lock, &lock_must_differ_from, 
                  sizeof(lock_must_differ_from), INFINITE);
	}
}

function void
s_win32_backend_unlock(void)
{
  InterlockedExchange(&s_win32_backend->lock, FALSE);
	WakeByAddressSingle(&s_win32_backend->lock);
}

//
// Audio thread
//

function DWORD CALLBACK
s_win32_audio_thread(LPVOID arg)
{
  S_Win32_Backend *backend = (S_Win32_Backend *)arg;
  
	DWORD task = 0;
	HANDLE handle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &task);
  Assert(handle);
  
	IAudioClient *client = backend->client;
  
	IAudioRenderClient *playback;
	HR(client->GetService(IID_IAudioRenderClient, (LPVOID *)&playback));
  
	// Get output audio buffer size in samples
	U32 out_size_samples;
	HR(client->GetBufferSize(&out_size_samples));
  
	// Start playback
	HR(client->Start());
  
	U32 bytes_per_sample = backend->sample_buff_fmt->nBlockAlign;
	U32 rb_mask = backend->rb_size - 1;
	U8 *input_buff = backend->rb_view1;
  
	while (WaitForSingleObject(backend->thread_run_event, INFINITE) == WAIT_OBJECT_0) {
		if (InterlockedExchange(&backend->thread_stop, FALSE)) {
			break;
		}
    
    // Get number of valid, to-be-read samples currently occupying the output buffer
		U32 padding_samples;
		HR(client->GetCurrentPadding(&padding_samples));
    
		// Get output audio buffer from WASAPI
		U8 *out_buff;
		U32 max_out_samples = out_size_samples - padding_samples;
		HR(playback->GetBuffer(max_out_samples, &out_buff));
    
		s_win32_backend_lock();
    
		U32 read_off  = backend->rb_read_off;
		U32 write_off = backend->rb_write_off;
    
		// Get # of samples available to read from input sample buffer (ringbuffer)
		U32 available_bytes = write_off - read_off;
		U32 available_samples = available_bytes / bytes_per_sample;
    
    // Use only as many samples as we can currently fit in the output buffer
		U32 samples_to_use = Min(available_samples, max_out_samples);
    
		// The range [read, lock) we can safely write to without worrying about 
    // other code overwriting while copying input to output
		U32 bytes_to_use = samples_to_use * bytes_per_sample;
		backend->rb_lock_off = read_off + bytes_to_use;
    
		// Will always submit required amount of samples, but if there's not enough to 
    // use, then submit silence
		U32 samples_submitted = samples_to_use ? samples_to_use : max_out_samples;
		DWORD flags = samples_to_use ? 0 : AUDCLNT_BUFFERFLAGS_SILENT;
    
		// Remember how many samples the thread most recently submitted
		backend->thread_samples_submitted += samples_submitted;
    
		s_win32_backend_unlock();
    
		// Copy input sample data from [read, lock) range to output buffer
		MemoryCopy(out_buff, input_buff + (read_off & rb_mask), bytes_to_use);
    
		// Advance read offset up to lock position, allowing writing to the latest 
    // [read, lock) interval we just read from
		InterlockedAdd(&backend->rb_read_off, bytes_to_use);
    
		// Submit output buffer to WASAPI
		HR(playback->ReleaseBuffer(samples_submitted, flags));
	}
  
	// Stop playback
	HR(client->Stop());
	playback->Release();
  
	AvRevertMmThreadCharacteristics(handle);
	return 0;
}

// NOTE: Temporary
function
DWORD RoundUpPow2(DWORD value)
{
	unsigned long index;
	_BitScanReverse(&index, value - 1);
	assert(index < 31);
	return 1U << (index + 1);
}

//
// Public API -- Windows-specific implementations
//

// TODO: Pass Win32 channel mask for now; use custom enum and convert eventually.
// TODO: Proper handling of errors
function void 
s_init(U32 sample_rate, U32 channel_count, U32 channel_mask)
{
  Arena *arena = arena_alloc_default();
  s_win32_backend = ArenaPushStruct(arena, S_Win32_Backend);
  s_win32_backend->arena = arena;
  
	// Initialize COM
	HR(CoInitializeEx(0, COINIT_APARTMENTTHREADED));
  
	// Enumerate through audio devices and get the default playback device
	IMMDeviceEnumerator *enumerator;
	HR(CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, 
                      IID_IMMDeviceEnumerator, (LPVOID *)&enumerator));
  
	IMMDevice *device;
	HR(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device));
	enumerator->Release();
  
	// Create an AudioClient interface for the device
	HR(device->Activate(IID_IAudioClient, CLSCTX_ALL, 0, (LPVOID *)&s_win32_backend->client));
	device->Release();
  
  // Define a mixing format for the AudioClient's audio stream
	WAVEFORMATEXTENSIBLE fmt_ex = {0};
  
  fmt_ex.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
  fmt_ex.Format.nChannels       = (WORD)channel_count;
  fmt_ex.Format.nSamplesPerSec  = (WORD)sample_rate;
  fmt_ex.Format.nAvgBytesPerSec = (DWORD)(sample_rate * channel_count * sizeof(F32));
  fmt_ex.Format.nBlockAlign     = (WORD)(channel_count * sizeof(F32));
  fmt_ex.Format.wBitsPerSample  = (WORD)(8 * sizeof(F32));
  fmt_ex.Format.cbSize          = sizeof(fmt_ex) - sizeof(fmt_ex.Format);
  
  fmt_ex.Samples.wValidBitsPerSample = 8 * sizeof(F32);
  fmt_ex.dwChannelMask = channel_mask;
  fmt_ex.SubFormat = MEDIASUBTYPE_IEEE_FLOAT;
  
  // Use the native mixing format if default arguments were passed
  WAVEFORMATEX *fmt;
  if (sample_rate == 0 || channel_count == 0 || channel_mask == 0) {
    HR(s_win32_backend->client->GetMixFormat(&fmt));
    s_win32_backend->sample_buff_fmt = fmt;
  }
  else {
    fmt = &fmt_ex.Format;
  }
  
  // Try to initialize client with newer functionality in Windows 10
  B32 newer_client_init = 0;
  IAudioClient3 *client3;
  
  if (SUCCEEDED(s_win32_backend->client->QueryInterface(IID_IAudioClient3, (LPVOID*)&client3))) {
    // Minimum buffer size will typically be 480 samples (10msec @ 48khz)
    // but it can be 128 samples (2.66 msec @ 48khz) if driver is properly installed.
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/s_win32_backend/
    // low-latency-s_win32_backend#measurement-tools 
    
    U32 default_period_samples;
    U32 fundamental_period_samples;
    U32 min_period_samples;
    U32 max_period_samples;
    
    HR(client3->GetSharedModeEnginePeriod(fmt, &default_period_samples, &fundamental_period_samples, 
                                          &min_period_samples, &max_period_samples));
    
    U32 flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
    if (SUCCEEDED(client3->InitializeSharedAudioStream(flags, min_period_samples, fmt, 0))) {
      newer_client_init = 1;
    }
    
    client3->Release();
  }
  
  if (!newer_client_init) {
    // Get duration for shared-mode streams (typically 480 samples, or 10msec @ 48khz)
    REFERENCE_TIME duration;
    HR(s_win32_backend->client->GetDevicePeriod(&duration, 0));
    
    // Initialize audio playback
    U32 flags = 
      AUDCLNT_STREAMFLAGS_EVENTCALLBACK | 
      AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | 
      AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
    
    HR(s_win32_backend->client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, duration, 0, fmt, 0));
  }
  
  if (fmt == &fmt_ex.Format) {
    HR(s_win32_backend->client->GetMixFormat(&fmt));
    s_win32_backend->sample_buff_fmt = fmt;
  }
  
  // Get the size of the output audio buffer
  U32 samples_max;
  HR(s_win32_backend->client->GetBufferSize(&samples_max));
  U32 bytes_per_sample = s_win32_backend->sample_buff_fmt->nBlockAlign; 
  s_win32_backend->out_buff_size = samples_max * bytes_per_sample;
  
  // Create an event that the audio thread will use to determine when to 
  // process the output buffer. 
  s_win32_backend->thread_run_event = CreateEventW(0, FALSE, FALSE, 0);
  HR(s_win32_backend->client->SetEventHandle(s_win32_backend->thread_run_event));
  
  // Create a magic ringbuffer for sample data
  
  // Determine sample ringbuffer size; either 64kb or 1 second worth of data, whichever
  // is larger. 
  U32 rb_size = Max(64*1024, s_win32_backend->sample_buff_fmt->nAvgBytesPerSec);
  rb_size = RoundUpPow2(rb_size);
  
  // Reserve virtual address placeholder that is double the size of the ringbuffer
  U8 *placeholder1 = (U8 *)VirtualAlloc2(0, 0, 2 * rb_size, 
                                         MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, 
                                         PAGE_NOACCESS, 0, 0);
  U8 *placeholder2 = placeholder1 + rb_size;
  
  // Split the allocated address space in half
  VirtualFree(placeholder1, rb_size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
  
  // Create a page-file-backed section for the ringbuffer
  HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 
                                      0, rb_size, 0);
  
  // Map the same section to both ringbuffer addresses
  void* view1 = MapViewOfFile3(section, 0, placeholder1, 0, rb_size, 
                               MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, 0, 0);
  void* view2 = MapViewOfFile3(section, 0, placeholder2, 0, rb_size, 
                               MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, 0, 0);
  Assert(view1 && view2);
  
  s_win32_backend->sample_buff = 0;
  s_win32_backend->sample_buff_count = 0;
  s_win32_backend->source_pos_adv = 0;
  s_win32_backend->rb_view1 = (U8 *)view1;
  s_win32_backend->rb_view2 = (U8 *)view2;
  s_win32_backend->rb_size = rb_size;
  s_win32_backend->thread_samples_submitted = 0;
  s_win32_backend->first_lock = 1;
  s_win32_backend->rb_read_off = 0;
  s_win32_backend->rb_lock_off = 0;
  s_win32_backend->rb_write_off = 0;
  
  // Kick off the audio thread
  InterlockedExchange(&s_win32_backend->thread_stop, 0);
  InterlockedExchange(&s_win32_backend->lock, 0);
  s_win32_backend->thread = CreateThread(0, 0, &s_win32_audio_thread, 
                                         s_win32_backend, 0, 0);
  
  // Actual memory will be freed only when it is unmapped
  VirtualFree(placeholder1, 0, MEM_RELEASE);
  VirtualFree(placeholder2, 0, MEM_RELEASE);
  CloseHandle(section);
}

function void
s_release(void)
{
  // Signal to the audio thread to stop
	InterlockedExchange(&s_win32_backend->thread_stop, TRUE);
	SetEvent(s_win32_backend->thread_run_event);
  
	// Wait for the audio thread to finish
	WaitForSingleObject(s_win32_backend->thread, INFINITE);
	CloseHandle(s_win32_backend->thread);
	CloseHandle(s_win32_backend->thread_run_event);
  
	// Release ringbuffer
	UnmapViewOfFileEx(s_win32_backend->rb_view1, 0);
	UnmapViewOfFileEx(s_win32_backend->rb_view2, 0);
  
	// Release the AudioClient interface 
	CoTaskMemFree(s_win32_backend->sample_buff_fmt);
	s_win32_backend->client->Release();
  
	// Uninitialize COM
	CoUninitialize();
}

//
// Sound creation/destruction
//

function S_Source 
s_source_load(String8 path, U32 sample_rate)
{
  S_Source source = {0};
  HR(MFStartup(MF_VERSION, MFSTARTUP_LITE));
  
  TempArena scratch = arena_scratch_begin(0,0);
  String16 path16 = str16_from_str8(scratch.arena, path);
  arena_scratch_end(scratch);
  
  IMFSourceReader* reader;
  HR(MFCreateSourceReaderFromURL((LPCWSTR)path16.data, 0, &reader));
  
  // Read only the first audio stream
  HR(reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE));
  HR(reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE));
  
  U32 channel_count = 1;
  
  WAVEFORMATEXTENSIBLE fmt = {0};
  fmt.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
  fmt.Format.nChannels = (WORD)channel_count;
  fmt.Format.nSamplesPerSec = (DWORD)sample_rate;
  fmt.Format.nAvgBytesPerSec = (DWORD)(sample_rate * channel_count * sizeof(S16));
  fmt.Format.nBlockAlign     = (WORD)(channel_count * sizeof(S16));
  fmt.Format.wBitsPerSample  = (WORD)(8 * sizeof(S16));
  fmt.Format.cbSize          = sizeof(fmt) - sizeof(fmt.Format);
  fmt.Samples.wValidBitsPerSample = 8 * sizeof(S16);
  fmt.dwChannelMask = SPEAKER_FRONT_CENTER;
  fmt.SubFormat = MEDIASUBTYPE_PCM;
  
  // Set output format
  IMFMediaType *media_type;
  HR(MFCreateMediaType(&media_type));
  HR(MFInitMediaTypeFromWaveFormatEx(media_type, &fmt.Format, sizeof(fmt)));
  HR(reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, media_type));
  media_type->Release();
  
  U64 used = 0;
  U64 capacity = 0;
  
  for (;;) {
    IMFSample *sample;
    DWORD flags = 0;
    
    HRESULT hr = reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 
                                    0, 0, &flags, 0, &sample);
    
    if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
      break;
    }
    
    Assert(flags == 0);
    
    IMFMediaBuffer *media_buffer;
    HR(sample->ConvertToContiguousBuffer(&media_buffer));
    
    U8 *sample_data;
    DWORD sample_data_size;
    HR(media_buffer->Lock(&sample_data, 0, &sample_data_size));
    
    // Resize buffer if needed
    U64 available = capacity - used;
    if (available < sample_data_size) {
      source.samples = (S16 *)realloc(source.samples, capacity += 64 * 1024);
    }
    MemoryCopy((U8 *)source.samples + used, sample_data, sample_data_size);
    used += sample_data_size;
    
    HR(media_buffer->Unlock());
    
    media_buffer->Release();
    sample->Release();
  }
  
  reader->Release();
  HR(MFShutdown());
  
  source.count = used / fmt.Format.nBlockAlign;
  source.pos = source.count;
  return source;
}

function void
s_lock_buffer(void)
{
  S_Win32_Backend *backend = s_win32_backend;
  
  U32 bytes_per_sample = backend->sample_buff_fmt->nBlockAlign;
  U32 rb_size = backend->rb_size;
  U32 out_size = backend->out_buff_size;
  
	s_win32_backend_lock();
  
  U32 read_off = backend->rb_read_off;
  U32 lock_off = backend->rb_lock_off;
  U32 write_off = backend->rb_write_off;
  
	// how many bytes are used in buffer by reader = [read, lock) range
  U32 used_size = lock_off - read_off;
  
	// make sure there are samples available for one wasapi buffer submission
	// so in case backend thread needs samples before UnlockBuffer is called, it can get some 
	if (used_size < out_size) {
		// how many bytes available in current buffer = [read, write) range
    U32 available_size = write_off - read_off;
    
		// if [read, lock) is smaller than outSize buffer, then increase lock to [read, read+outSize) range
    used_size = Min(out_size, available_size);
    lock_off = read_off + used_size;
		backend->rb_lock_off = lock_off;
	}
  
	// how many bytes can be written to buffer
  U32 write_size = rb_size - used_size;
  
	// reset write marker to beginning of lock offset (can start writing there)
	backend->rb_write_off = lock_off;
  
	// reset play sample count, use 0 for playCount when LockBuffer is called first time
	backend->source_pos_adv = backend->first_lock ? 0 : backend->thread_samples_submitted;
	backend->first_lock = 0;
	backend->thread_samples_submitted = 0;
  
	s_win32_backend_unlock();
  
	// buffer offset/size where to write
	// safe to write in [write, read) range, because reading happen in [read, lock) range (lock==write)
	backend->sample_buff = backend->rb_view1 + (lock_off & (rb_size - 1));
	backend->sample_buff_count = write_size / bytes_per_sample;
}

function void
s_unlock_buffer(void)
{
}

function void
s_advance_buffer(U64 sample_count)
{
  S_Win32_Backend *backend = s_win32_backend;
  
	U32 bytes_per_sample = backend->sample_buff_fmt->nBlockAlign;
  U64 written_samples = sample_count;
  U64 write_size = written_samples * bytes_per_sample;
  
	// advance write offset to allow reading new samples
	InterlockedAdd(&backend->rb_write_off, (LONG)write_size);
}

function F32 *
s_get_sample_buffer(void)
{
  // NOTE: Not sure if I should implictly zero.
  F32 *buff = (F32 *)s_win32_backend->sample_buff;
  return buff;
}

function U64
s_get_write_count(void)
{
  S_Win32_Backend *backend = s_win32_backend;
  U64 sample_rate = backend->sample_buff_fmt->nSamplesPerSec;
  U64 sample_count = backend->sample_buff_count;
  
  // write at least 100msec of samples into buffer (or whatever space available, whichever is smaller)
  // this is max amount of time you expect code will take until the next iteration of loop
  // if code will take more time then you'll hear discontinuity as buffer will be filled with silence
  U64 write_count = Min(sample_rate/10, sample_count);
  return write_count;
}

function U64 
s_get_play_count(void)
{
  U64 play_count = s_win32_backend->source_pos_adv;
  return play_count;
}
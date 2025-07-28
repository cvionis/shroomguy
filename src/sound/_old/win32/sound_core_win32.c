//
// Internal Helpers 
//

// NOTE: Temp
function U32 
round_up_pow2(U32 value)
{
	unsigned long idx;
	_BitScanReverse(&idx, value - 1);
	Assert(idx < 31);
	return 1U << (idx + 1);
}

function void 
s_win32_lock(volatile U32 *lock)
{
	while (InterlockedCompareExchange(lock, 1, 0) != 0) {
		U32 cmp = 0;
		WaitOnAddress(lock, &cmp, sizeof(cmp), INFINITE);
	}
}

function void
s_win32_unlock(volatile U32 *lock)
{
	InterlockedExchange(lock, 0);
	WakeByAddressSingle(lock);
}

//
// WASAPI audio thread entry point
//

function DWORD CALLBACK 
s_win32_audio_thread_entry(LPVOID arg)
{
	S_Win32_State *state = (S_Win32_State *)arg;

  HRESULT hr;
	DWORD task = 0;
	HANDLE handle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &task);
	Assert(handle);

	IAudioClient* client = state->client;

	IAudioRenderClient* playback;
	hr = IAudioClient_GetService(client, &IID_IAudioRenderClient, (LPVOID*)&playback);
  Assert(SUCCEEDED(hr));

	U32 samples_max;
	hr = IAudioClient_GetBufferSize(client, &samples_max);
  Assert(SUCCEEDED(hr));

	// Start audio playback
	hr = IAudioClient_Start(client);
  Assert(SUCCEEDED(hr));

	U32 bytes_per_sample = state->sample_buff_fmt->nBlockAlign;
	U32 rb_mask = state->rb_size - 1;
	U8* input = state->buffer1;

	while (WaitForSingleObject(state->event, INFINITE) == WAIT_OBJECT_0) {
		if (InterlockedExchange(&state->stop, 0)) {
			break;
		}

		U32 samples_padding;
		hr = IAudioClient_GetCurrentPadding(client, &samples_padding);

		// Get output buffer from WASAPI
		U8* output;
		U32 max_output_samples = samples_max - samples_padding;
		hr = IAudioRenderClient_GetBuffer(playback, max_output_samples, &output);
    Assert(SUCCEEDED(hr));

		s_win32_lock(&state->lock);

		U32 read_off  = state->rb_read_off;
		U32 write_off = state->rb_write_off;

		// # of bytes available to read from ringbuffer
		U32 available_bytes = write_off - read_off;

		// # of samples available
		U32 available_samples = available_bytes / bytes_per_sample;
		U32 samples_to_use = Min(available_samples, max_output_samples);

		// # of bytes to use
		U32 bytes_to_use = samples_to_use * bytes_per_sample;

		// Lock range [read, lock) that memcpy will read from below
		state->rb_lock_off = read_off + bytes_to_use;

		// Will always submit required amount of samples, but if there's not 
    // enough to use, then submit silence
		U32 sample_submit_count = samples_to_use ? samples_to_use : max_output_samples;
		U32 flags = samples_to_use ? 0 : AUDCLNT_BUFFERFLAGS_SILENT;

		// Remember how many samples are submitted
		state->samples_used += sample_submit_count;

		s_win32_unlock(&state->lock);

		// Copy samples to output buffer.
		// This is safe to do outside lock/unlock, because nobody will overwrite 
    // [read, lock) interval.
		MemoryCopy(output, input + (read_off & rb_mask), bytes_to_use);

		// Advance read offset up to lock position, allows writing to [read, lock) 
    // interval.
		InterlockedAdd(&state->rb_read_off, bytes_to_use);

		// Submit output buffer to WASAPI.
		hr = IAudioRenderClient_ReleaseBuffer(playback, sample_submit_count, flags);
    Assert(SUCCEEDED(hr));
	}

	// Stop audio playback
	hr = IAudioClient_Stop(client);
  Assert(SUCCEEDED(hr));
	IAudioRenderClient_Release(playback);

	AvRevertMmThreadCharacteristics(handle);
	return 0;
}

function void 
s_init(U32 sample_rate, U32 num_channels, U32 channel_mask)
{
  // Allocate the win32 audio context
  Arena *arena = ArenaAllocDefault();
  s_win32_state = ArenaPushStruct(arena, S_Win32_State);
  s_win32_state->arena = arena; 

	// Initialize COM
  HRESULT hr; 
	hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
  Assert(SUCCEEDED(hr));

	// Ennumerate through s_win32_state devices and get the default playback device
	IMMDeviceEnumerator *enumerator;
	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, 
    &IID_IMMDeviceEnumerator, (LPVOID*)&enumerator);
  Assert(SUCCEEDED(hr));

	IMMDevice* device;
	hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eConsole, 
    &device);
  Assert(SUCCEEDED(hr));
	IMMDeviceEnumerator_Release(enumerator);

	// Create an s_win32_state client for the device
	hr = IMMDevice_Activate(device, &IID_IAudioClient, CLSCTX_ALL, 0, 
    (LPVOID*)&s_win32_state->client);
  Assert(SUCCEEDED(hr));
	IMMDevice_Release(device);

	WAVEFORMATEXTENSIBLE fmt_ex = {
		.Format = {
			.wFormatTag      = WAVE_FORMAT_EXTENSIBLE,
			.nChannels       = (WORD)num_channels,
			.nSamplesPerSec  = (WORD)sample_rate,
			.nAvgBytesPerSec = (DWORD)(sample_rate * num_channels * sizeof(F32)),
			.nBlockAlign     = (WORD)(num_channels * sizeof(F32)),
			.wBitsPerSample  = (WORD)(8 * sizeof(F32)),
			.cbSize          = sizeof(fmt_ex) - sizeof(fmt_ex.Format),
		},
		.Samples.wValidBitsPerSample = 8 * sizeof(F32),
		.dwChannelMask = channel_mask,
		.SubFormat = MEDIASUBTYPE_IEEE_FLOAT,
	};

	WAVEFORMATEX* wfx;
	if (sample_rate == 0 || num_channels == 0 || channel_mask == 0) {
		// Use native mixing format
		hr = IAudioClient_GetMixFormat(s_win32_state->client, &wfx);
    Assert(SUCCEEDED(hr));
		s_win32_state->sample_buff_fmt = wfx;
	}
	else {
		// Use our format
		wfx = &fmt_ex.Format;
	}
	
	// Try to initialize client with newer functionality in Windows 10, 
  // no AUTOCONVERTPCM allowed
	B32 client_init = 0;
	IAudioClient3* client3;

	if (SUCCEEDED(IAudioClient_QueryInterface(s_win32_state->client, 
    &IID_IAudioClient3, (LPVOID*)&client3))) 
  {
		// Minimum buffer size will typically be 480 samples (10msec @ 48khz)
		// but it can be 128 samples (2.66 msec @ 48khz) if driver is properly installed
		// see bullet-point instructions here: 
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/s_win32_state/
    // low-latency-s_win32_state#measurement-tools - Martins
		U32 default_period_samples;
    U32 fundamental_period_samples;
    U32 min_period_samples;
    U32 max_period_samples;
		hr = IAudioClient3_GetSharedModeEnginePeriod(client3, wfx, 
      &default_period_samples, &fundamental_period_samples, 
      &min_period_samples, &max_period_samples);
    Assert(SUCCEEDED(hr));

		U32 flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
		if (SUCCEEDED(IAudioClient3_InitializeSharedAudioStream(client3, flags, 
      min_period_samples, wfx, 0)))
		{
			client_init = 1;
		}
		IAudioClient3_Release(client3);
	}

	if (!client_init) {
		// Get duration for shared-mode streams, this will typically be 480 samples 
    // (10msec @ 48khz)
		REFERENCE_TIME duration;
		hr = IAudioClient_GetDevicePeriod(s_win32_state->client, &duration, 0);
    Assert(SUCCEEDED(hr));

		// Initialize audio playback
		U32 flags = 
      AUDCLNT_STREAMFLAGS_EVENTCALLBACK | 
      AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | 
      AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;

		hr = IAudioClient_Initialize(s_win32_state->client, AUDCLNT_SHAREMODE_SHARED, 
      flags, duration, 0, wfx, 0);
    Assert(SUCCEEDED(hr));
	}

	if (wfx == &fmt_ex.Format) {
		hr = IAudioClient_GetMixFormat(s_win32_state->client, &wfx);
    Assert(SUCCEEDED(hr));
		s_win32_state->sample_buff_fmt = wfx;
	}

	U32 samples_max;
	hr = IAudioClient_GetBufferSize(s_win32_state->client, &samples_max);
  Assert(SUCCEEDED(hr));
	s_win32_state->out_size = samples_max * 
    s_win32_state->sample_buff_fmt->nBlockAlign;

	// Create an event handle that we can wait on in a spinlock
	s_win32_state->event = CreateEventW(0, FALSE, FALSE, 0);
	hr = IAudioClient_SetEventHandle(s_win32_state->client, s_win32_state->event);
  Assert(SUCCEEDED(hr));

	// Use at least 64KB or 1 second whichever is larger, and round upwards to pow2 
  // for ringbuffer
	U32 rb_size = round_up_pow2(Max(64*1024, 
    s_win32_state->sample_buff_fmt->nAvgBytesPerSec));

	// Reserve virtual address placeholder for 2x size for magic ringbuffer
	char* placeholder1 = VirtualAlloc2(0, 0, 2 * rb_size, 
    MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, 0, 0);
	char* placeholder2 = placeholder1 + rb_size;
	Assert(placeholder1);

	// Split allocated address space in half
	BOOL ok = VirtualFree(placeholder1, rb_size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
	Assert(ok);

	// Create page-file-backed section for buffer
	HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 
    rb_size, 0);
	Assert(section);

	// Map same section into both addresses
	void* view1 = MapViewOfFile3(section, 0, placeholder1, 0, rb_size, 
    MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, 0, 0);
	void* view2 = MapViewOfFile3(section, 0, placeholder2, 0, rb_size, 
    MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, 0, 0);
	Assert(view1 && view2);

	s_win32_state->sample_buff       = 0;
	s_win32_state->samples_max       = 0;
	s_win32_state->play_count        = 0;
	s_win32_state->buffer1           = view1;
	s_win32_state->buffer2           = view2;
	s_win32_state->rb_size           = rb_size;
	s_win32_state->samples_used      = 0;
	s_win32_state->buffer_first_lock = 1;
	s_win32_state->rb_read_off       = 0;
	s_win32_state->rb_lock_off       = 0;
	s_win32_state->rb_write_off      = 0;

  // Kick off the audio thread
	InterlockedExchange(&s_win32_state->stop, 0);
	InterlockedExchange(&s_win32_state->lock, 0);
	s_win32_state->thread = CreateThread(0, 0, &s_win32_audio_thread_entry, 
    s_win32_state, 0, 0);

	// Actual memory will be freed only when it is unmapped
	VirtualFree(placeholder1, 0, MEM_RELEASE);
	VirtualFree(placeholder2, 0, MEM_RELEASE);
	CloseHandle(section);
}

#pragma once

#if OS_WINDOWS
# include "sound/backend/win32/sound_backend_win32.h"
#else 
# error Sound backend not implemented on this platform!
#endif

#include "sound/backend/sound_backend.h"
#include "sound/sound_mixer.h"

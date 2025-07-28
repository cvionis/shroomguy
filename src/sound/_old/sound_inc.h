#ifndef SOUND_INC_H
#define SOUND_INC_H

#ifdef OS_WINDOWS
# include "sound/win32/sound_core_win32.h"
#else 
# error The sound layer has not been implemented for this platform! 
#endif

#endif // SOUND_INC_H

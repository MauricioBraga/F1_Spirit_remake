#ifndef __BRAIN_SDL_SOUND
#define __BRAIN_SDL_SOUND

#include <SDL3_mixer/SDL_mixer.h>

/* SDL3_mixer's new track API uses a 0.0-1.0 float gain instead of the old
   0-128 integer channel volume, but Sound_play()/Sound_play_ch()/
   Sound_music_volume() below still take the old 0-128 scale (see
   sound.cpp), and several other files still do volume arithmetic against
   MIX_MAX_VOLUME directly. */
#define MIX_MAX_VOLUME 128

/* SDL3_mixer dropped the classic Mix_Chunk / "channel index" model in
   favour of MIX_Audio (a loaded sound) played through a MIX_Track (a
   mixing slot, roughly equivalent to the old channel). SOUNDT keeps
   meaning "a loaded, ready-to-play sound" from the rest of the game's
   point of view - nothing outside sound.cpp needs to know it changed. */
typedef MIX_Audio * SOUNDT;

bool Sound_initialization(void);
int Sound_initialization(int nc, int nrc);
void Sound_release(void);

bool Sound_file_test(char *f1);

SOUNDT Sound_create_sound(char *file);
void Sound_delete_sound(SOUNDT s);
void Sound_play(SOUNDT s);
void Sound_play(SOUNDT s, int volume);
void Sound_play_ch(SOUNDT s, int channel);
void Sound_play_ch(SOUNDT s, int channel, int volume);

void Sound_create_music(char *f1, int times);
void Sound_release_music(void);
void Sound_pause_music(void);
void Sound_unpause_music(void);

void Sound_music_volume(int volume);

/* These functions are AGRESIVE! (i.e. they actually STOP the mixer device and restart it) */
void Stop_playback(void);
void Resume_playback(void);
int Resume_playback(int nc, int nr);

/* Small "old SDL_mixer channel API" compatibility wrappers - implemented in
   sound.cpp on top of the same track pool used by Sound_play_ch(). A few
   files still call these directly with a plain channel number instead of
   going through Sound_play_ch(). */
const char *Mix_GetError(void);
void Mix_HaltChannel(int channel);
int Mix_PlayChannel(int channel, SOUNDT sound, int loops);
int Mix_FadeInChannel(int channel, SOUNDT sound, int loops, int ms);
void Mix_FadeOutChannel(int channel, int ms);
void Mix_Pause(int channel);
void Mix_Resume(int channel);

/* Sets the playback speed/pitch of whatever is currently assigned to
   "channel" (1.0 = normal pitch). Replaces the old raw-PCM resampling
   effect that used to live in CarEngineSound.cpp. */
void Sound_set_channel_pitch(int channel, float ratio);

#endif

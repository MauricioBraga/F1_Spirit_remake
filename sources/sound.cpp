#include "compat/sdl3_compat.h"
#include <SDL3_mixer/SDL_mixer.h>
#include "sound.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "debug.h"

#ifdef KITSCHY_DEBUG_MEMORY
#include "debug_memorymanager.h"
#endif

/*
 * SDL3_mixer 3.x replaced the whole classic Mix_OpenAudio/Mix_PlayChannel/
 * Mix_Chunk/Mix_Music API with a new object model:
 *
 *   MIX_Mixer  - one real audio device
 *   MIX_Audio  - a loaded sound (what SOUNDT now is, see sound.h)
 *   MIX_Track  - a mixing slot that plays a MIX_Audio (the old "channel")
 *
 * The rest of the game (CCar.cpp, F1SpiritGame.cpp, state_race.cpp,
 * state_gameoptions.cpp, ...) still calls Sound_play()/Sound_play_ch()
 * plus a handful of *direct* Mix_HaltChannel()/Mix_FadeInChannel()/
 * Mix_FadeOutChannel()/Mix_Pause()/Mix_Resume() calls addressed by a plain
 * integer channel number (e.g. a fixed SFX_RAIN channel). To avoid
 * touching every one of those call sites, this file keeps a fixed pool of
 * MIX_Track objects indexed exactly like the old channels used to be, and
 * re-implements those few Mix_* names as small wrappers around the new
 * per-track API.
 */

static MIX_Mixer *mixer = 0;
static MIX_Track **channel_tracks = 0;
static int n_channels = 0;
static MIX_Track *music_track = 0;
static MIX_Audio *music_audio = 0;

bool sound_enabled = false;
int music_loops_pending = 0;

static MIX_Track *get_track(int channel)
{
	if (channel < 0 || channel >= n_channels)
		return 0;

	return channel_tracks[channel];
}

static void alloc_channels(int nc)
{
	int i;

	if (channel_tracks) {
		for (i = 0; i < n_channels; i++)
			MIX_DestroyTrack(channel_tracks[i]);

		delete[] channel_tracks;
	}

	n_channels = (nc > 0) ? nc : 16;
	channel_tracks = new MIX_Track * [n_channels];

	for (i = 0; i < n_channels; i++)
		channel_tracks[i] = MIX_CreateTrack(mixer);
}

/* find a track that isn't currently playing anything, for Sound_play()
   which (like the old Mix_PlayChannel(-1, ...)) doesn't care which
   channel it lands on. */
static int find_free_channel(void)
{
	static int rr = 0;
	int i;

	for (i = 0; i < n_channels; i++) {
		int c = (rr + i) % n_channels;
		if (!MIX_TrackPlaying(channel_tracks[c])) {
			rr = (c + 1) % n_channels;
			return c;
		}
	}

	rr = (rr + 1) % n_channels;
	return rr;
}

static void play_on_track(MIX_Track *track, MIX_Audio *audio, int loops)
{
	if (!track || !audio)
		return;

	MIX_SetTrackAudio(track, audio);
	MIX_SetTrackLoops(track, loops);
	MIX_PlayTrack(track, 0);
}

bool Sound_initialization(void)
{
	if (-1 == Sound_initialization(0, 0))
		return false;

	return true;
} /* Sound_initialization */


int Sound_initialization(int nc, int nrc)
{
	sound_enabled = true;

#ifdef F1SPIRIT_DEBUG_MESSAGES
	output_debug_message("Initializing SDL3_mixer.\n");
#endif

	if (!MIX_Init()) {
		sound_enabled = false;
#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("Unable to init SDL3_mixer: %s\n", SDL_GetError());
		output_debug_message("Running the game without audio.\n");
#endif
		return -1;
	}

	mixer = MIX_CreateMixerDevice(0, NULL);

	if (!mixer) {
		sound_enabled = false;
#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("Unable to open audio device: %s\n", SDL_GetError());
		output_debug_message("Running the game without audio.\n");
#endif
		return -1;
	}

	alloc_channels(nc > 0 ? nc : 16);
	(void)nrc; /* reserved channels have no equivalent concept any more: every
	              track in the pool is available to Sound_play()'s round robin. */

	music_track = MIX_CreateTrack(mixer);

	return n_channels;
} /* Sound_init */

void Sound_release(void)
{
	Sound_release_music();

	if (sound_enabled) {
		int i;

		if (music_track)
			MIX_DestroyTrack(music_track);
		music_track = 0;

		for (i = 0; i < n_channels; i++)
			MIX_DestroyTrack(channel_tracks[i]);

		delete[] channel_tracks;
		channel_tracks = 0;
		n_channels = 0;

		MIX_DestroyMixer(mixer);
		mixer = 0;

		MIX_Quit();
	}

	sound_enabled = false;
} /* Sound_release */


void Stop_playback(void)
{
	if (sound_enabled) {
		Sound_pause_music();
		Sound_release();
	}
} /* Stop_playback */

void Resume_playback(void)
{
	Resume_playback(0, 0);
} /* Resume_playback */


int Resume_playback(int nc, int nrc)
{
	int n = Sound_initialization(nc, nrc);

	if (n != -1 && music_audio) {
		music_track = MIX_CreateTrack(mixer);
		play_on_track(music_track, music_audio, music_loops_pending);
	}

	return n;
} /* Resume_playback */


/* a check to see if file is readable and greater than zero */
int file_check(char *fname)
{
	FILE *fp;

	if ((fp = fopen(fname, "r")) != NULL) {
		if (fseek(fp, 0L, SEEK_END) == 0 && ftell(fp) > 0) {
			fclose(fp);
			return true;
		}

#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("ERROR in file_check(): the file %s is corrupted.\n", fname);
#endif

		fclose(fp);

		exit(1);
	}

	return false;
} /* file_check */


static char *find_sound_file(char *file, const char *const *ext, int n_ext, char *name)
{
	int i;

	for (i = 0; i < n_ext; i++) {
		strcpy(name, file);
		strcat(name, ext[i]);

		if (file_check(name))
			return name;
	}

	return 0;
} /* find_sound_file */


SOUNDT Sound_create_sound(char *file)
{
	static const char *const ext[6] = {".ogg", ".wav", ".mp3", ".OGG", ".WAV", ".MP3"};
	char name[256];

	if (sound_enabled) {
		if (find_sound_file(file, ext, 6, name))
			return MIX_LoadAudio(mixer, name, true);

#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("ERROR in Sound_create_sound(): Could not load sound file: %s.(wav|ogg|mp3)\n", file);
#endif

		exit(1);
	}

	return 0;
} /* Sound_create_sound */


void Sound_delete_sound(SOUNDT s)
{
	if (sound_enabled && s)
		MIX_DestroyAudio(s);
} /* Sound_delete_sound */


void Sound_play(SOUNDT s)
{
	if (sound_enabled)
		play_on_track(channel_tracks[find_free_channel()], s, 0);
} /* Sound_play */


void Sound_play(SOUNDT s, int volume)
{
	if (sound_enabled) {
		MIX_Track *t = channel_tracks[find_free_channel()];
		play_on_track(t, s, 0);
		MIX_SetTrackGain(t, volume / 128.0f);
	}
} /* Sound_play */

void Sound_play_ch(SOUNDT s, int ch)
{
	if (sound_enabled && ch >= 0 && ch < n_channels)
		play_on_track(channel_tracks[ch], s, 0);
} /* Sound_play_ch */


void Sound_play_ch(SOUNDT s, int ch, int volume)
{
	if (sound_enabled && ch >= 0 && ch < n_channels) {
		MIX_Track *t = channel_tracks[ch];
		play_on_track(t, s, 0);
		MIX_SetTrackGain(t, volume / 128.0f);
	}
} /* Sound_play_ch */

MIX_Audio *Sound_create_stream(char *file)
{
	static const char *const ext[6] = {".ogg", ".wav", ".mp3", ".OGG", ".WAV", ".MP3"};
	char name[256];

	if (sound_enabled) {
		if (find_sound_file(file, ext, 6, name))
			return MIX_LoadAudio(mixer, name, false);

#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("ERROR in Sound_create_stream(): Could not load sound file: %s.(wav|ogg|mp3)\n", file);
#endif

		exit(1);
	}

	return 0;
} /* Sound_create_stream */


void Sound_create_music(char *f1, int loops)
{
	if (sound_enabled) {
		if (music_audio) {
			MIX_DestroyAudio(music_audio);
			music_audio = 0;
		}

		if (f1 != 0) {
			music_audio = Sound_create_stream(f1);
			music_loops_pending = loops;
			play_on_track(music_track, music_audio, loops);
		} else {
			music_audio = 0;
		}
	}
} /* Sound_create_music */


bool Sound_file_test(char *f1)
{
	static const char *const ext[6] = {".WAV", ".OGG", ".MP3", ".wav", ".ogg", ".mp3"};
	char name[256];

	if (sound_enabled)
		return find_sound_file(f1, ext, 6, name) != 0;

	return false;
} /* Sound_file_test */


void Sound_release_music(void)
{
	if (sound_enabled) {
		if (music_track)
			MIX_StopTrack(music_track, 0);

		if (music_audio != 0)
			MIX_DestroyAudio(music_audio);

		music_audio = 0;
	}
} /* Sound_release_music */


void Sound_pause_music(void)
{
	if (sound_enabled && music_track)
		MIX_PauseTrack(music_track);
} /* Sound_pause_music */


void Sound_unpause_music(void)
{
	if (sound_enabled && music_track)
		MIX_ResumeTrack(music_track);
} /* Sound_unpause_music */


void Sound_music_volume(int volume)
{
	if (volume < 0)
		volume = 0;

	if (volume > 127)
		volume = 127;

	if (sound_enabled && music_track)
		MIX_SetTrackGain(music_track, volume / 127.0f);
} /* Sound_music_volume */


/* ------------------------------------------------------------------ *
 * Small "channel index" compatibility wrappers.
 *
 * A few files (CCar.cpp, F1SpiritGame.cpp, state_gameoptions.cpp,
 * state_race.cpp) call the classic Mix_HaltChannel()/Mix_FadeInChannel()/
 * Mix_FadeOutChannel()/Mix_Pause()/Mix_Resume()/Mix_GetError() directly
 * with a plain channel number (e.g. a fixed SFX_RAIN channel), rather
 * than going through Sound_play_ch(). These give those call sites the
 * same names/signatures, backed by the same track pool used above.
 * (Declared in sound.h.)
 * ------------------------------------------------------------------ */

const char *Mix_GetError(void)
{
	return SDL_GetError();
}

void Mix_HaltChannel(int channel)
{
	MIX_Track *t = get_track(channel);
	if (t)
		MIX_StopTrack(t, 0);
}

int Mix_PlayChannel(int channel, SOUNDT sound, int loops)
{
	MIX_Track *t = get_track(channel);
	play_on_track(t, sound, loops);
	return channel;
}

int Mix_FadeInChannel(int channel, SOUNDT sound, int loops, int ms)
{
	MIX_Track *t = get_track(channel);

	if (t && sound) {
		MIX_SetTrackAudio(t, sound);
		MIX_SetTrackLoops(t, loops);

		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_FRAMES_NUMBER, MIX_TrackMSToFrames(t, ms));
		MIX_PlayTrack(t, props);
		SDL_DestroyProperties(props);
	}

	return channel;
}

void Mix_FadeOutChannel(int channel, int ms)
{
	MIX_Track *t = get_track(channel);
	if (t)
		MIX_StopTrack(t, MIX_TrackMSToFrames(t, ms));
}

void Mix_Pause(int channel)
{
	MIX_Track *t = get_track(channel);
	if (t)
		MIX_PauseTrack(t);
}

void Mix_Resume(int channel)
{
	MIX_Track *t = get_track(channel);
	if (t)
		MIX_ResumeTrack(t);
}

/* used only by the car engine sound; the manual PCM pitch-shifting effect
   that used to live in CarEngineSound.cpp is gone (see the comment there
   and in CCar.cpp) - MIX_SetTrackFrequencyRatio() now does that natively. */
void Sound_set_channel_pitch(int channel, float ratio)
{
	MIX_Track *t = get_track(channel);
	if (t)
		MIX_SetTrackFrequencyRatio(t, ratio);
}

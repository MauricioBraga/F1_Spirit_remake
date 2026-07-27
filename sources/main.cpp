#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "math.h"
#include "string.h"

#include "GL/gl.h"
#include "GL/glu.h"
#include "compat/sdl3_compat.h"
#include <SDL3_mixer/SDL_mixer.h>
#include "compat/SDL_net.h"

#include "F1Spirit.h"
#include "sound.h"
#include "List.h"
#include "2DCMC.h"
#include "auxiliar.h"
#include "GLTile.h"
#include "PlacedGLTile.h"
#include "RotatedGLTile.h"
#include "keyboardstate.h"
#include "2DCMC.h"
#include "RoadPiece.h"
#include "track.h"
#include "CPlayer.h"
#include "CCar.h"
#include "RacingCCar.h"
#include "PlayerCCar.h"
#include "EnemyCCar.h"
#include "GameParameters.h"
#include "ReplayInfo.h"
#include "F1SpiritGame.h"
#include "F1SpiritApp.h"
#include "F1Spirit-auxiliar.h"
#include "randomc.h"

#include "debug.h"


#ifdef KITSCHY_DEBUG_MEMORY
#include "debug_memorymanager.h"
#endif

/*      GLOBAL VARIABLES INITIALIZATION:       */

char *application_name = "F-1 Spirit";
int application_version = 0;
int SCREEN_X = 640;
int SCREEN_Y = 480;
int g_stencil_bits = 0;
int N_SFX_CHANNELS = 16;
int COLOUR_DEPTH = 32;
int MAX_CONNECTIONS = 16;
bool sound = true;
bool fullscreen = false;

/* See the call site in initialization() for why this depends on
   windowed vs. fullscreen. Called at startup and again every time
   fullscreen is toggled at runtime (Alt+Enter/Alt+F4/Alt+F - see the
   event loop in main()). */
void apply_swap_interval_for_mode(void)
{
	int wanted = fullscreen ? 0 : 1;

	if (!SDL_GL_SetSwapInterval(wanted)) {
#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("SDL_GL_SetSwapInterval(%i) failed: %s\n", wanted, SDL_GetError());
#endif
	} 
} /* apply_swap_interval_for_mode */
bool network = true;
int network_tcp_port = 32124;
int network_udp_port = 32125;
int LISTENING_TIME = 1;

TRanrotBGenerator *rg = 0;

/* Redrawing constant: */
const int REDRAWING_PERIOD = 20;

/* Frames per second counter: */
int frames_per_sec = 0;
int frames_per_sec_tmp = 0;
int init_time = 0;
bool show_fps = false;

/* F1Spirit console messages: */
bool show_console_msg = false;
char console_msg[80] = "";


/*      AUXILIAR FUNCTION DEFINITION:       */


/* The window and GL context are real objects in SDL3 (there is no more
   "the screen surface" the way SDL 1.2's SDL_SetVideoMode() returned).
   main.cpp owns them; F1SpiritApp.cpp swaps buffers on g_window. */
SDL_Window *g_window = 0;
static SDL_GLContext g_gl_context = 0;

SDL_Window *initialization(SDL_WindowFlags flags)
{
	SDL_Window *window;

	rg = new TRanrotBGenerator(0);

#ifdef F1SPIRIT_DEBUG_MESSAGES

	output_debug_message("Initializing SDL\n");
#endif

	if (!SDL_Init(SDL_INIT_VIDEO | (sound ? SDL_INIT_AUDIO : 0) | SDL_INIT_JOYSTICK)) {
#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("Video initialization failed: %s\n", SDL_GetError());
#endif

		return 0;
	} 

#ifdef F1SPIRIT_DEBUG_MESSAGES
	output_debug_message("SDL initialized\n");

#endif

#ifdef F1SPIRIT_DEBUG_MESSAGES
	output_debug_message("Setting OpenGL attributes\n");

#endif

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);

	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);

	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);

	/* SDL3 defaults SDL_GL_ALPHA_SIZE to 8 bits if left unset, so without
	   this the window's own backbuffer (not the game's textures - those
	   are a separate matter, already handled) gets an 8-bit alpha
	   channel by default. The game runs glEnable(GL_BLEND) globally for
	   the whole frame, so that alpha channel ends up with varying,
	   non-opaque values wherever anything semi-transparent was drawn.
	   This is a normal, non-layered window with no legitimate use for
	   per-pixel window transparency, so ask for zero alpha bits instead -
	   removing any ambiguity in how Windows composites this window in
	   windowed (DWM) mode vs. exclusive fullscreen, which bypasses the
	   compositor entirely and never had this ambiguity in the first
	   place. */
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	/* Windows' DWM compositor is known to apply its own gamma/color
	   handling to composited (windowed) legacy-OpenGL content, making it
	   look darker/washed out compared to exclusive fullscreen, which
	   bypasses the compositor and shows the raw framebuffer. Requesting
	   an sRGB-capable framebuffer lets DWM interpret our output correctly
	   instead of guessing. */
	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

	/* This game draws everything with legacy fixed-function OpenGL
	   (glMatrixMode/glLoadIdentity/gluPerspective/gluLookAt here, plus
	   glBegin/glVertex/glColor throughout GLTile.cpp and friends), which
	   only exists in the Compatibility Profile. SDL3's default profile is
	   platform/driver-dependent - without this, some drivers hand back a
	   Core Profile context, where none of the drawing calls below do
	   anything (silently), even though every non-GPU part of the game
	   keeps running normally. */
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

#ifdef F1SPIRIT_DEBUG_MESSAGES
	output_debug_message("OpenGL attributes set\n");

#endif

#ifdef F1SPIRIT_DEBUG_MESSAGES

	output_debug_message("Initializing video mode\n");

#endif

	flags = SDL_WINDOW_OPENGL | flags;

	window = SDL_CreateWindow(application_name, SCREEN_X, SCREEN_Y, flags);

	if (window == 0) {
#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("Window creation failed: %s\n", SDL_GetError());
#endif

		return 0;
	} 

	g_gl_context = SDL_GL_CreateContext(window);

	if (g_gl_context == 0) {
#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("GL context creation failed: %s\n", SDL_GetError());
#endif

		SDL_DestroyWindow(window);
		return 0;
	} 

#ifdef F1SPIRIT_DEBUG_MESSAGES
	output_debug_message("Video mode initialized\n");

	/* Diagnostics: find out what GL implementation we actually got. */
	output_debug_message("GL_VENDOR: %s\n", (const char *)glGetString(GL_VENDOR));
	output_debug_message("GL_RENDERER: %s\n", (const char *)glGetString(GL_RENDERER));
	output_debug_message("GL_VERSION: %s\n", (const char *)glGetString(GL_VERSION));
	output_debug_message("glGetError() right after context creation: %i\n", glGetError());

	{
		int db_value = -1, srgb_value = -1;
		SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &db_value);
		SDL_GL_GetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, &srgb_value);
		output_debug_message("Actual SDL_GL_DOUBLEBUFFER: %i, SDL_GL_FRAMEBUFFER_SRGB_CAPABLE: %i\n", db_value, srgb_value);
	}

	{
		int drawable_w = 0, drawable_h = 0;
		SDL_GetWindowSizeInPixels(window, &drawable_w, &drawable_h);
		output_debug_message("Window size (pixels): %ix%i (SCREEN_X/Y = %ix%i)\n", drawable_w, drawable_h, SCREEN_X, SCREEN_Y);
	}
#endif

	/* Empirically, on at least some Intel driver/Windows/DWM
	   combinations, this game's legacy fixed-function OpenGL rendering
	   wants different vsync behavior depending on windowed vs. exclusive
	   fullscreen: vsync ON is what makes windowed mode show anything at
	   all (with vsync OFF, windowed mode never presented a single
	   frame), while vsync OFF measurably reduces (though doesn't fully
	   eliminate) a duplicated-geometry artifact seen in fullscreen. This
	   also gets re-applied every time fullscreen is toggled at runtime. */
	apply_swap_interval_for_mode();

	{
		SDL_Surface *icon = SDL_LoadBMP("graphics/f1sicon.bmp");

		if (icon) {
			SDL_SetWindowIcon(window, icon);
			SDL_DestroySurface(icon);
		} 
	}

	SDL_HideCursor();

	/* Needed for the SDL_EVENT_TEXT_INPUT events the hiscore/race-result
	   name-entry text boxes rely on (see the event loop in main()). */
	SDL_StartTextInput(window);

	if (sound) {
#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("Initializing Audio\n");
#endif

		N_SFX_CHANNELS = Sound_initialization(N_SFX_CHANNELS, 0);

#ifdef F1SPIRIT_DEBUG_MESSAGES

		output_debug_message("Audio initialized\n");
#endif

	} 

	// Network:
#ifdef F1SPIRIT_DEBUG_MESSAGES
	output_debug_message("Initializing SDL_net...\n");

#endif

	if (SDLNet_Init() == -1) {
#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("Error initializing SDL_net: %s.\n", SDLNet_GetError());
#endif

		network = false;
	} else {
#ifdef F1SPIRIT_DEBUG_MESSAGES
		output_debug_message("SDL_net initialized.\n");
#endif

		network = true;
	} 

	/* SDL3 dropped SDL_EnableUNICODE(): text translation is now opt-in per
	   window via SDL_StartTextInput() (called above, right after
	   SDL_HideCursor()), and delivered through separate SDL_EVENT_TEXT_INPUT
	   events instead of a unicode field tacked onto key-down events -
	   handled in the event loop in main(), since state_menu.cpp and
	   state_race_result.cpp's name-entry text boxes read ks->unicode. */

	glGetIntegerv(GL_STENCIL_BITS, &g_stencil_bits);

#ifdef F1SPIRIT_DEBUG_MESSAGES
	output_debug_message("OpenGL stencil buffer bits: %i\n", g_stencil_bits);

#endif


	g_window = window;

	return window;
} /* initialization */


void finalization()
{
#ifdef F1SPIRIT_DEBUG_MESSAGES
	output_debug_message("Finalizing SDL\n");
#endif

	if (network) {
		SDLNet_Quit();
	} 

	delete rg;

	rg = 0;

	free_auxiliar_menu_surfaces();

	if (sound)
		Sound_release();

	if (g_gl_context) {
		SDL_GL_DestroyContext(g_gl_context);
		g_gl_context = 0;
	} 

	if (g_window) {
		SDL_DestroyWindow(g_window);
		g_window = 0;
	} 

	SDL_Quit();

#ifdef F1SPIRIT_DEBUG_MESSAGES

	output_debug_message("SDL finalized\n");

#endif

} /* finalization */



#ifdef _WIN32
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow)
{
#else
int main(int argc, char** argv) {
#endif

	SDL_Window *window;
	F1SpiritApp *game;
	KEYBOARDSTATE *k;

	int time, act_time;
	SDL_Event event;
	bool quit = false;
	bool need_to_redraw = true;

#ifdef F1SPIRIT_DEBUG_MESSAGES

	output_debug_message("Application started\n");
#endif

	window = initialization((fullscreen ? SDL_WINDOW_FULLSCREEN : 0));

	if (window == 0)
		return 0;

	k = new KEYBOARDSTATE();

	game = new F1SpiritApp();

	time = init_time = SDL_GetTicks();

	while (!quit) {
		while ( SDL_PollEvent( &event ) ) {
			switch ( event.type ) {
					/* Keyboard event */

				case SDL_EVENT_KEY_DOWN: {
					/* Tracks whether this keydown was already consumed as a
					   hotkey (Alt+Enter/Alt+F4/Alt+F/F10/F12/...), so it
					   doesn't also get recorded into k->keyevents and misread
					   by the game itself (e.g. a name-entry text box reading
					   the same Enter keypress as "confirm" right after it
					   was used to toggle fullscreen). */
					bool consumed_by_hotkey = false;

#ifdef __APPLE__

					if (event.key.key == SDLK_Q) {
						SDLMod modifiers;
						modifiers = SDL_GetModState();

						if ((modifiers &KMOD_META) != 0) {
							quit = true;
						}
					}

#else
					if (event.key.scancode == SDL_SCANCODE_F12) {
						quit = true;
					} 

#endif
					if (event.key.scancode == SDL_SCANCODE_F10) {
						game->save_configuration("f1spirit.cfg");
						game->load_configuration("f1spirit.cfg");
					} 

#ifdef _WIN32
					if (event.key.scancode == SDL_SCANCODE_F4) {
						SDLMod modifiers;

						modifiers = SDL_GetModState();

						if ((modifiers&KMOD_ALT) != 0)
							quit = true;
					} 

#endif
#ifdef __APPLE__
					if (event.key.key == SDLK_F) {
						SDLMod modifiers;

						modifiers = SDL_GetModState();

						if ((modifiers&KMOD_META) != 0) {
							// FIX: doesn't register the key event in the game when it is 
							// generated by toggling full screen.
							consumed_by_hotkey = true;
							k->keyboard[SDLK_F] = 1;
							
							/* Toggle FULLSCREEN mode: */
							if (fullscreen)
								fullscreen = false;
							else
								fullscreen = true;

							SDL_SetWindowFullscreen(window, fullscreen);

							apply_swap_interval_for_mode();

							/* Defensive: some drivers don't reliably restore
							   the previous windowed size/backbuffer on their
							   own after a fullscreen round trip, which can
							   leave stale content visible. Force it back to
							   the game's native size explicitly. */
							if (!fullscreen)
								SDL_SetWindowSize(window, SCREEN_X, SCREEN_Y);

							/* Defensive: explicitly flush both halves of the
							   double-buffered swap chain right after a
							   fullscreen/windowed transition. On some
							   drivers, the old (differently-sized)
							   backbuffer content can otherwise linger
							   composited behind the next few real frames
							   (seen as an oversized "ghost" of whatever was
							   on screen before the switch). */
							glClearColor(0, 0, 0, 0);
							glClear(GL_COLOR_BUFFER_BIT);
							SDL_GL_SwapWindow(window);
							glClear(GL_COLOR_BUFFER_BIT);
							SDL_GL_SwapWindow(window);

							reload_textures++;
						}
					}

#else
					if (event.key.key == SDLK_RETURN) {
						SDLMod modifiers;

						modifiers = SDL_GetModState();

						if ((modifiers&KMOD_ALT) != 0) {
							// FIX: doesn't register the ENTER key event when it is 
							// generated by pressing ALT + ENTER.
							// This prevent the game from toggling fullscreen 
							// and also registering the ENTER key event in the game as a command.
							consumed_by_hotkey = true;
							k->keyboard[SDLK_RETURN] = 1;

							/* Toggle FULLSCREEN mode: */
							if (fullscreen)
								fullscreen = false;
							else
								fullscreen = true;

							SDL_SetWindowFullscreen(window, fullscreen);

							apply_swap_interval_for_mode();

							/* Defensive: some drivers don't reliably restore
							   the previous windowed size/backbuffer on their
							   own after a fullscreen round trip, which can
							   leave stale content visible. Force it back to
							   the game's native size explicitly. */
							if (!fullscreen)
								SDL_SetWindowSize(window, SCREEN_X, SCREEN_Y);

							/* Defensive: explicitly flush both halves of the
							   double-buffered swap chain right after a
							   fullscreen/windowed transition. On some
							   drivers, the old (differently-sized)
							   backbuffer content can otherwise linger
							   composited behind the next few real frames
							   (seen as an oversized "ghost" of whatever was
							   on screen before the switch). */
							glClearColor(0, 0, 0, 0);
							glClear(GL_COLOR_BUFFER_BIT);
							SDL_GL_SwapWindow(window);
							glClear(GL_COLOR_BUFFER_BIT);
							SDL_GL_SwapWindow(window);

							reload_textures++;
						}
					}

#endif

					if (event.key.key == SDLK_F) {
						SDLMod modifiers;

						modifiers = SDL_GetModState();

						if ((modifiers&KMOD_ALT) != 0) {
							/* toggle FPS mode: */
							if (show_fps)
								show_fps = false;
							else
								show_fps = true;
						} 
					} 

					/* Keyboard event: SDL3's SDL_KeyboardEvent no longer nests
					   a "keysym" sub-struct (and dropped its unicode field
					   entirely - see the note in initialization() above), so
					   this now fills in our compat SDL_keysym by hand from the
					   flat event fields instead of struct-copying event.key.keysym. */
					if (!consumed_by_hotkey) {
						SDL_keysym *ks;

						ks = new SDL_keysym();

						ks->scancode = event.key.scancode;
						ks->sym = event.key.key;
						ks->unicode = 0;
						ks->mod = event.key.mod;

						k->keyevents.Add(ks);
					}

					break;
				}

					/* SDL3 delivers typed text (respecting keyboard layout,
					   dead keys, IME, etc.) through a separate event instead
					   of a unicode field on the key-down event (see the note
					   in initialization()). state_menu.cpp/state_race_result.cpp
					   only read ks->unicode for name-entry text boxes, so we
					   just need to get *a* value in there; taking the first
					   UTF-8 byte is enough for the plain ASCII initials this
					   game expects. */

				case SDL_EVENT_TEXT_INPUT: {
					SDL_keysym *tks;

					tks = new SDL_keysym();

					tks->scancode = 0;
					tks->sym = 0;
					tks->unicode = (Uint16)(unsigned char)event.text.text[0];
					tks->mod = 0;

					k->keyevents.Add(tks);

					break;
				}

					/* SDL_EVENT_QUIT event (window close) */

				case SDL_EVENT_QUIT:
					quit = true;

					break;
			} 
		} 

		act_time = SDL_GetTicks();

		if (act_time - time >= REDRAWING_PERIOD) {
			int max_frame_step = 10;
			/*
			   frames_per_sec_tmp+=1;
			   if ((act_time-init_time)>=1000) {
			    frames_per_sec=frames_per_sec_tmp;
			    frames_per_sec_tmp=0;
			    init_time=act_time;
			   } // if
			*/

			do {
				time += REDRAWING_PERIOD;

				if ((act_time - time) > 10*REDRAWING_PERIOD)
					time = act_time;

				/* cycle */
				k->cycle();

				if (!game->cycle(k))
					quit = true;

				need_to_redraw = true;

				k->keyevents.Delete();

				act_time = SDL_GetTicks();

				max_frame_step--;
			} while (act_time - time >= REDRAWING_PERIOD && max_frame_step > 0);

		} 

		/* Redraw */
		if (need_to_redraw) {
			game->draw();
			need_to_redraw = false;
			frames_per_sec_tmp += 1;
		} 

		if ((act_time - init_time) >= 1000) {
			frames_per_sec = frames_per_sec_tmp;
			frames_per_sec_tmp = 0;
			init_time = act_time;
		} 

		SDL_Delay(1);

	} 


	delete k;

	k = 0;

	delete game;

	game = 0;

	Stop_playback();

	finalization();

#ifdef F1SPIRIT_DEBUG_MESSAGES

	output_debug_message("Application finished\n");

	close_debug_messages();

#endif

	return 0;
} /* main */

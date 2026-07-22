#ifndef __F1SPIRIT_SDL3_COMPAT_H
#define __F1SPIRIT_SDL3_COMPAT_H

/*
 * F1 Spirit Remake - SDL 1.2 -> SDL3 compatibility shim.
 *
 * The original codebase was written against SDL 1.2's software-surface
 * API (SDL_CreateRGBSurface, SDL_SetAlpha, SDL_MapRGBA taking a raw
 * SDL_PixelFormat*, SDLKey/SDL_keysym, SDL_SetVideoMode, ...). SDL3 removed
 * or renamed most of these. Rather than rewrite every call site in every
 * file, this header re-creates the old, narrow subset of the API that this
 * project actually uses, implemented on top of real SDL3 calls.
 *
 * Include this header AFTER SDL3/SDL.h in every source file that used to
 * include "SDL.h" (a repo-wide sed pass takes care of that).
 */

#include <SDL3/SDL.h>
#include <string.h>

/* ---- legacy surface creation flags: meaningless in SDL3, kept as 0 ---- */
#ifndef SDL_SWSURFACE
#define SDL_SWSURFACE 0
#endif
#ifndef SDL_HWSURFACE
#define SDL_HWSURFACE 0
#endif
#ifndef SDL_HWPALETTE
#define SDL_HWPALETTE 0
#endif
#ifndef SDL_SRCALPHA
#define SDL_SRCALPHA 0x00010000
#endif
#ifndef SDL_FULLSCREEN
#define SDL_FULLSCREEN 0x80000000
#endif
#ifndef SDL_OPENGL
#define SDL_OPENGL 0x00000002
#endif

/* ---- legacy keyboard/keysym types ---- */
typedef SDL_Keycode SDLKey;
typedef SDL_Keymod  SDLMod;

#undef KMOD_ALT
#define KMOD_ALT  SDL_KMOD_ALT
#define KMOD_META SDL_KMOD_GUI

/* SDL3 dropped the keysym struct (and unicode translation altogether, in
   favour of text-input events). We keep the shape around because replay
   files (.rpl) and the input-remapping code serialize it. */
struct SDL_keysym {
	int scancode;
	SDLKey sym;
	Uint16 unicode;
	SDLMod mod;
};

/* SDL3 has no fixed "last keycode" - use the scancode table size, which is
   what the code actually indexes the keyboard-state array with. */
#define SDLK_LAST SDL_SCANCODE_COUNT

/* ---- SDL_CreateRGBSurface (removed in SDL3) ---- */
static inline SDL_Surface *SDL_CreateRGBSurface(Uint32 /*flags*/, int width, int height, int depth,
        Uint32 rmask, Uint32 gmask, Uint32 bmask, Uint32 amask)
{
	SDL_PixelFormat fmt = SDL_GetPixelFormatForMasks(depth, rmask, gmask, bmask, amask);

	if (fmt == SDL_PIXELFORMAT_UNKNOWN)
		fmt = (depth == 32) ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24;

	return SDL_CreateSurface(width, height, fmt);
}

/* ---- SDL_FillRect / SDL_BlitSurface: renamed / unchanged ---- */
#undef SDL_FillRect
#define SDL_FillRect SDL_FillSurfaceRect
#undef SDL_FreeSurface
#define SDL_FreeSurface SDL_DestroySurface
/* SDL_BlitSurface keeps the exact same name & signature in SDL3. */

/* ---- SDL_GetClipRect: renamed ---- */
#undef SDL_GetClipRect
#define SDL_GetClipRect SDL_GetSurfaceClipRect

/* ---- pixel format helpers ----
   In SDL 1.2, "surface->format" was a pointer to an SDL_PixelFormat struct
   fed straight into SDL_MapRGB/SDL_MapRGBA/SDL_GetRGBA. In SDL3,
   SDL_Surface::format is just an SDL_PixelFormat enum value, and the real
   SDL_MapRGB/SDL_MapRGBA/SDL_GetRGBA take a resolved SDL_PixelFormatDetails*
   + palette instead. Every call site in this codebase looks like
   SDL_MapRGBA(sfc->format, r, g, b, a) - i.e. still passes the (now-enum)
   "format" field positionally. These overloads keep that call syntax
   working unchanged by resolving the details on the fly and forwarding to
   the real SDL3 functions. Overload resolution disambiguates against the
   real SDL_MapRGB/SDL_MapRGBA/SDL_GetRGBA by argument count. */
static inline Uint32 SDL_MapRGB(SDL_PixelFormat fmt, Uint8 r, Uint8 g, Uint8 b)
{
	return SDL_MapRGB(SDL_GetPixelFormatDetails(fmt), NULL, r, g, b);
}

static inline Uint32 SDL_MapRGBA(SDL_PixelFormat fmt, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return SDL_MapRGBA(SDL_GetPixelFormatDetails(fmt), NULL, r, g, b, a);
}

static inline void SDL_GetRGBA(Uint32 pixel, SDL_PixelFormat fmt, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a)
{
	SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(fmt), NULL, r, g, b, a);
}

/* surface->format->BytesPerPixel used to be a direct field; now it must be
   resolved through the format details. */
static inline int SDL_PixelFormatBytesPerPixel(SDL_PixelFormat fmt)
{
	return SDL_GetPixelFormatDetails(fmt)->bytes_per_pixel;
}

/* ---- SDL_SetAlpha (removed in SDL3; split into blend mode + alpha mod) --- */
static inline int SDL_SetAlpha(SDL_Surface *surface, Uint32 flag, Uint8 alpha)
{
	SDL_SetSurfaceBlendMode(surface, (flag & SDL_SRCALPHA) ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
	SDL_SetSurfaceAlphaMod(surface, alpha);
	return 0;
}

/* ---- SDL_DisplayFormatAlpha: no "the display's format" concept without a
   software screen surface any more; normalize to RGBA32 for GL upload ---- */
static inline SDL_Surface *SDL_DisplayFormatAlpha(SDL_Surface *surface)
{
	return SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
}

/* ---- SDL_AudioDriverName(buf, size): SDL3 just returns a string ---- */
static inline char *SDL_AudioDriverName(char *buf, int maxlen)
{
	const char *name = SDL_GetCurrentAudioDriver();
	if (!name)
		name = "unknown";
	strncpy(buf, name, maxlen - 1);
	buf[maxlen - 1] = 0;
	return buf;
}

/* ---- Joystick enumeration: SDL3 replaced SDL_NumJoysticks()/SDL_JoystickOpen(index)
   with SDL_GetJoysticks()/SDL_OpenJoystick(SDL_JoystickID). We keep a tiny
   index-based wrapper so keyboardstate.cpp doesn't need restructuring. ---- */
static inline int SDL_NumJoysticks(void)
{
	int count = 0;
	SDL_JoystickID *ids = SDL_GetJoysticks(&count);
	if (ids)
		SDL_free(ids);
	return count;
}

static inline SDL_Joystick *SDL_JoystickOpen(int index)
{
	int count = 0;
	SDL_Joystick *joy = NULL;
	SDL_JoystickID *ids = SDL_GetJoysticks(&count);

	if (ids && index >= 0 && index < count)
		joy = SDL_OpenJoystick(ids[index]);

	if (ids)
		SDL_free(ids);

	return joy;
}

#undef SDL_JoystickClose
#define SDL_JoystickClose SDL_CloseJoystick
#undef SDL_JoystickGetAxis
#define SDL_JoystickGetAxis SDL_GetJoystickAxis
#undef SDL_JoystickGetButton
#define SDL_JoystickGetButton SDL_GetJoystickButton
#undef SDL_JoystickNumButtons
#define SDL_JoystickNumButtons(joy) SDL_GetNumJoystickButtons(joy)
#undef SDL_JoystickUpdate
#define SDL_JoystickUpdate() SDL_UpdateJoysticks()

template <typename A, typename B>
static inline auto min(A a, B b) -> decltype(a < b ? a : b)
{
	return (a < b) ? a : b;
}

template <typename A, typename B>
static inline auto max(A a, B b) -> decltype(a > b ? a : b)
{
	return (a > b) ? a : b;
}


#endif /* __F1SPIRIT_SDL3_COMPAT_H */

#include "assert.h"

#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"

#include "compat/sdl3_compat.h"

#include "List.h"

#include "keyboardstate.h"


#ifdef KITSCHY_DEBUG_MEMORY
#include "debug_memorymanager.h"
#endif

#define JOYSTICK_SIZE 12


KEYBOARDSTATE::KEYBOARDSTATE(void)
{
	int i;

	n_joysticks = SDL_NumJoysticks();

	if (n_joysticks > 0) {
		joysticks = new SDL_Joystick * [n_joysticks];

		for (i = 0;i < n_joysticks;i++)
			joysticks[i] = SDL_JoystickOpen(i);
	} else {
		joysticks = 0;
	} 

	k_size = SDLK_LAST + n_joysticks * JOYSTICK_SIZE;

	joystick_0_pos = SDLK_LAST;

	joystick_size = JOYSTICK_SIZE;

	keyboard = new unsigned char[k_size];

	old_keyboard = new unsigned char[k_size];

	for (i = 0;i < k_size;i++) {
		keyboard[i] = 0;
		old_keyboard[i] = 0;
	} 
} 


KEYBOARDSTATE::~KEYBOARDSTATE(void)
{
	int i;

	delete []keyboard;
	keyboard = 0;
	delete []old_keyboard;
	old_keyboard = 0;

	for (i = 0;i < n_joysticks;i++) {
		SDL_JoystickClose(joysticks[i]);
		joysticks[i] = 0;
	} 

	delete []joysticks;


} 

static SDL_Scancode classic_key_to_scancode(int classic_key)
{
	switch (classic_key) {
		case SDLK_UP: return SDL_SCANCODE_UP;
		case SDLK_DOWN: return SDL_SCANCODE_DOWN;
		case SDLK_LEFT: return SDL_SCANCODE_LEFT;
		case SDLK_RIGHT: return SDL_SCANCODE_RIGHT;
		case SDLK_PAGEUP: return SDL_SCANCODE_PAGEUP;
		case SDLK_PAGEDOWN: return SDL_SCANCODE_PAGEDOWN;
		case SDLK_F4: return SDL_SCANCODE_F4;
		case SDLK_F10: return SDL_SCANCODE_F10;
		case SDLK_F12: return SDL_SCANCODE_F12;
		case SDLK_LSHIFT: return SDL_SCANCODE_LSHIFT;
		case SDLK_LCTRL: return SDL_SCANCODE_LCTRL;
		default: return SDL_GetScancodeFromKey((SDL_Keycode)classic_key, NULL);
	}
}


void KEYBOARDSTATE::cycle(void)
{
	int i, j, v;
	const bool *k = SDL_GetKeyboardState(NULL);

	/* Update keyboard: */

	for (i = 0;i < SDLK_LAST;i++) {
		SDL_Scancode sc = classic_key_to_scancode(i);

		old_keyboard[i] = keyboard[i];
		keyboard[i] = (sc != SDL_SCANCODE_UNKNOWN && k[sc]) ? 1 : 0;
	} 

	/* (o resto do joystick continua exatamente igual) */


	if (n_joysticks > 0) {
		SDL_JoystickUpdate();

		/* Update joysticks: */

		for (i = 0;i < n_joysticks;i++) {
			for (j = 0;j < joystick_size;j++)
				old_keyboard[joystick_0_pos + i*joystick_size + j] = keyboard[joystick_0_pos + i * joystick_size + j];

			v = SDL_JoystickGetAxis(joysticks[i], 0);

			if (v < -16000)
				keyboard[joystick_0_pos + i*joystick_size] = 1;
			else
				keyboard[joystick_0_pos + i*joystick_size] = 0;

			if (v > 16000)
				keyboard[joystick_0_pos + i*joystick_size + 1] = 1;
			else
				keyboard[joystick_0_pos + i*joystick_size + 1] = 0;

			v = SDL_JoystickGetAxis(joysticks[i], 1);

			if (v < -16000)
				keyboard[joystick_0_pos + i*joystick_size + 2] = 1;
			else
				keyboard[joystick_0_pos + i*joystick_size + 2] = 0;

			if (v > 16000)
				keyboard[joystick_0_pos + i*joystick_size + 3] = 1;
			else
				keyboard[joystick_0_pos + i*joystick_size + 3] = 0;

			for (j = 0;j < 8 && j < SDL_JoystickNumButtons(joysticks[i]);j++) {
				keyboard[joystick_0_pos + i*joystick_size + 4 + j] = SDL_JoystickGetButton(joysticks[i], j);
			} 
		} 
	} 

} 


void KEYBOARDSTATE::copy(KEYBOARDSTATE *lvk)
{
	int i;
	List<SDL_keysym> l;
	SDL_keysym *ks, *ks2;

	LVK_ID = lvk->LVK_ID;

	for (i = 0;i < k_size && i < lvk->k_size;i++) {
		keyboard[i] = lvk->keyboard[i];
		old_keyboard[i] = lvk->old_keyboard[i];
	} 

	l.Instance(lvk->keyevents);

	l.Rewind();

	while (l.Iterate(ks)) {
		ks2 = new SDL_keysym;
		ks2->scancode = ks->scancode;
		ks2->sym = ks->sym;
		ks2->unicode = ks->unicode;
		ks2->mod = ks->mod;
		keyevents.Add(ks2);
	} 

} 


bool KEYBOARDSTATE::save(FILE *fp)
{
	int i;
	List<SDL_keysym> l;
	SDL_keysym *ks;

	fprintf(fp, "%i %i %i %i\n", n_joysticks, k_size, joystick_0_pos, joystick_size);

	for (i = 0;i < k_size;i++) {
		if (keyboard[i] ||
		        old_keyboard[i]) {
			fprintf(fp, "K %i %i %i\n", i, int(keyboard[i]), int(old_keyboard[i]));
		} 
	} 

	fprintf(fp, "%i\n", keyevents.Length());

	l.Instance(keyevents);

	l.Rewind();

	while (l.Iterate(ks)) {
		fprintf(fp, "%i %i %i %i\n", int(ks->scancode), int(ks->sym), int(ks->unicode), int(ks->mod));
	} 

	return true;
} 


bool KEYBOARDSTATE::load(FILE *fp)
{
	int i, j, k, l, m, n;
	SDL_keysym *ks;
	char tmp[32];

	if (4 != fscanf(fp, "%i %i %i %i", &n_joysticks, &k_size, &joystick_0_pos, &joystick_size))
		return false;

	for (i = 0;i < k_size;i++) {
		keyboard[i] = 0;
		old_keyboard[i] = 0;
	} 

	keyevents.Delete();

	do {
		if (1 != fscanf(fp, "%s", tmp))
			return false;

		if (strcmp(tmp, "K") == 0) {
			if (1 != fscanf(fp, "%i", &i))
				return false;

			if (2 != fscanf(fp, "%i %i", &j, &k))
				return false;

			keyboard[i] = j;

			old_keyboard[i] = k;
		} else {
			if (1 != sscanf(tmp, "%i", &i))
				return false;

			for (j = 0;j < i;j++) {
				ks = new SDL_keysym();

				if (4 != fscanf(fp, "%i %i %i %i", &k, &l, &m, &n))
					return false;

				ks->scancode = k;

				ks->sym = (SDLKey)l;

				ks->unicode = m;

				ks->mod = (SDLMod)n;

				keyevents.Add(ks);
			} 
		} 

	} while (strcmp(tmp, "K") == 0);

	return true;
} 

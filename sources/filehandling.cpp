#include "filehandling.h"

//  #ifndef _WIN32
#ifdef _WIN32
#include <direct.h>
#endif

// make (sub)directories including multiple subdirs
// int mkdirp(const char *fqfn, mode_t mode)
int mkdirp(const char *fqfn)
{
	char *t, str[STRLEN];

	struct stat stbuf;
	int len;

	t = (char *)fqfn;
	memset(str, '\0', STRLEN);

	while (*t) {
		if (*t == '/') {
			len = t - fqfn;

			if ((len < STRLEN) && (len > 0)) {
				strncpy(str, fqfn, len);

				// if (stat(str, &stbuf) != 0)
				//	if (mkdir(str, mode) != 0) return(-1);
				if (stat(str, &stbuf) != 0) {
#ifdef _WIN32
					if (_mkdir(str) != 0) return(-1);
#else
					if (mkdir(str, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) return(-1);
#endif
				}
			}
		}

		t++;
	}

	return(0);
}

// #endif

// new fopen()
FILE *f1open(const char *f, const char *m, const enum filetype t)
{
#ifdef _WIN32
	// USERDATA writes/appends need their subdirectory (players/, replays/,
	// highscores/) to already exist - fopen() never creates it. The
	// original distribution shipped those as pre-existing empty folders;
	// a fresh checkout/build tree doesn't have them.
	if (t == USERDATA && (strchr(m, 'w') != 0 || strchr(m, 'a') != 0))
		mkdirp(f);
	return(fopen(f, m));
#else
	// *nix is a bitch ;)
	char fname[STRLEN];

	switch (t) {

		case GAMEDATA:
			// gamedata is read-only
			return(fopen(f, m));
			break;

		case USERDATA:
			// userdata is put in $HOME/.GAMENAME/
			snprintf(fname, STRLEN - 1, "%s/.%s/%s", getenv("HOME"), GAMENAME, f);
			// create subdirs if they don't exist
			// mkdirp(fname, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			mkdirp(fname);
			// open file
			return(fopen(fname, m));
			break;
	}

#endif

	// should not be reached
	return(NULL);
}


/*
 * ux_file.c - File handling functions.
 *
 * This file is part of Frotz.
 *
 * Frotz is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Frotz is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/*
 * The objective here is to have ALL attempts to open, close, read, and
 * write files go through wrapper functions in this file.  Hopefully this
 * will make it easier to do things like support zlib.
 *
 */

#define __UNIX_PORT_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include <unistd.h>
#include <ctype.h>


/*
 * os_path_open
 *
 * Open a file in the current directory.  If this fails, then search the
 * directories in the ZCODE_PATH environmental variable.  If that's not
 * defined, search INFOCOM_PATH.
 *
 */

FILE *os_path_open(const char *name, const char *mode)
{
	FILE *fp;
	char buf[FILENAME_MAX + 1];
	char *p;

	/* Let's see if the file is in the currect directory */
	/* or if the user gave us a full path. */
	if ((fp = fopen(name, mode))) {
		return fp;
	}

	/* If zcodepath is defined in a config file, check that path. */
	/* If we find the file a match in that path, great. */
	/* Otherwise, check some environmental variables. */
	if (f_setup.zcode_path != NULL) {
		if ((fp = pathopen(name, f_setup.zcode_path, mode, buf)) != NULL) {
			strncpy(f_setup.story_name, buf, FILENAME_MAX);
			return fp;
		}
	}

	if ( (p = getenv(PATH1) ) == NULL)
		p = getenv(PATH2);

	if (p != NULL) {
		fp = pathopen(name, p, mode, buf);
		strncpy(f_setup.story_name, buf, FILENAME_MAX);
		return fp;
	}
	return NULL;	/* give up */
} /* os_path_open() */

/*
 * pathopen
 *
 * Given a standard Unix-style path and a filename, search the path for
 * that file.  If found, return a pointer to that file and put the full
 * path where the file was found in fullname.
 *
 */

FILE *pathopen(const char *name, const char *p, const char *mode, char *fullname)
{
	FILE *fp;
	char buf[FILENAME_MAX + 1];
	char *bp, lastch;

	lastch = 'a';	/* makes compiler shut up */

	while (*p) {
		bp = buf;
		while (*p && *p != PATHSEP)
			lastch = *bp++ = *p++;
		if (lastch != DIRSEP)
			*bp++ = DIRSEP;
		strcpy(bp, name);
		if ((fp = fopen(buf, mode)) != NULL) {
			strncpy(fullname, buf, FILENAME_MAX);
			return fp;
		}
		if (*p)
			p++;
	}
	return NULL;
} /* FILE *pathopen() */

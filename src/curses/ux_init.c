/*
 * ux_init.c - Unix interface, initialisation
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or visit http://www.fsf.org/
 */

#define __UNIX_PORT_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <libgen.h>

#ifdef USE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#ifndef WIN32
#include <termios.h>
#endif

#include "ux_frotz.h"
#include "ux_blorb.h"

f_setup_t f_setup;
u_setup_t u_setup;

static int getconfig(char *);
static int geterrmode(char *);
static int getcolor(char *);
static int getbool(char *);

/* static void sigwinch_handler(int); */
static void sigint_handler(int);
/* static void redraw(void); */


#define INFORMATION "\
An interpreter for all Infocom and other Z-Machine games.\n\
\n\
Syntax: frotz [options] story-file\n\
  -a   watch attribute setting    \t -O   watch object locating\n\
  -A   watch attribute testing    \t -p   plain ASCII output only\n\
  -b <colorname> background color \t -P   alter piracy opcode\n\
  -c # context lines              \t -q   quiet (disable sound effects)\n\
  -d   disable color              \t -r # right margin\n\
  -e   enable sound               \t -R <path> restricted read/write\n\
  -f <colorname> foreground color \t -s # random number seed value\n\
  -F   Force color mode           \t -S # transcript width\n\
  -h # screen height              \t -t   set Tandy bit\n\
  -i   ignore fatal errors        \t -u # slots for multiple undo\n\
  -l # left margin                \t -v   show version information\n\
  -L <file> load this save file   \t -w # screen width\n\
  -o   watch object movement      \t -x   expand abbreviations g/x/z\n"

/*
char stripped_story_name[FILENAME_MAX+1];
char semi_stripped_story_name[FILENAME_MAX+1];
*/

static int zgetopt (int, char **, const char *);
static int zoptind = 1;
static int zoptopt = 0;
static char *zoptarg = NULL;

static void	print_version(void);
static int	getconfig(char *);
static int	getbool(char *);
static int	getcolor(char *);
static int	geterrmode(char *);
/* static void	redraw(void); */
/* static FILE	*pathopen(const char *, const char *, const char *, char *); */


/*
 * os_warn
 *
 * Display a warning message and continue with the game.
 *
 */
void os_warn (const char *s, ...)
{
    if (u_setup.curses_active) {
	/* Solaris 2.6's cc complains if the below cast is missing */
	os_display_string((zchar *)"\n\n");
	os_beep(BEEP_HIGH);
	os_set_text_style(BOLDFACE_STYLE);
	os_display_string((zchar *)"Warning: ");
	os_set_text_style(0);
	os_display_string((zchar *)s);
	os_display_string((zchar *)"\n");
	new_line();
    }
}/* os_warn */

/*
 * os_fatal
 *
 * Display error message and exit program.
 *
 */
void os_fatal (const char *s, ...)
{
    if (u_setup.curses_active) {
	/* Solaris 2.6's cc complains if the below cast is missing */
	os_display_string((zchar *)"\n\n");
	os_beep(BEEP_HIGH);
	os_set_text_style(BOLDFACE_STYLE);
	os_display_string((zchar *)"Fatal error: ");
	os_set_text_style(0);
	os_display_string((zchar *)s);
	os_display_string((zchar *)"\n");
	new_line();
	os_reset_screen();
	ux_blorb_stop();
	exit(1);
    }

    fputs ("\nFatal error: ", stderr);
    fputs (s, stderr);
    fputs ("\n\n", stderr);

    exit (1);
}/* os_fatal */

/* extern char script_name[]; */
/* extern char command_name[]; */
/* extern char save_name[];*/
/*extern char auxilary_name[];*/


/*
 * os_process_arguments
 *
 * Handle command line switches. Some variables may be set to activate
 * special features of Frotz:
 *
 *     option_attribute_assignment
 *     option_attribute_testing
 *     option_context_lines
 *     option_object_locating
 *     option_object_movement
 *     option_left_margin
 *     option_right_margin
 *     option_ignore_errors
 *     option_piracy
 *     option_undo_slots
 *     option_expand_abbreviations
 *     option_script_cols
 *
 * The global pointer "story_name" is set to the story file name.
 *
 *
 */
void os_process_arguments (int argc, char *argv[])
{
    int c;
    char *p = NULL;
// FIXME: put this back before committing merge fixes
//    char *blorb_ext = NULL;

    char *home;
    char configfile[FILENAME_MAX + 1];

#ifndef WIN32
    if ((getuid() == 0) || (geteuid() == 0)) {
	printf("I won't run as root!\n");
	exit(1);
    }
#endif

#ifdef WIN32
#define HOMEDIR "USERPROFILE"
#else
#define HOMEDIR "HOME"
#endif

    if ((home = getenv(HOMEDIR)) == NULL) {
	printf("Hard drive on fire!\n");
	exit(1);
    }


/*
 * It doesn't look like Frotz can reliably be resized given its current
 * screen-handling code.  While playing with Nitfol, I noticed that it
 * resized itself fairly reliably, even though the terminal looked rather
 * ugly to begin with.  Since Nitfol uses the Glk library for screen I/O,
 * I think there might be something in Glk that can make resizing easier.
 * Something to think about for later.
 *
 */

/*
    if (signal(SIGWINCH, SIG_IGN) != SIG_IGN)
	signal(SIGWINCH, sigwinch_handler);
*/

    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	signal(SIGINT, sigint_handler);


    if (signal(SIGTTIN, SIG_IGN) != SIG_IGN)
	signal(SIGTTIN, SIG_IGN);

    if (signal(SIGTTOU, SIG_IGN) != SIG_IGN)
	signal(SIGTTOU, SIG_IGN);

    /* First check for a "$HOME/.frotzrc". */
    /* If not found, look for CONFIG_DIR/frotz.conf */
    /* $HOME/.frotzrc overrides CONFIG_DIR/frotz.conf */

    strncpy(configfile, home, FILENAME_MAX);
    strncat(configfile, "/", 1);

    strncat(configfile, USER_CONFIG, strlen(USER_CONFIG));
    if (!getconfig(configfile)) {
        strncpy(configfile, CONFIG_DIR, FILENAME_MAX);
        strncat(configfile, "/", 1);    /* added by DJP */
        strncat(configfile, MASTER_CONFIG, FILENAME_MAX-10);
        getconfig(configfile);  /* we're not concerned if this fails */
    }

    /* Parse the options */
    do {
	c = zgetopt(argc, argv, "-aAb:c:def:Fh:il:oOpPqrR:s:S:tu:vw:W:xZ:");
	switch(c) {
	  case 'a': f_setup.attribute_assignment = 1; break;
	  case 'A': f_setup.attribute_testing = 1; break;
	  case 'b': u_setup.background_color = getcolor(zoptarg);
		u_setup.force_color = 1;
		u_setup.disable_color = 0;
		if ((u_setup.background_color < 2) ||
		    (u_setup.background_color > 9))
		  u_setup.background_color = -1;
		break;
	  case 'c': f_setup.context_lines = atoi(zoptarg); break;
	  case 'd': u_setup.disable_color = 1; break;
	  case 'e': f_setup.sound = 1; break;
	  case 'f': u_setup.foreground_color = getcolor(zoptarg);
		    u_setup.force_color = 1;
		    u_setup.disable_color = 0;
	            if ((u_setup.foreground_color < 2) ||
			(u_setup.foreground_color > 9))
			u_setup.foreground_color = -1;
		    break;
	  case 'F': u_setup.force_color = 1;
		    u_setup.disable_color = 0;
		    break;
          case 'h': u_setup.screen_height = atoi(zoptarg); break;
	  case 'i': f_setup.ignore_errors = 1; break;
	  case 'l': f_setup.left_margin = atoi(zoptarg); break;
	  case 'L': f_setup.restore_mode = 1;
		    f_setup.tmp_save_name = malloc(FILENAME_MAX * sizeof(char) + 1);
		    strncpy(f_setup.tmp_save_name, zoptarg, FILENAME_MAX);
		    break;
	  case 'o': f_setup.object_movement = 1; break;
	  case 'O': f_setup.object_locating = 1; break;
	  case 'p': u_setup.plain_ascii = 1; break;
	  case 'P': f_setup.piracy = 1; break;
	  case 'q': f_setup.sound = 0; break;
	  case 'r': f_setup.right_margin = atoi(zoptarg); break;
	  case 'R': f_setup.restricted_path = strndup(zoptarg, PATH_MAX); break;
	  case 's': u_setup.random_seed = atoi(zoptarg); break;
	  case 'S': f_setup.script_cols = atoi(zoptarg); break;
	  case 't': u_setup.tandy_bit = 1; break;
	  case 'u': f_setup.undo_slots = atoi(zoptarg); break;
	  case 'v': print_version(); exit(2); break;
	  case 'w': u_setup.screen_width = atoi(zoptarg); break;
	  case 'x': f_setup.expand_abbreviations = 1; break;
	  case 'Z': f_setup.err_report_mode = atoi(zoptarg);
		    if ((f_setup.err_report_mode < ERR_REPORT_NEVER) ||
			(f_setup.err_report_mode > ERR_REPORT_FATAL))
		      f_setup.err_report_mode = ERR_DEFAULT_REPORT_MODE;
		    break;
	}

    } while (c != EOF);

    if (zoptind != argc - 1) {
	printf("FROTZ V%s\t", VERSION);

#ifndef NO_SOUND
	printf("Audio output enabled.");
#else
	printf("Audio output disabled.");
#endif
	putchar('\n');

	puts (INFORMATION);
	printf ("\t-Z # error checking mode (default = %d)\n"
	    "\t     %d = don't report errors   %d = report first error\n"
	    "\t     %d = report all errors     %d = exit after any error\n\n",
	    ERR_DEFAULT_REPORT_MODE, ERR_REPORT_NEVER,
	    ERR_REPORT_ONCE, ERR_REPORT_ALWAYS,
	    ERR_REPORT_FATAL);
	exit (1);
    }

    /* This section is exceedingly messy and really can't be fixed
       without major changes all over the place.
     */

    /* Save the story file name */

    f_setup.story_file = strdup(argv[zoptind]);
    f_setup.story_name = strdup(basename(argv[zoptind]));

    /* Now strip off the extension. */
    p = strrchr(f_setup.story_name, '.');
    if ((p != NULL) &&
        ((strcmp(p,EXT_BLORB2) == 0) ||
         (strcmp(p,EXT_BLORB3) == 0) ||
         (strcmp(p,EXT_BLORB4) == 0) ) ) {
//        blorb_ext = strdup(p);
    }
    else
//	blorb_ext = strdup(EXT_BLORB);


    /* Get rid of extensions with 1 to 6 character extensions. */
    /* This will take care of an extension like ".zblorb". */
    /* More than that, there might be something weird going on */
    /* which is not our concern. */
    if (p != NULL) {
	if (strlen(p) >= 2 && strlen(p) <= 7) {
	    *p = '\0';      /* extension removed */
	}
    }

    f_setup.story_path = strdup(dirname(argv[zoptind]));

    f_setup.script_name = malloc((strlen(f_setup.story_name) + strlen(EXT_SCRIPT)) * sizeof(char) + 1);
    strncpy(f_setup.script_name, f_setup.story_name, strlen(f_setup.story_name) + 1);
    strncat(f_setup.script_name, EXT_SCRIPT, strlen(EXT_SCRIPT));

    f_setup.command_name = malloc((strlen(f_setup.story_name) + strlen(EXT_COMMAND)) * sizeof(char) + 1);
    strncpy(f_setup.command_name, f_setup.story_name, strlen(f_setup.story_name) + 1);
    strncat(f_setup.command_name, EXT_COMMAND, strlen(EXT_COMMAND));

    if (!f_setup.restore_mode) {
	f_setup.save_name = malloc(strlen(f_setup.story_name) * sizeof(char) + 5);
	strncpy(f_setup.save_name, f_setup.story_name, strlen(f_setup.story_name));
	strncat(f_setup.save_name, EXT_SAVE, strlen(EXT_SAVE));
    } else {  /*Set our auto load save as the name_save*/
	f_setup.save_name = malloc(strlen(f_setup.tmp_save_name) * sizeof(char) + 5);
	strncpy(f_setup.save_name, f_setup.tmp_save_name, strlen(f_setup.tmp_save_name));
	free(f_setup.tmp_save_name);
    }

    f_setup.aux_name = malloc((strlen(f_setup.story_name) + strlen(EXT_AUX)) * sizeof(char) + 1);
    strncpy(f_setup.aux_name, f_setup.story_name, strlen(f_setup.story_name) + 1);
    strncat(f_setup.aux_name, EXT_AUX, strlen(EXT_AUX));

}/* os_process_arguments */


/*
 * os_init_screen
 *
 * Initialise the IO interface. Prepare the screen and other devices
 * (mouse, sound board). Set various OS depending story file header
 * entries:
 *
 *     h_config (aka flags 1)
 *     h_flags (aka flags 2)
 *     h_screen_cols (aka screen width in characters)
 *     h_screen_rows (aka screen height in lines)
 *     h_screen_width
 *     h_screen_height
 *     h_font_height (defaults to 1)
 *     h_font_width (defaults to 1)
 *     h_default_foreground
 *     h_default_background
 *     h_interpreter_number
 *     h_interpreter_version
 *     h_user_name (optional; not used by any game)
 *
 * Finally, set reserve_mem to the amount of memory (in bytes) that
 * should not be used for multiple undo and reserved for later use.
 *
 * (Unix has a non brain-damaged memory model which dosen't require such
 *  ugly hacks, neener neener neener. --GH :)
 *
 */
void os_init_screen (void)
{
    /*trace(TRACE_CALLS);*/

    if (initscr() == NULL) {    /* Set up curses */
	os_fatal("Unable to initialize curses. Maybe your $TERM setting is bad.");
	exit(1);
    }
    u_setup.curses_active = 1;	/* Let os_fatal know curses is running */
    cbreak();			/* Raw input mode, no line processing */
    noecho();			/* No input echo */
    nonl();			/* No newline translation */
    intrflush(stdscr, TRUE);	/* Flush output on interrupt */
    keypad(stdscr, TRUE);	/* Enable the keypad and function keys */
    scrollok(stdscr, FALSE);	/* No scrolling unless explicitly asked for */

    if (h_version == V3 && u_setup.tandy_bit != 0)
        h_config |= CONFIG_TANDY;

    if (h_version == V3)
	h_config |= CONFIG_SPLITSCREEN;

    if (h_version >= V4)
	h_config |= CONFIG_BOLDFACE | CONFIG_EMPHASIS | CONFIG_FIXED | CONFIG_TIMEDINPUT;

    if (h_version >= V5)
      h_flags &= ~(GRAPHICS_FLAG | MOUSE_FLAG | MENU_FLAG);

#ifdef NO_SOUND
    if (h_version >= V5)
      h_flags &= ~SOUND_FLAG;

    if (h_version == V3)
      h_flags &= ~OLD_SOUND_FLAG;
#else
    if ((h_version >= 5) && (h_flags & SOUND_FLAG))
	h_flags |= SOUND_FLAG;

    if ((h_version == 3) && (h_flags & OLD_SOUND_FLAG))
	h_flags |= OLD_SOUND_FLAG;

    if ((h_version == 6) && (f_setup.sound != 0))
	h_config |= CONFIG_SOUND;
#endif

    if (h_version >= V5 && (h_flags & UNDO_FLAG))
        if (f_setup.undo_slots == 0)
            h_flags &= ~UNDO_FLAG;

    getmaxyx(stdscr, h_screen_rows, h_screen_cols);

    if (u_setup.screen_height != -1)
	h_screen_rows = u_setup.screen_height;
    if (u_setup.screen_width != -1)
	h_screen_cols = u_setup.screen_width;

    h_screen_width = h_screen_cols;
    h_screen_height = h_screen_rows;

#ifdef WIDE_SCREEN
    if (h_screen_width < 1)
	os_fatal("Invalid screen width. Must be between 1 and 255.");

    if (h_screen_width > 255)
	h_screen_width = 255;
#else
    if (h_screen_width > 255 || h_screen_width < 1)
	os_fatal("Invalid screen width. Must be between 1 and 255.");
#endif

    h_font_width = 1;
    h_font_height = 1;

    /* Must be after screen dimensions are computed.  */
    if (h_version == V6) {
      if (unix_init_pictures())
	h_config |= CONFIG_PICTURES;
      else
	h_flags &= ~GRAPHICS_FLAG;
    }

    /* Use the ms-dos interpreter number for v6, because that's the
     * kind of graphics files we understand.  Otherwise, use DEC.  */
    h_interpreter_number = h_version == 6 ? INTERP_MSDOS : INTERP_DEC_20;
    h_interpreter_version = 'F';

#ifdef COLOR_SUPPORT
    /* Enable colors if the terminal supports them, the user did not
     * disable colors, and the game or user requested colors.  User
     * requests them by specifying a foreground or background.
     */
    u_setup.color_enabled = (has_colors()
			&& !u_setup.disable_color
			&& (((h_version >= 5) && (h_flags & COLOUR_FLAG))
			  || (u_setup.foreground_color != -1)
			  || (u_setup.background_color != -1)));

    /* Maybe we don't want to muck about with changing $TERM to
     * xterm-color which some supposedly current Unicies still don't
     * understand.
     */
    if (u_setup.force_color)
	u_setup.color_enabled = TRUE;

    if (u_setup.color_enabled) {
        h_config |= CONFIG_COLOUR;
        h_flags |= COLOUR_FLAG; /* FIXME: beyond zork handling? */
        start_color();
	h_default_foreground =
	  (u_setup.foreground_color == -1)
	  	? FOREGROUND_DEF : u_setup.foreground_color;
	h_default_background =
	  (u_setup.background_color ==-1)
		? BACKGROUND_DEF : u_setup.background_color;
    } else
#endif /* COLOR_SUPPORT */
    {
	/* Set these per spec 8.3.2. */
	h_default_foreground = WHITE_COLOUR;
	h_default_background = BLACK_COLOUR;
	if (h_flags & COLOUR_FLAG) h_flags &= ~COLOUR_FLAG;
    }
    os_set_colour(h_default_foreground, h_default_background);
    os_erase_area(1, 1, h_screen_rows, h_screen_cols, 0);
}/* os_init_screen */


/*
 * os_reset_screen
 *
 * Reset the screen before the program stops.
 *
 */
void os_reset_screen (void)
{
    os_stop_sample(0);
    os_set_text_style(0);
    os_display_string((zchar *)"[Hit any key to exit.]\n");
    os_read_key(0, FALSE);
    scrollok(stdscr, TRUE); scroll(stdscr);
    refresh(); endwin();
}/* os_reset_screen */


/*
 * os_restart_game
 *
 * This routine allows the interface to interfere with the process of
 * restarting a game at various stages:
 *
 *     RESTART_BEGIN - restart has just begun
 *     RESTART_WPROP_SET - window properties have been initialised
 *     RESTART_END - restart is complete
 *
 */
void os_restart_game (int UNUSED (stage))
{
}


/*
 * os_random_seed
 *
 * Return an appropriate random seed value in the range from 0 to
 * 32767, possibly by using the current system time.
 *
 */
int os_random_seed (void)
{

    if (u_setup.random_seed == -1)
      /* Use the epoch as seed value */
      return (time(0) & 0x7fff);
    else return u_setup.random_seed;

}/* os_random_seed */


/*
 * os_load_story
 *
 * This is different from os_path_open() because we need to see if the
 * story file is actually a chunk inside a blorb file.  Right now we're
 * looking only at the exact path we're given on the command line.
 *
 * Open a file in the current directory.  If this fails, then search the
 * directories in the ZCODE_PATH environmental variable.  If that's not
 * defined, search INFOCOM_PATH.
 *
 */
FILE *os_load_story(void)
{
    FILE *fp;

    switch (ux_blorb_init(f_setup.story_file)) {
	case bb_err_NoBlorb:
//	  printf("No blorb file found.\n\n");
	  break;
        case bb_err_Format:
	  printf("Blorb file loaded, but unable to build map.\n\n");
	  break;
	case bb_err_NotFound:
	  printf("Blorb file loaded, but lacks executable chunk.\n\n");
	  break;
	case bb_err_None:
//	  printf("No blorb errors.\n\n");
	  break;
    }

    fp = fopen(f_setup.story_file, "rb");

    /* Is this a Blorb file containing Zcode? */
    if (f_setup.exec_in_blorb)
	fseek(fp, blorb_res.data.startpos, SEEK_SET);

    return fp;
}


/*
 * os_storyfile_seek
 *
 * Seek into a storyfile, either a standalone file or the
 * ZCODE chunk of a blorb file.
 *
 */
int os_storyfile_seek(FILE * fp, long offset, int whence)
{
    /* Is this a Blorb file containing Zcode? */
    if (f_setup.exec_in_blorb)
    {
	switch (whence)
	{
	    case SEEK_END:
		return fseek(fp, blorb_res.data.startpos + blorb_res.length + offset, SEEK_SET);
		break;
	    case SEEK_CUR:
		return fseek(fp, offset, SEEK_CUR);
		break;
	    case SEEK_SET:
	    default:
		return fseek(fp, blorb_res.data.startpos + offset, SEEK_SET);
		break;
	}
    }
    else
    {
	return fseek(fp, offset, whence);
    }

}


/*
 * os_storyfile_tell
 *
 * Tell the position in a storyfile, either a standalone file
 * or the ZCODE chunk of a blorb file.
 *
 */
int os_storyfile_tell(FILE * fp)
{
    /* Is this a Blorb file containing Zcode? */
    if (f_setup.exec_in_blorb)
    {
	return ftell(fp) - blorb_res.data.startpos;
    }
    else
    {
	return ftell(fp);
    }

}


#ifdef CRAP
/*
 * pathopen
 *
 * Given a standard Unix-style path and a filename, search the path for
 * that file.  If found, return a pointer to that file and put the full
 * path where the file was found in fullname.
 *
 */
static FILE *pathopen(const char *name, const char *p, const char *mode, char *fullname)
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
#endif


/*
 * getconfig
 *
 * Parse a <variable> <whitespace> <value> config file.
 * The til-end-of-line comment character is the COMMENT define.  I use '#'
 * here.  This code originally appeared in my q2-wrapper program.  Find it
 * at metalab.cs.unc.edu or assorted Quake2 websites.
 *
 * This section must be modified whenever new options are added to
 * the config file.  Ordinarily I would use yacc and lex, but the grammar
 * is too simple to involve those resources, and I can't really expect all
 * compile targets to have those two tools installed.
 *
 */
static int getconfig(char *configfile)
{
	FILE	*fp;

	size_t	num, num2;

	char	varname[LINELEN + 1];
	char	value[LINELEN + 1];

	/*
	 * We shouldn't care if the config file is unreadable or not
	 * present.  Just use the defaults.
	 *
	 */

	if ((fp = fopen(configfile, "r")) == NULL)
		return FALSE;

	while (fgets(varname, LINELEN, fp) != NULL) {

		/* If we don't get a whole line, dump the rest of the line */
		if (varname[strlen(varname)-1] != '\n')
			while (fgetc(fp) != '\n')
			;

		/* Remove trailing whitespace and newline */
		for (num = strlen(varname) - 1; isspace(varname[num]); num--)
		;
		varname[num+1] = 0;

		/* Drop everything past the comment character */
		for (num = 0; num <= strlen(varname)+1; num++) {
			if (varname[num] == COMMENT)
				varname[num] = 0;
		}

		/* Find end of variable name */
		for (num = 0; !isspace(varname[num]) && num < LINELEN; num++);

		for (num2 = num; isspace(varname[num2]) && num2 < LINELEN; num2++);

		/* Find the beginning of the value */
		strncpy(value, &varname[num2], LINELEN);
		varname[num] = 0; /* chop off value from the var name */

		/* varname now contains the variable name */


		/* First, boolean config stuff */
		if (strcmp(varname, "attrib_set") == 0) {
			f_setup.attribute_assignment = getbool(value);
		}
		else if (strcmp(varname, "attrib_test") == 0) {
			f_setup.attribute_testing = getbool(value);
		}

		else if (strcmp(varname, "ignore_fatal") == 0) {
			f_setup.ignore_errors = getbool(value);
		}

		else if (strcmp(varname, "color") == 0) {
			u_setup.disable_color = !getbool(value);
		}
		else if (strcmp(varname, "colour") == 0) {
			u_setup.disable_color = !getbool(value);
		}
		else if (strcmp(varname, "force_color") == 0) {
			u_setup.force_color = getbool(value);
		}
		else if (strcmp(varname, "obj_move") == 0) {
			f_setup.object_movement = getbool(value);
		}
		else if (strcmp(varname, "obj_loc") == 0) {
			f_setup.object_locating = getbool(value);
		}
		else if (strcmp(varname, "piracy") == 0) {
			f_setup.piracy = getbool(value);
		}
		else if (strcmp(varname, "ascii") == 0) {
			u_setup.plain_ascii = getbool(value);
		}
		else if (strcmp(varname, "sound") == 0) {
			f_setup.sound = getbool(value);
		}
		else if (strcmp(varname, "tandy") == 0) {
			u_setup.tandy_bit = getbool(value);
		}
		else if (strcmp(varname, "expand_abb") == 0) {
			f_setup.expand_abbreviations = getbool(value);
		}

		/* now for stringtype yet still numeric variables */
		else if (strcmp(varname, "background") == 0) {
			u_setup.background_color = getcolor(value);
		}
		else if (strcmp(varname, "foreground") == 0) {
			u_setup.foreground_color = getcolor(value);
		}
		else if (strcmp(varname, "context_lines") == 0) {
			f_setup.context_lines = atoi(value);
		}
		else if (strcmp(varname, "screen_height") == 0) {
			u_setup.screen_height = atoi(value);
		}
		else if (strcmp(varname, "left_margin") == 0) {
			f_setup.left_margin = atoi(value);
		}
		else if (strcmp(varname, "right_margin") == 0) {
			f_setup.right_margin = atoi(value);
		}
		else if (strcmp(varname, "randseed") == 0) {
			u_setup.random_seed = atoi(value);
		}
		else if (strcmp(varname, "script_width") == 0) {
			f_setup.script_cols = atoi(value);
		}
		else if (strcmp(varname, "undo_slots") == 0) {
			f_setup.undo_slots = atoi(value);
		}
		else if (strcmp(varname, "screen_width") == 0) {
			u_setup.screen_width = atoi(value);
		}
		/* default is set in main() by call to init_err() */
		else if (strcmp(varname, "errormode") == 0) {
			f_setup.err_report_mode = geterrmode(value);
		}

		/* now for really stringtype variable */

		else if (strcmp(varname, "zcode_path") == 0) {
			f_setup.zcode_path = malloc(strlen(value) * sizeof(char) + 1);
			strncpy(f_setup.zcode_path, value, strlen(value) * sizeof(char));
		}

		/* The big nasty if-else thingy is finished */
	} /* while */
	return TRUE;
} /* getconfig() */


/*
 * getbool
 *
 * Check a string for something that means "yes" and return TRUE.
 * Otherwise return FALSE.
 *
 */
static int getbool(char *value)
{
	int num;

	/* Be case-insensitive */
	for (num = 0; value[num] !=0; num++)
		value[num] = tolower(value[num]);

	if (strncmp(value, "y", 1) == 0)
		return TRUE;
	if (strcmp(value, "true") == 0)
		return TRUE;
	if (strcmp(value, "on") == 0)
		return TRUE;
	if (strcmp(value, "1") == 0)
		return TRUE;

	return FALSE;
} /* getbool() */


/*
 * getcolor
 *
 * Figure out what color this string might indicate and returns an integer
 * corresponding to the color macros defined in frotz.h.
 *
 */
static int getcolor(char *value)
{
	int num;

	/* Be case-insensitive */
	for (num = 0; value[num] !=0; num++)
		value[num] = tolower(value[num]);

	if (strcmp(value, "black") == 0)
		return BLACK_COLOUR;
	if (strcmp(value, "red") == 0)
		return RED_COLOUR;
	if (strcmp(value, "green") == 0)
		return GREEN_COLOUR;
	if (strcmp(value, "blue") == 0)
		return BLUE_COLOUR;
	if (strcmp(value, "magenta") == 0)
		return MAGENTA_COLOUR;
	if (strcmp(value, "cyan") == 0)
		return CYAN_COLOUR;
	if (strcmp(value, "white") == 0)
		return WHITE_COLOUR;

	if (strcmp(value, "purple") == 0)
		return MAGENTA_COLOUR;
	if (strcmp(value, "violet") == 0)
		return MAGENTA_COLOUR;
	if (strcmp(value, "aqua") == 0)
		return CYAN_COLOUR;

	/* If we can't tell what that string means,
	 * we tell caller to use the default.
	 */

	return -1;

} /* getcolor() */


/*
 * geterrmode
 *
 * Parse for "never", "once", "always", or "fatal" and return a macro
 * defined in ux_frotz.h related to the error reporting mode.
 *
 */
static int geterrmode(char *value)
{
	int num;

        /* Be case-insensitive */
	for (num = 0; value[num] !=0; num++)
		value[num] = tolower(value[num]);

	if (strcmp(value, "never") == 0)
		return ERR_REPORT_NEVER;
	if (strcmp(value, "once") == 0)
		return ERR_REPORT_ONCE;
	if (strcmp(value, "always") == 0)
		return ERR_REPORT_ALWAYS;
	if (strcmp(value, "fatal") == 0)
		return ERR_REPORT_FATAL;

	return ERR_DEFAULT_REPORT_MODE;
} /* geterrmode() */


/*
 * sigwinch_handler
 *
 * Called whenever Frotz recieves a SIGWINCH signal to make curses
 * cleanly resize the window.
 *
 */
// FIXME: figure out what to do with this
//static void sigwinch_handler(int UNUSED(sig))
//{
/*
There are some significant problems involved in getting resizes to work
properly with at least this implementation of the Z Machine and probably
the Z-Machine standard itself.  See the file BUGS for a detailed
explaination for this.  Because of this trouble, this function currently
does nothing.
*/
//}


/*
 * sigint_handler
 * Sometimes the screen will be left in a weird state if the following
 * is not done.
 *
 */
static void sigint_handler(int dummy)
{
    signal(SIGINT, sigint_handler);
    // dummy = dummy;

    os_stop_sample(0);
    scrollok(stdscr, TRUE); scroll(stdscr);
    refresh(); endwin();

    exit(1);
}

/*
void redraw(void)
{
}
*/

void os_init_setup(void)
{
	f_setup.attribute_assignment = 0;
	f_setup.attribute_testing = 0;
	f_setup.context_lines = 0;
	f_setup.object_locating = 0;
	f_setup.object_movement = 0;
	f_setup.left_margin = 0;
	f_setup.right_margin = 0;
	f_setup.ignore_errors = 0;
	f_setup.piracy = 0;		/* enable the piracy opcode */
	f_setup.undo_slots = MAX_UNDO_SLOTS;
	f_setup.expand_abbreviations = 0;
	f_setup.script_cols = 80;
	f_setup.sound = 1;
	f_setup.err_report_mode = ERR_DEFAULT_REPORT_MODE;
	f_setup.restore_mode = 0;

	f_setup.use_blorb = 0;
	f_setup.exec_in_blorb = 0;

	u_setup.disable_color = 0;
	u_setup.force_color = 0;
	u_setup.foreground_color = -1;
	u_setup.background_color = -1;
	u_setup.screen_width = -1;
	u_setup.screen_height = -1;
	u_setup.random_seed = -1;
	u_setup.random_seed = -1;
	u_setup.tandy_bit = 0;
	u_setup.current_text_style = 0;
			/* Since I can't use attr_get, which
			would make things easier, I need
			to use the same hack the MS-DOS port
			does...keep the current style in a
			global variable. */
	u_setup.plain_ascii = 0; /* true if user wants to disable Latin-1 */
	u_setup.curses_active = 0;      /* true if os_init_screen has run */
	/* u_setup.interpreter = INTERP_DEFAULT; */
	u_setup.current_color = 0;
	u_setup.color_enabled = FALSE;

}

#ifdef NO_STRRCHR
/*
 * This is for operating systems that lack strrchr(3).
 *
 */
char *strrchr(const char *s, int c)
{
    const char *save;

    if (c == 0) return (char *)s + strlen(s);
    save = 0;
    while (*s) {
	if (*s == c)
	    save = s;
	s++;
    }
    return (char *)save;
#endif	/* NO_STRRCHR */


/* A unix-like getopt, but with the names changed to avoid any problems. */
static int zgetopt (int argc, char *argv[], const char *options)
{
    static int pos = 1;
    const char *p;
    if (zoptind >= argc || argv[zoptind][0] != '-' || argv[zoptind][1] == 0)
	return EOF;
    zoptopt = argv[zoptind][pos++];
    zoptarg = NULL;
    if (argv[zoptind][pos] == 0) {
	pos = 1;
	zoptind++;
    }
    p = strchr (options, zoptopt);
    if (zoptopt == ':' || p == NULL) {
	fputs ("illegal option -- ", stderr);
	goto error;
    } else if (p[1] == ':') {
	if (zoptind >= argc) {
	    fputs ("option requires an argument -- ", stderr);
	    goto error;
	} else {
	    zoptarg = argv[zoptind];
	    if (pos != 1)
		zoptarg += pos;
	    pos = 1; zoptind++;
	}
    }
    return zoptopt;
error:
    fputc (zoptopt, stderr);
    fputc ('\n', stderr);
    return '?';
}/* zgetopt */


static void print_version(void)
{
    printf("FROTZ V%s\t", VERSION);
#ifndef NO_SOUND
        printf("Audio output enabled.");
#else
	printf("Audio output disabled.");
#endif
    printf("\nGit commit:\t%s\n", GIT_HASH);
    printf("Git tag:\t%s\n", GIT_TAG);
    printf("Git branch:\t%s\n", GIT_BRANCH);
    printf("  Frotz was originally written by Stefan Jokisch.\n");
    printf("  It complies with standard 1.0 of Graham Nelson's specification.\n");
    printf("  It was ported to Unix by Galen Hazelwood.\n");
    printf("  The core and Unix port are currently maintained by David Griffith.\n");
    printf("  See https://github.com/DavidGriffith/frotz for Frotz's homepage.\n\n");
    return;
}

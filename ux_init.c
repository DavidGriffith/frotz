/*
 * ux_init.c
 *
 * Unix interface, initialisation
 *
 */

#define __UNIX_PORT_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

/* Prototypes for getopt */
#ifdef USE_UNISTD_H
#include <unistd.h>
#elif USE_GETOPT_H
#include <getopt.h>
#else
#ifndef USE_NOTHING
extern int getopt(int, char **, char *);
extern int optind, opterr;
extern char *optarg;
#endif
#endif

#ifdef USE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "frotz.h"
#include "ux_frotz.h"

#define INFORMATION "\
\n\
FROTZ V2.32 - interpreter for all Infocom games. Complies with standard\n\
1.0 of Graham Nelson's specification. Written by Stefan Jokisch in 1995-7\n\
\n\
Syntax: frotz [options] story-file\n\
\n\
  -a   watch attribute setting  \t -l # left margin\n\
  -A   watch attribute testing  \t -o   watch object movement\n\
  -b # background colour        \t -O   watch object locating\n\
                                \t -p   plain ASCII output only\n\
                                \t -P   alter piracy opcode\n\
  -c # context lines            \t -r # right margin\n\
  -d   disable color            \t -s # random number seed value\n\
                                \t -S # transscript width\n\
  -f # foreground colour        \t -t   set Tandy bit\n\
                                \t -u # slots for multiple undo\n\
  -h # screen height            \t -w # screen width\n\
  -i   ignore runtime errors    \t -x   expand abbreviations g/x/z\n"

static int user_disable_color = 0;
static int user_foreground_color = -1;
static int user_background_color = -1;
static int user_screen_width = -1;
static int user_screen_height = -1;
static int user_random_seed = -1;
static int user_tandy_bit = 0;

static int curses_active = 0; /* true if os_init_screen has been run */
int current_text_style = 0;           /* Since I can't use attr_get, which
                        	      would make things easier, I need to
				      use the same hack the MS-DOS port
				      does...keep the current style in a
				      global variable. */
char unix_plain_ascii = 0;    /* true if user wants to disable latin-1 */

/*
 * os_fatal
 *
 * Display error message and exit program.
 *
 */

void os_fatal (const char *s)
{

    if (curses_active) {
      os_display_string("\n\n");
      beep();
      os_set_text_style(BOLDFACE_STYLE);
      os_display_string("Fatal error: ");
      os_set_text_style(0);
      os_display_string(s);
      os_display_string("\n");
      os_reset_screen();
      exit(1);
    }

    fputs ("\nFatal error: ", stderr);
    fputs (s, stderr);
    fputs ("\n\n", stderr);

    exit (1);

}/* os_fatal */

extern char script_name[];
extern char command_name[];
extern char save_name[];
extern char auxilary_name[];

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
 */

void os_process_arguments (int argc, char *argv[])
{
    int c, i;
    char stripped_story_name[MAX_FILE_NAME];
    char *p = NULL;

    /* Parse the options */

    do {

	c = getopt(argc, argv, "aAb:c:df:h:il:oOpPr:s:S:tu:w:x");

	switch(c) {
	  case 'a': option_attribute_assignment = 1; break;
	  case 'A': option_attribute_testing = 1; break;
	  case 'b': user_background_color = atoi(optarg);
	            if ((user_background_color < 2) ||
			(user_background_color > 9))
		      user_background_color = -1;
		    break;
	  case 'c': option_context_lines = atoi(optarg); break;
	  case 'd': user_disable_color = 1; break;
	  case 'f': user_foreground_color = atoi(optarg);
	            if ((user_foreground_color < 2) ||
			(user_foreground_color > 9))
		      user_foreground_color = -1;
		    break;
          case 'h': user_screen_height = atoi(optarg); break;
	  case 'i': option_ignore_errors = 1; break;
	  case 'l': option_left_margin = atoi(optarg); break;
	  case 'o': option_object_movement = 1; break;
	  case 'O': option_object_locating = 1; break;
	  case 'p': unix_plain_ascii = 1; break;
	  case 'P': option_piracy = 1; break;
	  case 'r': option_right_margin = atoi(optarg); break;
	  case 's': user_random_seed = atoi(optarg); break;
	  case 'S': option_script_cols = atoi(optarg); break;
	  case 't': user_tandy_bit = 1; break;
	  case 'u': option_undo_slots = atoi(optarg); break;
	  case 'w': user_screen_width = atoi(optarg); break;
	  case 'x': option_expand_abbreviations = 1; break;
	}

    } while (c != EOF);

    if (optind != argc - 1) {
	puts (INFORMATION);
	exit (1);
    }

    /* Save the story file name */

    story_name = argv[optind];

    /* Strip path off the story file name */

    p = story_name;

    for (i = 0; story_name[i] != 0; i++)
        if (story_name[i] == '/')
	  p = story_name + i + 1;

    for (i = 0; p[i] != '\0' && p[i] != '.'; i++)
        stripped_story_name[i] = p[i];
    stripped_story_name[i] = '\0';

    /* Create nice default file names */

    strcpy (script_name, stripped_story_name);
    strcpy (command_name, stripped_story_name);
    strcpy (save_name, stripped_story_name);
    strcpy (auxilary_name, stripped_story_name);

    /* Don't forget the extensions */

    strcat (script_name, ".scr");
    strcat (command_name, ".rec");
    strcat (save_name, ".sav");
    strcat (auxilary_name, ".aux");

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
    curses_active = 1;          /* Let os_fatal know curses is running */
    initscr();                  /* Set up curses */
    cbreak();                   /* Raw input mode, no line processing */
    noecho();                   /* No input echo */
    nonl();                     /* No newline translation */
    intrflush(stdscr, TRUE);    /* Flush output on interrupt */
    keypad(stdscr, TRUE);       /* Enable the keypad and function keys */
    scrollok(stdscr, FALSE);    /* No scrolling unless explicitly asked for */

    if (h_version == V3 && user_tandy_bit != 0)
        h_config |= CONFIG_TANDY;

    if (h_version == V3)
	h_config |= CONFIG_SPLITSCREEN;

    if (h_version >= V4)
	h_config |= CONFIG_BOLDFACE | CONFIG_EMPHASIS | CONFIG_FIXED | CONFIG_TIMEDINPUT;

#ifndef SOUND_SUPPORT
    if (h_version >= V5)
      h_flags &= ~(GRAPHICS_FLAG | SOUND_FLAG | MOUSE_FLAG | MENU_FLAG);

    if (h_version == V3)
	h_flags &= ~OLD_SOUND_FLAG;
#else
    if (h_version >= V5)
      h_flags &= ~(GRAPHICS_FLAG | MOUSE_FLAG | MENU_FLAG);

    if ((h_version >= 5) && (h_flags & SOUND_FLAG)) {
      if (unix_init_sound())
	h_flags |= SOUND_FLAG;
      else
	h_flags &= ~SOUND_FLAG;
    }
    if ((h_version == 3) && (h_flags & OLD_SOUND_FLAG)) {
      if (unix_init_sound())
	h_flags |= OLD_SOUND_FLAG;
      else
	h_flags &= ~OLD_SOUND_FLAG;
    }
#endif

    if (h_version >= V5 && (h_flags & UNDO_FLAG))
        if (option_undo_slots == 0)
            h_flags &= ~UNDO_FLAG;

    getmaxyx(stdscr, h_screen_rows, h_screen_cols);

    if (user_screen_height != -1)
	h_screen_rows = user_screen_height;
    if (user_screen_width != -1)
	h_screen_cols = user_screen_width;

    h_screen_width = h_screen_cols;
    h_screen_height = h_screen_rows;

    h_font_width = 1;
    h_font_height = 1;

    h_interpreter_number = INTERP_DEC_20; /* We is a DECsystem-20 :) */
    h_interpreter_version = 'F';

#ifdef COLOR_SUPPORT
    if (has_colors() && !user_disable_color) {
        h_config |= CONFIG_COLOUR;
        h_flags |= COLOUR_FLAG; /* FIXME: beyond zork handling? */
        start_color();
	if (user_foreground_color != -1)
	  h_default_foreground = user_foreground_color;
	else
	  h_default_foreground = WHITE_COLOUR;
	if (user_background_color != -1)
	  h_default_background = user_background_color;
	else
	  h_default_background = BLUE_COLOUR;
	os_set_colour(h_default_foreground, h_default_background);
	bkgdset(current_color);
	erase();
	/*os_erase_area(1, 1, h_screen_rows, h_screen_cols);*/
    }
    else
      if (h_flags & COLOUR_FLAG) h_flags &= ~COLOUR_FLAG;
#endif

    refresh();

}/* os_init_screen */

/*
 * os_reset_screen
 *
 * Reset the screen before the program stops.
 *
 */

void os_reset_screen (void)
{

    os_set_text_style(0);
    os_display_string((zchar *) "[Hit any key to exit.]");
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

void os_restart_game (int stage)
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

    if (user_random_seed == -1)
      /* Use the epoch as seed value */
      return (time(0) & 0x7fff);
    else return user_random_seed;

}/* os_random_seed */

/* dumb-init.c
 * $Id: dumb-init.c,v 1.1.1.1 2002/03/26 22:38:34 feedle Exp $
 *
 * Copyright 1997,1998 Alva Petrofsky <alva@petrofsky.berkeley.ca.us>.
 * Any use permitted provided this notice stays intact.
 */

#include "dumb_frotz.h"

f_setup_t f_setup;

#define INFORMATION "\
An interpreter for all Infocom and other Z-Machine games.\n\
Complies with standard 1.0 of Graham Nelson's specification.\n\
\n\
Syntax: dfrotz [options] story-file\n\
  -a   watch attribute setting \t  R xxx  do runtime setting \\xxx\n\
  -A   watch attribute testing \t    before starting (can be used repeatedly)\n\
  -h # screen height           \t -s # random number seed value\n\
  -i   ignore fatal errors     \t -S # transcript width\n\
  -I # interpreter number      \t -t   set Tandy bit\n\
  -o   watch object movement   \t -u # slots for multiple undo\n\
  -O   watch object locating   \t -w # screen width\n\
  -p   plain ASCII output only \t -x   expand abbreviations g/x/z\n\
  -P   alter piracy opcode"


/* A unix-like getopt, but with the names changed to avoid any problems.  */
static int zoptind = 1;
static int zoptopt = 0;
static char *zoptarg = NULL;
static int zgetopt (int argc, char *argv[], const char *options)
{
    static int pos = 1;
    const char *p;
    if (zoptind >= argc || argv[zoptind][0] != '-' || argv[zoptind][1] == 0)
	return EOF;
    zoptopt = argv[zoptind][pos++];
    zoptarg = NULL;
    if (argv[zoptind][pos] == 0)
	{ pos = 1; zoptind++; }
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

static int user_screen_width = 75;
static int user_screen_height = 24;
static int user_interpreter_number = -1;
static int user_random_seed = -1;
static int user_tandy_bit = 0;
static char *graphics_filename = NULL;
static bool plain_ascii = FALSE;

void os_process_arguments(int argc, char *argv[])
{
    int c;

    do_more_prompts = TRUE;
    /* Parse the options */
    do {
	c = zgetopt(argc, argv, "aAh:iI:moOpPs:R:S:tu:w:xZ:");
	switch(c) {
	  case 'a': f_setup.attribute_assignment = 1; break;
	  case 'A': f_setup.attribute_testing = 1; break;
	case 'h': user_screen_height = atoi(zoptarg); break;
	  case 'i': f_setup.ignore_errors = 1; break;
	  case 'I': f_setup.interpreter_number = atoi(zoptarg); break;
	  case 'm': do_more_prompts = FALSE;
	  case 'o': f_setup.object_movement = 1; break;
	  case 'O': f_setup.object_locating = 1; break;
	  case 'P': f_setup.piracy = 1; break;
	case 'p': plain_ascii = 1; break;
	case 'R': dumb_handle_setting(zoptarg, FALSE, TRUE); break;
	case 's': user_random_seed = atoi(zoptarg); break;
	  case 'S': f_setup.script_cols = atoi(zoptarg); break;
	case 't': user_tandy_bit = 1; break;
	  case 'u': f_setup.undo_slots = atoi(zoptarg); break;
	case 'w': user_screen_width = atoi(zoptarg); break;
	  case 'x': f_setup.expand_abbreviations = 1; break;
	  case 'Z': f_setup.err_report_mode = atoi(zoptarg);
		if ((f_setup.err_report_mode < ERR_REPORT_NEVER) ||
		(f_setup.err_report_mode > ERR_REPORT_FATAL))
			f_setup.err_report_mode = ERR_DEFAULT_REPORT_MODE;
		break;
	}
    } while (c != EOF);

    if (((argc - zoptind) != 1) && ((argc - zoptind) != 2)) {
	printf("FROTZ V%s\tdumb interface.\n", VERSION);
	puts(INFORMATION);
	printf("\t-Z # error checking mode (default = %d)\n"
	    "\t     %d = don't report errors   %d = report first error\n"
	    "\t     %d = report all errors     %d = exit after any error\n\n",
	    ERR_DEFAULT_REPORT_MODE, ERR_REPORT_NEVER,
	    ERR_REPORT_ONCE, ERR_REPORT_ALWAYS, ERR_REPORT_FATAL);
	printf("While running, enter \"\\help\" to list the runtime escape sequences\n\n");
	exit(1);
    }
/*
    if (((argc - zoptind) != 1) && ((argc - zoptind) != 2)) {
	puts(usage);
	exit(1);
    }
*/
    f_setup.story_file = argv[zoptind++];
    if (zoptind < argc)
	graphics_filename = argv[zoptind++];

    f_setup.save_name = malloc(FILENAME_MAX);
}

void os_init_screen(void)
{
    if (h_version == V3 && user_tandy_bit)
	h_config |= CONFIG_TANDY;

    if (h_version >= V5 && f_setup.undo_slots == 0)
	h_flags &= ~UNDO_FLAG;

    h_screen_rows = user_screen_height;
    h_screen_cols = user_screen_width;

    if (user_interpreter_number > 0)
	h_interpreter_number = user_interpreter_number;
    else {
	/* Use ms-dos for v6 (because that's what most people have the
	* graphics files for), but don't use it for v5 (or Beyond Zork
	* will try to use funky characters).  */
	h_interpreter_number = h_version == 6 ? INTERP_MSDOS : INTERP_DEC_20;
    }
    h_interpreter_version = 'F';

    dumb_init_input();
    dumb_init_output();
    dumb_init_pictures(graphics_filename);
}

int os_random_seed (void)
{
    if (user_random_seed == -1)
	/* Use the epoch as seed value */
	return (time(0) & 0x7fff);
    else return user_random_seed;
}

void os_restart_game (int stage) {}

void os_fatal (const char *s, ...)
{
    fprintf(stderr, "\nFatal error: %s\n", s);
    exit(1);
}

FILE *os_load_story(void)
{
    return fopen(f_setup.story_file, "rb");
}

/*
 * Seek into a storyfile, either a standalone file or the
 * ZCODE chunk of a blorb file (dumb does not support blorb
 * so this is just a wrapper for fseek)
 */
int os_storyfile_seek(FILE * fp, long offset, int whence)
{
    return fseek(fp, offset, whence);
}

/*
 * Tell the position in a storyfile, either a standalone file
 * or the ZCODE chunk of a blorb file (dumb does not support 
 * blorb so this is just a wrapper for fseek)
 */ 
int os_storyfile_tell(FILE * fp)
{
    return ftell(fp);
}


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
	f_setup.piracy = 0;
	f_setup.undo_slots = MAX_UNDO_SLOTS;
	f_setup.expand_abbreviations = 0;
	f_setup.script_cols = 80;
	f_setup.sound = 1;
	f_setup.err_report_mode = ERR_DEFAULT_REPORT_MODE;

}

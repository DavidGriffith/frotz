/*
 * strictz.c
 *
 * Strict Z error reporting functions
 */

#include "frotz.h"

#ifdef STRICTZ

#include <stdio.h>

/* Define stuff for stricter Z-code error checking, for the generic
   Unix/DOS/etc terminal-window interface. Feel free to change the way
   player prefs are specified, or replace report_zstrict_error()
   completely if you want to change the way errors are reported. */

int strictz_report_mode;
static int strictz_error_count[STRICTZ_NUM_ERRORS];

/*
 * report_strictz_error
 *
 * This handles Z-code error conditions which ought to be fatal errors,
 * but which players might want to ignore for the sake of finishing the
 * game.
 *
 * The error is provided as both a numeric code and a string. This allows
 * us to print a warning the first time a particular error occurs, and
 * ignore it thereafter.
 *
 * errnum : Numeric code for error (0 to STRICTZ_NUM_ERRORS-1)
 * errstr : Text description of error
 *
 */

void init_strictz ()
{
    int i;

    /* Initialize the STRICTZ variables. */

    strictz_report_mode = STRICTZ_DEFAULT_REPORT_MODE;

    for (i = 0; i < STRICTZ_NUM_ERRORS; i++) {
        strictz_error_count[i] = 0;
    }
}

void report_strictz_error (int errnum, const char *errstr)
{
    int wasfirst;

    if (errnum <= 0 || errnum >= STRICTZ_NUM_ERRORS)
	return;

    if (strictz_report_mode == STRICTZ_REPORT_FATAL) {
	flush_buffer ();
	os_fatal (errstr);
	return;
    }

    wasfirst = (strictz_error_count[errnum] == 0);
    strictz_error_count[errnum]++;

    if ((strictz_report_mode == STRICTZ_REPORT_ALWAYS)
	|| (strictz_report_mode == STRICTZ_REPORT_ONCE && wasfirst)) {
	char buf[256];
	long pc;

	GET_PC (pc);
	sprintf (buf, "Warning: %s (PC = %lx)", errstr, pc);
	print_string (buf);

	if (strictz_report_mode == STRICTZ_REPORT_ONCE) {
	    print_string(" (will ignore further occurrences)");
	} else {
	    sprintf (buf, " (occurrence %d)", strictz_error_count[errnum]);
	    print_string(buf);
	}
	new_line ();
    }

} /* report_strictz_error */

#endif /* STRICTZ */

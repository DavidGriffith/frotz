/*
 * ux_screen.c
 *
 * Unix interface, screen manipulation
 *
 */

#define __UNIX_PORT_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "frotz.h"
#include "ux_frotz.h"

/*
 * os_erase_area
 *
 * Fill a rectangular area of the screen with the current background
 * colour. Top left coordinates are (1,1). The cursor does not move.
 *
 */

void os_erase_area (int top, int left, int bottom, int right)
{
    int y, x, i, j;
    int saved_style;

    /* Catch the most common situation and do things the easy way */
    if ((top == 1) && (bottom == h_screen_rows) &&
	(left == 1) && (right == h_screen_cols))
      erase();
    else {
        /* Sigh... */
        getyx(stdscr, y, x);
	saved_style = current_text_style;
	os_set_text_style(0);
	top--; left--; bottom--; right--;
	for (i = top; i <= bottom; i++) {
	  move(i, left);
	  for (j = left; j <= right; j++)
	    addch(' ');
	}

	os_set_text_style(saved_style);
	move(y, x);
    }

    refresh();

}/* os_erase_area */

/*
 * os_scroll_area
 *
 * Scroll a rectangular area of the screen up (units > 0) or down
 * (units < 0) and fill the empty space with the current background
 * colour. Top left coordinates are (1,1). The cursor stays put.
 *
 */

static int old_scroll_top = 0;
static int old_scroll_bottom = 0;

void os_scroll_area (int top, int left, int bottom, int right, int units)
{
#ifdef COLOR_SUPPORT
    int y, x, i;
    int saved_style;
#endif

    if (units != 1) os_fatal("Can't Happen (os_scroll_area)"); /* FIXME */

    if (!((old_scroll_top == top) && (old_scroll_bottom == bottom))) {
        old_scroll_top = top; old_scroll_bottom = bottom;
        setscrreg(--top, --bottom);
    }
    scrollok(stdscr, TRUE);
    scroll(stdscr);
    scrollok(stdscr, FALSE);

#ifdef COLOR_SUPPORT
    if (h_flags & COLOUR_FLAG) {
        getyx(stdscr, y, x);
	move(old_scroll_bottom, 0);
	saved_style = current_text_style;
	os_set_text_style(0);
	for (i = 0; i <= right; i++) addch(' ');
	os_set_text_style(saved_style);
	move(y, x);
    }
#endif

}/* os_scroll_area */

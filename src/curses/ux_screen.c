/*
 * ux_screen.c - Unix interface, screen manipulation
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

#ifdef USE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "ux_frotz.h"

extern void resize_screen(void);
extern void restart_header(void);
extern void restart_screen(void);

/*
 * os_erase_area
 *
 * Fill a rectangular area of the screen with the current background
 * colour. Top left coordinates are (1,1). The cursor does not move.
 *
 * The final argument gives the window being changed, -1 if only a
 * portion of a window is being erased, or -2 if the whole screen is
 * being erased.  This is not relevant for the curses interface.
 *
 */
void os_erase_area (int top, int left, int bottom, int right, int UNUSED(win))
{
    int y, x, i, j;

    /* Catch the most common situation and do things the easy way */
    if ((top == 1) && (bottom == h_screen_rows) &&
	(left == 1) && (right == h_screen_cols)) {
#ifdef COLOR_SUPPORT
      /* Only set the curses background when doing an erase, so it won't
       * interfere with the copying we do in os_scroll_area.
       */
      bkgdset(u_setup.current_color | ' ');
      erase();
      bkgdset(0);
#else
      erase();
#endif
    } else {
        /* Sigh... */
	int saved_style = u_setup.current_text_style;
	os_set_text_style(u_setup.current_color);
	getyx(stdscr, y, x);
	top--; left--; bottom--; right--;
	for (i = top; i <= bottom; i++) {
	  move(i, left);
	  for (j = left; j <= right; j++)
	    addch(' ');
	}
	move(y, x);
	os_set_text_style(saved_style);
    }
}/* os_erase_area */


/*
 * os_scroll_area
 *
 * Scroll a rectangular area of the screen up (units > 0) or down
 * (units < 0) and fill the empty space with the current background
 * colour. Top left coordinates are (1,1). The cursor stays put.
 *
 */
void os_scroll_area (int top, int left, int bottom, int right, int units)
{
  top--; left--; bottom--; right--;

  if ((left == 0) && (right == h_screen_cols - 1)) {
    static int old_scroll_top = 0;
    static int old_scroll_bottom = 0;

    if (!((old_scroll_top == top) && (old_scroll_bottom == bottom))) {
        old_scroll_top = top; old_scroll_bottom = bottom;
        setscrreg(top, bottom);
    }
    scrollok(stdscr, TRUE);
    scrl(units);
    scrollok(stdscr, FALSE);
  } else {
    int row, col, x, y;
    chtype ch;

    getyx(stdscr, y, x);
    /* Must turn off attributes during copying.  */
    attrset(0);
    if (units > 0) {
      for (row = top; row <= bottom - units; row++)
	for (col = left; col <= right; col++) {
	  ch = mvinch(row + units, col);
	  mvaddch(row, col, ch);
	}
    } else if (units < 0) {
      for (row = bottom; row >= top - units; row--)
	for (col = left; col <= right; col++) {
	  ch = mvinch(row + units, col);
	  mvaddch(row, col, ch);
	}
    }
    /* Restore attributes.  */
    os_set_text_style(u_setup.current_text_style);
    move(y, x);
  }
  if (units > 0)
    os_erase_area(bottom - units + 2, left + 1, bottom + 1, right + 1, 0);
  else if (units < 0)
    os_erase_area(top + 1, left + 1, top - units, right + 1, 0);
}/* os_scroll_area */


/*
 * Copy the screen into a buffer.  The buffer must be at least
 * cols * lines + 1 elements long and will contain the top left part of
 * the screen with the given dimensions.  Line n on the screen will start
 * at position col * n in the array.
 *
 * Reading stops on error, typically if the screen has fewer lines than
 * requested.  If n lines were successfully read, a zero is written at
 * position cols * n of the array (hence the size requirement).  If the
 * screen has fewer columns than requested, lines are read up to the last
 * column.  They may or may not be terminated by zero - it depends on
 * the curses implementation.
 *
 * The cursor position is affected.  The number of successfully read lines
 * is returned.
 */
static int save_screen(int lines, int cols, chtype *buf)
{
    int li;
    for (li = 0; li < lines; ++li)
        if (ERR == mvinchnstr(li, 0, buf + li * cols, cols))
            break;
    buf[li * cols] = 0;
    return li;
}


/*
 * Restore the screen from a buffer.  This is the inverse of save_screen.
 * The buffer must be at least cols * lines elements long.  See save_screen
 * about the layout.
 *
 * The screen is cleared first.  Areas are left blank if the buffer has
 * fewer columns or lines than the screen.  If the buffer has more columns
 * or lines than the screen, the leftmost columns and topmost lines are
 * printed, as much as will fit.  (This is probably what you want for
 * columns but not for lines, so add an appropriate offset to buf.)
 *
 * Writing stops early if a line in buf starts with zero or if there is an
 * error.  The cursor position is affected.  Returns the number of successfully
 * written lines.
 */
static int restore_screen(int lines, int cols, const chtype *buf)
{
    int li, n = MIN(lines, LINES);
    clear();
    for (li = 0; li < n; ++li) {
        const chtype *p = buf + li * cols;
        if (!*p || ERR == mvaddchnstr(li, 0, p, cols))
            break;
    }
    return li;
}


/*
 * unix_resize_display
 *
 * Resize the display and redraw.
 *
 */
void unix_resize_display(void)
{
    int x, y,
        lines = h_screen_rows, cols = h_screen_cols;
    chtype
        *const buf = malloc(sizeof(chtype) * (cols * lines + 1)),
        *pos = buf;

    getyx(stdscr, y, x);
    lines = save_screen(lines, cols, buf);

    endwin();
    refresh();
    unix_get_terminal_size();

    if (lines > h_screen_rows) {
        /* Lose some of the topmost lines that won't fit. */
        int lost = lines - h_screen_rows;
        /* Don't lose the cursor position though,
           drop some trailing lines instead. */
        if (y < lost)
            lost = y;
        lines = h_screen_rows;
        y -= lost;
        pos += cols * lost;
    }
    if (y >= h_screen_rows)
        y = h_screen_rows - 1;
    if (x >= h_screen_cols)
        x = h_screen_cols - 1;
    restore_screen(lines, cols, pos);
    free(buf);
    move(y, x);

    /* Notify the game that the display needs refreshing */
    if (h_version == V6)
	h_flags |= REFRESH_FLAG;

    if (zmp != NULL) {
	resize_screen();
	restart_header();
    }
}/* unix_redraw_display */

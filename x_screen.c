/*
 * x_screen.c
 *
 * X interface, screen manipulation
 *
 */
/*
 * Copyright (c) 1998-2000 Daniel Schepler
 * Permission is granted to distribute, copy, and modify this source and
 * resulting binaries under the turns of the Artistic License.  This
 * should have been included in the COPYING file along with this
 * distribution.
 *
 * xfrotz comes with NO WARRANTY.  See the Artistic License for more
 * information.
 */

#include "frotz.h"
#include "x_frotz.h"
#include <stdio.h>

/*
 * os_erase_area
 *
 * Fill a rectangular area of the screen with the current background
 * colour. Top left coordinates are (1,1). The cursor does not move.
 *
 */

void os_erase_area (int top, int left, int bottom, int right)
{
  XFillRectangle(dpy, main_window, reversed_gc,
                 left - 1, top - 1, right - left + 1, bottom - top + 1);
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
  if (units == 0)
    return;
  else if (units > bottom - top || units < top - bottom)
    XFillRectangle(dpy, main_window, reversed_gc,
                   left - 1, top - 1, right - left + 1, bottom - top + 1);
  else if (units > 0) {
    XCopyArea(dpy, main_window, main_window, current_gc,
              left - 1, top - 1 + units,
              right - left + 1, bottom - top - units + 1,
              left - 1, top - 1);
    XFillRectangle(dpy, main_window, reversed_gc,
                   left - 1, bottom - units, right - left + 1, units);
  }
  else {
    XCopyArea(dpy, main_window, main_window, current_gc,
              left - 1, top - 1, right - left + 1, bottom - top + units + 1,
              left - 1, top - 1 + units);
    XFillRectangle(dpy, main_window, reversed_gc,
                   left - 1, top - 1, right - left + 1, - units);
  }
}/* os_scroll_area */

/*
 * x_input.c
 *
 * X interface, input functions
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
#include <X11/Xutil.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#undef XK_MISCELLANY
#include <stdio.h>
#include <string.h>

extern bool is_terminator(zchar key);
extern void x_del_char(zchar c);

/*
 * os_read_line
 *
 * Read a line of input from the keyboard into a buffer. The buffer
 * may already be primed with some text. In this case, the "initial"
 * text is already displayed on the screen. After the input action
 * is complete, the function returns with the terminating key value.
 * The length of the input should not exceed "max" characters plus
 * an extra 0 terminator.
 *
 * Terminating keys are the return key (13) and all function keys
 * (see the Specification of the Z-machine) which are accepted by
 * the is_terminator function. Mouse clicks behave like function
 * keys except that the mouse position is stored in global variables
 * "mouse_x" and "mouse_y" (top left coordinates are (1,1)).
 *
 * Furthermore, Frotz introduces some special terminating keys:
 *
 *     ZC_HKEY_KEY_PLAYBACK (Alt-P)
 *     ZC_HKEY_RECORD (Alt-R)
 *     ZC_HKEY_SEED (Alt-S)
 *     ZC_HKEY_UNDO (Alt-U)
 *     ZC_HKEY_RESTART (Alt-N, "new game")
 *     ZC_HKEY_QUIT (Alt-X, "exit game")
 *     ZC_HKEY_DEBUGGING (Alt-D)
 *     ZC_HKEY_HELP (Alt-H)
 *
 * If the timeout argument is not zero, the input gets interrupted
 * after timeout/10 seconds (and the return value is 0).
 *
 * The complete input line including the cursor must fit in "width"
 * screen units.
 *
 * The function may be called once again to continue after timeouts,
 * misplaced mouse clicks or hot keys. In this case the "continued"
 * flag will be set. This information can be useful if the interface
 * implements input line history.
 *
 * The screen is not scrolled after the return key was pressed. The
 * cursor is at the end of the input line when the function returns.
 *
 * Since Inform 2.2 the helper function "completion" can be called
 * to implement word completion (similar to tcsh under Unix).
 *
 */

zchar os_read_line (int max, zchar *buf, int timeout, int width, int continued)
{
  zchar c;
  int index;

  index = 0;
  while (buf[index] != 0) index++;
  for (;;) {
    c = os_read_key(timeout, TRUE);
    if (is_terminator(c))
      return c;
    if (c == ZC_BACKSPACE) {
      if (index == 0)
        os_beep(1);
      else {
        index--;
        x_del_char(buf[index]);
        buf[index] = 0;
      }
    }
    else if (index == max)
      os_beep(1);
    else {
      os_display_char(c);
      buf[index++] = c;
      buf[index] = 0;
    }
  }
}/* os_read_line */

/*
 * os_read_key
 *
 * Read a single character from the keyboard (or a mouse click) and
 * return it. Input aborts after timeout/10 seconds.
 *
 */

zchar os_read_key (int timeout, int cursor)
{
  XEvent ev;
  char result[2];
  int size;
  KeySym symbol;
  int text_height;

  text_height = current_font_info->ascent + current_font_info->descent;
  if (cursor)
    XFillRectangle(dpy, main_window, cursor_gc,
                   curr_x, curr_y, 2, text_height);
  for(;;) {
    XNextEvent(dpy, &ev);
    if (ev.type == KeyPress) {
      XKeyEvent *key_ev = (XKeyEvent *) &ev;

      if (cursor)
        XFillRectangle(dpy, main_window, cursor_gc,
                       curr_x, curr_y, 2, text_height);
      size = XLookupString(key_ev, result, sizeof result,
                           &symbol, NULL);
      if (size == 1) {
        if (key_ev->state & Mod1Mask)
          switch (result[0]) {
          case 'r':
            return ZC_HKEY_RECORD;
          case 'p':
            return ZC_HKEY_PLAYBACK;
          case 's':
            return ZC_HKEY_SEED;
          case 'u':
            return ZC_HKEY_UNDO;
          case 'n':
            return ZC_HKEY_RESTART;
          case 'x':
            return ZC_HKEY_QUIT;
          case 'd':
            return ZC_HKEY_DEBUG;
          case 'h':
            return ZC_HKEY_HELP;
          default:
            os_beep(1);
          }
        else
          return result[0] == 10 ? 13 : result[0];
      }
      else {
        switch (symbol) {
        case XK_Left:
        case XK_KP_Left:
          return ZC_ARROW_LEFT;
        case XK_Up:
        case XK_KP_Up:
          return ZC_ARROW_UP;
        case XK_Right:
        case XK_KP_Right:
          return ZC_ARROW_RIGHT;
        case XK_Down:
        case XK_KP_Down:
          return ZC_ARROW_DOWN;
        default:
          if (symbol >= XK_F1 && symbol <= XK_F12)
	    return (ZC_FKEY_MIN - XK_F1) + symbol;
        }
      }
      if (cursor)
        XFillRectangle(dpy, main_window, cursor_gc,
                       curr_x, curr_y, 2, text_height);
    }
    else if (ev.type == ButtonPress) {
      XButtonEvent *button_ev = (XButtonEvent *) &ev;

      if (cursor)
        XFillRectangle(dpy, main_window, cursor_gc,
                       curr_x, curr_y, 2, text_height);
      mouse_x = button_ev->x + 1;
      mouse_y = button_ev->y + 1;
      return ZC_SINGLE_CLICK;
    }
  }
}/* os_read_key */

/*
 * os_read_file_name
 *
 * Return the name of a file. Flag can be one of:
 *
 *    FILE_SAVE     - Save game file
 *    FILE_RESTORE  - Restore game file
 *    FILE_SCRIPT   - Transscript file
 *    FILE_RECORD   - Command file for recording
 *    FILE_PLAYBACK - Command file for playback
 *    FILE_SAVE_AUX - Save auxilary ("preferred settings") file
 *    FILE_LOAD_AUX - Load auxilary ("preferred settings") file
 *
 * The length of the file name is limited by MAX_FILE_NAME. Ideally
 * an interpreter should open a file requester to ask for the file
 * name. If it is unable to do that then this function should call
 * print_string and read_string to ask for a file name.
 *
 */

extern void read_string(int, zchar *);

int os_read_file_name (char *file_name, const char *default_name, int flag)
{
    int saved_replay = istream_replay;
    int saved_record = ostream_record;

    /* Turn off playback and recording temporarily */

    istream_replay = 0;
    ostream_record = 0;

    print_string ("Enter a file name.\nDefault is \"");
    print_string (default_name);
    print_string ("\": ");

    read_string (MAX_FILE_NAME, file_name);

    /* Use the default name if nothing was typed */

    if (file_name[0] == 0)
        strcpy (file_name, default_name);

    /* Restore state of playback and recording */

    istream_replay = saved_replay;
    ostream_record = saved_record;

    return 1;
} /* os_read_file_name */

/*
 * ux_input.c
 *
 * Unix interface, input functions
 *
 */

#define __UNIX_PORT_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#ifdef USE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "frotz.h"
#include "ux_frotz.h"

static struct timeval global_timeout;

#define MAX_HISTORY 20
static char *history_buffer[MAX_HISTORY];
static short history_pointer = 0; /* Pointer to next available slot */
static short history_frame = 0; /* Pointer to what the user's looking at */

extern bool is_terminator (zchar);
extern void read_string (int, zchar *);
extern int completion (const zchar *, zchar *);

/*
 * unix_set_global_timeout
 *
 * This sets up a time structure to determine when unix_read_char should
 * return zero (representing input timeout).  When current system time
 * equals global_timeout, boom.
 *
 */

static void unix_set_global_timeout(int timeout)
{
    if (!timeout) global_timeout.tv_sec = 0;
    else {
        gettimeofday(&global_timeout, NULL);
        global_timeout.tv_sec += (timeout/10);
        global_timeout.tv_usec += ((timeout%10)*100000);
        if (global_timeout.tv_usec > 999999) {
          global_timeout.tv_sec++;
          global_timeout.tv_usec -= 1000000;
        }
    }
}

/*
 * unix_read_char
 *
 * This uses the curses getch() routine to get the next character typed,
 * and returns it unless the global timeout is reached.  It returns values
 * which the standard considers to be legal input, and also returns editing
 * and frotz hot keys.  If called with a non-zero flag, it will also return
 * line-editing keys like INSERT, etc,
 *
 */

static int unix_read_char(int flag)
{
    int c;
    struct timeval ctime;

    while(1) {
      /* Timed keyboard input.  Crude but functional. */
      if (global_timeout.tv_sec) {
	  nodelay(stdscr, TRUE);
	  do {
              c = getch();
              if (c == ERR) {
                  gettimeofday(&ctime, NULL);
                  if ((ctime.tv_sec >= global_timeout.tv_sec) &&
                      (ctime.tv_usec >= global_timeout.tv_usec)) {
                      nodelay(stdscr, FALSE);
                      return 0;
                  }
              }
          } while (c == ERR);
          nodelay(stdscr, FALSE);
      }
      /* The easy way. */
      else c = getch();

      /* Catch 98% of all input right here... */
      if ( ((c >= 32) && (c <= 126)) || (c == 13) || (c == 8))
        return c;

      /* ...and the other 2% makes up 98% of the code. :( */
      switch(c) {
        /* Lucian P. Smith reports KEY_ENTER on Irix 5.3.  10 has never
           been reported, but I'm leaving it in just in case. */
        case 10: case KEY_ENTER: return 13;
        /* I've seen KEY_BACKSPACE returned on some terminals. */
        case KEY_BACKSPACE: return 8;
        /* On seven-bit connections, "Alt-Foo" is returned as an escape
           followed by the ASCII value of the letter.  We have to decide
           here whether to return a single escape or a frotz hot key. */
        case 27: nodelay(stdscr, TRUE); c = getch(); nodelay(stdscr, FALSE);
                 if (c == ERR) return 27;
                 switch(c) {
                   case 112: return ZC_HKEY_PLAYBACK; /* Alt-P */
                   case 114: return ZC_HKEY_RECORD; /* Alt-R */
                   case 115: return ZC_HKEY_SEED; /* Alt-S */
                   case 117: return ZC_HKEY_UNDO; /* Alt-U */
                   case 110: return ZC_HKEY_RESTART; /* Alt-N */
                   case 120: return ZC_HKEY_QUIT; /* Alt-X */
		   case 100: return ZC_HKEY_DEBUG; /* Alt-D */
		   case 104: return ZC_HKEY_HELP; /* Alt-H */
                   default: return 27;
                 }
                 break;
	/* The standard function key block. */
        case KEY_UP: return 129;
        case KEY_DOWN: return 130;
        case KEY_LEFT: return 131;
        case KEY_RIGHT: return 132;
        case KEY_F(1): return 133; case KEY_F(2): return 134;
        case KEY_F(3): return 135; case KEY_F(4): return 136;
        case KEY_F(5): return 137; case KEY_F(6): return 138;
        case KEY_F(7): return 139; case KEY_F(8): return 140;
        case KEY_F(9): return 141; case KEY_F(10): return 142;
        case KEY_F(11): return 143; case KEY_F(12): return 144;
        /* Curses can't differentiate keypad numbers from cursor keys. Damn. */
        /* This will catch the alt-keys on 8-bit clean input streams... */
        case 240: return ZC_HKEY_PLAYBACK; /* Alt-P */
        case 242: return ZC_HKEY_RECORD; /* Alt-R */
        case 243: return ZC_HKEY_SEED; /* Alt-S */
        case 245: return ZC_HKEY_UNDO; /* Alt-U */
        case 238: return ZC_HKEY_RESTART; /* Alt-N */
        case 248: return ZC_HKEY_QUIT; /* Alt-X */
        case 228: return ZC_HKEY_DEBUG; /* Alt-D */
        case 232: return ZC_HKEY_HELP; /* Alt-H */
#ifdef EMACS_EDITING
        case 21: return 27; /* Ctrl-U, erase line */
        case 2: return 131; /* Ctrl-B, left arrow  */
        case 6: return 132; /* Ctrl-F, right arrow */
        case 16: return 129; /* Ctrl-P, up arrow */
        case 14: return 130; /* Ctrl-N, down arrow */
        case 1: c = KEY_HOME; break; /* Ctrl-A */
        case 4: c = 127; break; /* Ctrl-D */
        case 5: c = KEY_END; break; /* Ctrl-E */
#endif
	default: break; /* Who knows? */
      }

      /* Finally, if we're in full line mode (os_read_line), we might return
         codes which aren't legal Z-machine keys but are used by the editor. */
      if (flag) return c;
    }
}


/*
 * unix_add_to_history
 *
 * Add the given string to the next available history buffer slot.  Commandeer
 * that slot if necessary using realloc.
 *
 */

static void unix_add_to_history(zchar *str)
{

    if (history_buffer[history_pointer] == NULL)
      history_buffer[history_pointer] = (char *) malloc(strlen(str) + 1);
    else
      history_buffer[history_pointer] =
        (char *) realloc(history_buffer[history_pointer], strlen(str) + 1);
    strcpy(history_buffer[history_pointer], str);
    history_pointer = ((history_pointer + 1) % MAX_HISTORY);
    history_frame = history_pointer; /* Reset user frame after each line */
}

/*
 * unix_history_back
 *
 * Copy last available string to str, if possible.  Return 1 if successful.
 *
 */

static int unix_history_back(zchar *str)
{

    history_frame--; if (history_frame==-1) history_frame = (MAX_HISTORY - 1);
    if ((history_frame == history_pointer) ||
        (history_buffer[history_frame] == NULL)) {
        beep(); history_frame = (history_frame + 1) % MAX_HISTORY;
        return 0;
    }
    strcpy(str, history_buffer[history_frame]);
    return 1;

}

/*
 * unix_history_forward
 *
 * Opposite of unix_history_back, and works in the same way.
 *
 */

static int unix_history_forward(zchar *str)
{

    history_frame = (history_frame + 1) % MAX_HISTORY;
    if ((history_frame == history_pointer) ||
        (history_buffer[history_frame] == NULL)) {
        beep(); history_frame--; if (history_frame == -1) history_frame =
                                                            (MAX_HISTORY - 1);
        return 0;
    }
    strcpy(str, history_buffer[history_frame]);
    return 1;

}

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
    int ch, scrpos, pos, y, x;
    char insert_flag = 1;  /* Insert mode is now default */

    scrpos = pos = strlen((char *) buf);

    if (!continued)
      history_frame = history_pointer;  /* Reset user's history view */

    unix_set_global_timeout(timeout);

    getyx(stdscr, y, x);

    do {
        refresh(); /* Shouldn't be necessary, but is, to print spaces */

        ch = unix_read_char(1);

	/* Backspace */
        if ((ch == 8) && (scrpos)) {
            mvdelch(y, --x);
            pos--; scrpos--;
            if (scrpos != pos)
              memmove(&(buf[scrpos]), &(buf[scrpos+1]), pos-scrpos);
        }

	/* Delete */
	if (((ch == 127) || (ch == KEY_DC)) && (scrpos < pos)) {
	    delch(); pos--;
	    memmove(&(buf[scrpos]), &(buf[scrpos+1]), pos-scrpos);
	}

        /* Left key */
        if ((ch == 131) && (scrpos)) {
            scrpos--;
            move(y, --x);
        }
        /* Right key */
        if ((ch == 132) && (scrpos < pos)) {
            scrpos++;
            move(y, ++x);
        }

        /* Home */
        if (ch == KEY_HOME) {
            x -= scrpos; scrpos = 0;
            move(y, x);
        }
        /* End */
        if (ch == KEY_END) {
            x += (pos - scrpos); scrpos = pos;
            move(y,x);
        }

        /* Insert */
        if (ch == KEY_IC)
          insert_flag = (insert_flag ? 0 : 1);

        /* Up and down keys */
        if (ch == 129) {
            if (unix_history_back(buf)) {
              x -= scrpos;
              move(y, x);
              while (scrpos) {addch(' '); scrpos--;}
              move(y, x);
              addstr(buf);
              scrpos = pos = strlen(buf);
              x += scrpos;
            }
        }
        if (ch == 130) {
            if (unix_history_forward(buf)) {
              x -= scrpos;
              move(y, x);
              while(scrpos) {addch(' '); scrpos--;}
              move(y, x);
              addstr(buf);
              scrpos = pos = strlen(buf);
              x += scrpos;
            }
        }

	/* Page up/down (passthrough as up/down arrows for beyond zork) */
        if (ch == KEY_PPAGE) ch = 129;
        if (ch == KEY_NPAGE) ch = 130;

        /* Escape */
        if (ch == 27) {
	    /* Move cursor to end of buffer first */
            x += (pos - scrpos); scrpos = pos; move(y,x);
            x -= scrpos;
            move(y, x);
            while (scrpos) {addch(' '); scrpos--;}
            move(y, x); pos = 0;
        }

	/* Tab */
	if ((ch == 9) && (scrpos == pos)) {
	    int status;
	    zchar extension[10];

	    status = completion(buf, extension);
	    if (status != 2) {
	      addstr(extension);
	      strcpy(&buf[pos], extension);
	      pos += strlen(extension); scrpos += strlen(extension);
	    }
	    if (status) beep();
	}

        /* ASCII printable */
        if ((ch >= 32) && (ch <= 126)) {
            if (pos == scrpos) {
                /* Append to end of buffer */
                if ((pos < max) && (pos < width)) {
                    buf[pos++] = (char) ch;
                    addch(ch);
                    scrpos++; x++;
                } else beep();
            }
            else {
                /* Insert/overwrite in middle of buffer */
                if (insert_flag) {
                    memmove(&buf[scrpos+1], &buf[scrpos], pos-scrpos);
                    buf[scrpos++] = (char) ch;
                    insch(ch);
                    pos++; x++; move(y, x);
                } else {
                    buf[scrpos++] = (char) ch;
                    addch(ch);
                    x++;
                }
            }
        }
    } while (!is_terminator(ch));

    buf[pos] = '\0';
    if (ch == 13) unix_add_to_history(buf);
    return ch;

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
    zchar c;

    refresh();
    if (!cursor) curs_set(0);

    unix_set_global_timeout(timeout);
    c = (zchar) unix_read_char(0);

    if (!cursor) curs_set(1);
    return c;

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

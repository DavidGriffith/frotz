/*
 * x_init.c
 *
 * X interface, initialisation
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
#include <stdlib.h>
#include <time.h>
#include <X11/Intrinsic.h>

/* Variables to save from os_process_arguments for use in os_init_screen */
static int saved_argc;
static char **saved_argv;
static char *user_bg, *user_fg;
static int user_tandy_bit;
static int user_random_seed = -1;
char *x_class;
char *x_name;

/*
 * os_fatal
 *
 * Display error message and exit program.
 *
 */

void os_fatal (const char *s)
{
  fprintf(stderr, "%s\n", s);
  exit(1);
}/* os_fatal */

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

static void parse_boolean(char *value, void *parsed) {
  int *parsed_var = (int *) parsed;

  if (! strcasecmp(value, "on") || ! strcasecmp(value, "true") ||
      ! strcasecmp(value, "yes"))
    *parsed_var = 1;
  else if (! strcasecmp(value, "off") || ! strcasecmp(value, "false") ||
           ! strcasecmp(value, "no"))
    *parsed_var = 0;
  else
    fprintf(stderr, "Warning: invalid boolean resource `%s'\n", value);
}

static void parse_int(char *value, void *parsed) {
  int *parsed_var = (int *) parsed;
  char *parse_end;

  int result = (int) strtol(value, &parse_end, 10);
  if (*parse_end || !(*value))
    fprintf(stderr, "Warning: invalid integer resource `%s'\n", value);
  else
    *parsed_var = result;
}

static void parse_zstrict(char *value, void *parsed) {
  int *parsed_var = (int *) parsed;

  if (! strcasecmp(value, "on") || ! strcasecmp(value, "true") ||
      ! strcasecmp(value, "yes") || ! strcasecmp(value, "always"))
    *parsed_var = STRICTZ_REPORT_ALWAYS;
  else if (! strcasecmp(value, "off") || ! strcasecmp(value, "false") ||
           ! strcasecmp(value, "no") || ! strcasecmp(value, "never"))
    *parsed_var = STRICTZ_REPORT_NEVER;
  else if (! strcasecmp(value, "once"))
    *parsed_var = STRICTZ_REPORT_ONCE;
  else if (! strcasecmp(value, "fatal"))
    *parsed_var = STRICTZ_REPORT_FATAL;
  else {
    char *parse_end;
    int result = (int) strtol(value, &parse_end, 10);

    if (*parse_end || !(*value) || result < STRICTZ_REPORT_NEVER ||
        result > STRICTZ_REPORT_FATAL)
      fprintf(stderr, "Warning: invalid zstrict level resource `%s'\n",
              value);
    else
      *parsed_var = result;
  }
}

static void parse_string(char *value, void *parsed) {
  *((char **) parsed) = value;
}

void os_process_arguments (int argc, char *argv[])
{
  static XrmOptionDescRec options[] = {
    { "-aa", ".watchAttrAssign", XrmoptionNoArg, (caddr_t) "true" },
    { "+aa", ".watchAttrAssign", XrmoptionNoArg, (caddr_t) "false" },
    { "-at", ".watchAttrTest", XrmoptionNoArg, (caddr_t) "true" },
    { "+at", ".watchAttrTest", XrmoptionNoArg, (caddr_t) "false" },
    { "-c", ".contextLines", XrmoptionSepArg, (caddr_t) NULL },
    { "-ol", ".watchObjLocating", XrmoptionNoArg, (caddr_t) "true" },
    { "+ol", ".watchObjLocating", XrmoptionNoArg, (caddr_t) "false" },
    { "-om", ".watchObjMovement", XrmoptionNoArg, (caddr_t) "true" },
    { "+om", ".watchObjMovement", XrmoptionNoArg, (caddr_t) "false" },
    { "-lm", ".leftMargin", XrmoptionSepArg, (caddr_t) NULL },
    { "-rm", ".rightMargin", XrmoptionSepArg, (caddr_t) NULL },
    { "-e", ".ignoreErrors", XrmoptionNoArg, (caddr_t) "true" },
    { "+e", ".ignoreErrors", XrmoptionNoArg, (caddr_t) "false" },
    { "-p", ".piracy", XrmoptionNoArg, (caddr_t) "true" },
    { "+p", ".piracy", XrmoptionNoArg, (caddr_t) "false" },
    { "-t", ".tandy", XrmoptionNoArg, (caddr_t) "true" },
    { "+t", ".tandy", XrmoptionNoArg, (caddr_t) "false" },
    { "-u", ".undoSlots", XrmoptionSepArg, (caddr_t) NULL },
    { "-x", ".expandAbbrevs", XrmoptionNoArg, (caddr_t) "true" },
    { "+x", ".expandAbbrevs", XrmoptionNoArg, (caddr_t) "false" },
    { "-sc", ".scriptColumns", XrmoptionSepArg, (caddr_t) NULL },
    { "-rs", ".randomSeed", XrmoptionSepArg, (caddr_t) NULL },
    { "-zs", ".zStrict", XrmoptionSepArg, (caddr_t) NULL },
    /* I can never remember whether it's zstrict or strictz */
    { "-sz", ".zStrict", XrmoptionSepArg, (caddr_t) NULL },
    { "-fn-b", ".fontB", XrmoptionSepArg, (caddr_t) NULL },
    { "-fn-i", ".fontI", XrmoptionSepArg, (caddr_t) NULL },
    { "-fn-bi", ".fontBI", XrmoptionSepArg, (caddr_t) NULL },
    { "-fn-f", ".fontF", XrmoptionSepArg, (caddr_t) NULL },
    { "-fn-fb", ".fontFB", XrmoptionSepArg, (caddr_t) NULL },
    { "-fn-fi", ".fontFI", XrmoptionSepArg, (caddr_t) NULL },
    { "-fn-fbi", ".fontFBI", XrmoptionSepArg, (caddr_t) NULL },
    { "-fn-z", ".fontZ", XrmoptionSepArg, (caddr_t) NULL }
  };
  static struct {
    char *class;
    char *name;
    void (*parser)(char *, void *);
    void *ptr;
  } vars[] = {
    { ".WatchAttribute", ".watchAttrAssign", parse_boolean,
      &option_attribute_assignment },
    { ".WatchAttribute", ".watchAttrTest", parse_boolean,
      &option_attribute_testing },
    { ".ContextLines", ".contextLines", parse_int,
      &option_context_lines },
    { ".WatchObject", ".watchObjLocating", parse_boolean,
      &option_object_locating },
    { ".WatchObject", ".watchObjMovement", parse_boolean,
      &option_object_movement },
    { ".Margin", ".leftMargin", parse_int,
      &option_left_margin },
    { ".Margin", ".rightMargin", parse_int,
      &option_right_margin },
    { ".IgnoreErrors", ".ignoreErrors", parse_boolean,
      &option_ignore_errors },
    { ".Piracy", ".piracy", parse_boolean,
      &option_piracy },
    { ".Tandy", ".tandy", parse_boolean,
      &user_tandy_bit },
    { ".UndoSlots", ".undoSlots", parse_int,
      &option_undo_slots },
    { ".ExpandAbbrevs", ".expandAbbrevs", parse_boolean,
      &option_expand_abbreviations },
    { ".ScriptColumns", ".scriptColumns", parse_int,
      &option_script_cols },
    { ".RandomSeed", ".randomSeed", parse_int,
      &user_random_seed },
    { ".ZStrict", ".zStrict", parse_zstrict,
      &strictz_report_mode },
    { ".Background", ".background", parse_string,
      &user_bg },
    { ".Foreground", ".foreground", parse_string,
      &user_fg },
    { ".Font", ".font", parse_string,
      &font_names[0] },
    { ".Font", ".fontB", parse_string,
      &font_names[1] },
    { ".Font", ".fontI", parse_string,
      &font_names[2] },
    { ".Font", ".fontBI", parse_string,
      &font_names[3] },
    { ".Font", ".fontF", parse_string,
      &font_names[4] },
    { ".Font", ".fontFB", parse_string,
      &font_names[5] },
    { ".Font", ".fontFI", parse_string,
      &font_names[6] },
    { ".Font", ".fontFBI", parse_string,
      &font_names[7] },
    { ".Font", ".fontZ", parse_string,
      &font_names[8] }
  };
  XtAppContext app_context;
  char *str_type_return;
  char *class_buf, *name_buf, *class_append, *name_append;
  int i;
  XrmValue value;

  saved_argv = malloc(sizeof(char *) * argc);
  memcpy(saved_argv, argv, sizeof(char *) * argc);
  saved_argc = argc;
  app_context = XtCreateApplicationContext();
  dpy = XtOpenDisplay(app_context, NULL, NULL, "XFrotz",
                      options, XtNumber(options), &argc, argv);
  if (dpy == NULL)
    os_fatal("Could not open display.");
  if (argc != 2)
    os_fatal("Usage: xfrotz [options] storyfile");
  story_name = argv[1];

  XtGetApplicationNameAndClass(dpy, &x_name, &x_class);

  class_buf = malloc(strlen(x_class) + 20);
  strcpy(class_buf, x_class);
  class_append = strchr(class_buf, 0);
  name_buf = malloc(strlen(x_name) + 20);
  strcpy(name_buf, x_name);
  name_append = strchr(name_buf, 0);

  for (i = 0; i < XtNumber(vars); i++) {
    strcpy(class_append, vars[i].class);
    strcpy(name_append, vars[i].name);
    if (XrmGetResource(XtDatabase(dpy), name_buf, class_buf,
                       &str_type_return, &value))
      vars[i].parser((char *) value.addr, vars[i].ptr);
  }
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

Display *dpy;
Window main_window = 0;
const XFontStruct *current_font_info;
GC normal_gc, reversed_gc, bw_gc, cursor_gc, current_gc;

void os_init_screen (void)
{
  XSetWindowAttributes window_attrs;
  XSizeHints *size_hints;
  XWMHints *wm_hints;
  XClassHint *class_hint;
  const char *story_basename;
  char *window_title;
  int font_width, font_height;
  XGCValues gc_setup;

  /* First, configuration parameters get set up */
  if (h_version == V3 && user_tandy_bit != 0)
    h_config |= CONFIG_TANDY;
  if (h_version == V3)
    h_config |= CONFIG_SPLITSCREEN | CONFIG_PROPORTIONAL;
  if (h_version >= V4)
    h_config |= CONFIG_BOLDFACE | CONFIG_EMPHASIS | CONFIG_FIXED;
  if (h_version >= V5) {
    h_flags |= GRAPHICS_FLAG | UNDO_FLAG | MOUSE_FLAG | COLOUR_FLAG;
#ifdef NO_SOUND
    h_flags &= ~SOUND_FLAG;
#else
    h_flags |= SOUND_FLAG;
#endif
  }
  if (h_version >= V6) {
    h_config |= CONFIG_PICTURES;
    h_flags &= ~MENU_FLAG;
  }

  if (h_version >= V5 && (h_flags & UNDO_FLAG) && option_undo_slots == 0)
    h_flags &= ~UNDO_FLAG;

  h_interpreter_number = INTERP_DEC_20;
  h_interpreter_version = 'F';

  x_init_colour(user_bg, user_fg);

  window_attrs.background_pixel = def_bg_pixel;
  window_attrs.backing_store = Always /* NotUseful */;
  window_attrs.save_under = FALSE;
  window_attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask;
  window_attrs.override_redirect = FALSE;
  main_window = XCreateWindow(dpy, DefaultRootWindow(dpy),
                              0, 0, 800, 600, 0, CopyFromParent,
                              InputOutput, CopyFromParent,
                              CWBackPixel | CWBackingStore |
                              CWOverrideRedirect | CWSaveUnder | CWEventMask,
                              &window_attrs);

  size_hints = XAllocSizeHints();
  size_hints->min_width = size_hints->max_width = size_hints->base_width = 800;
  size_hints->min_height = size_hints->max_height =
    size_hints->base_height = 600;
  size_hints->flags = PMinSize | PMaxSize | PBaseSize;

  wm_hints = XAllocWMHints();
  wm_hints->input = TRUE;
  wm_hints->initial_state = NormalState;
  wm_hints->flags = InputHint | StateHint;

  class_hint = XAllocClassHint();
  class_hint->res_name = x_name;
  class_hint->res_class = x_class;

  story_basename = strrchr(story_name, '/');
  if (story_basename == NULL)
    story_basename = story_name;
  else
    story_basename++;
  window_title = malloc(strlen(story_basename) + 14);
  sprintf(window_title, "XFrotz (V%d): %s", h_version, story_basename);

  XmbSetWMProperties(dpy, main_window, window_title, story_basename,
                     saved_argv, saved_argc, size_hints, wm_hints, class_hint);

  XMapWindow(dpy, main_window);
  free(window_title);

  current_font_info = get_font(TEXT_FONT, 0);
  if (! current_font_info)
    os_fatal("Could not open default font");

  h_screen_width = 800;
  h_screen_height = 600;
  os_font_data(FIXED_WIDTH_FONT, &font_height, &font_width);
  h_font_height = font_height; h_font_width = font_width;
  h_screen_cols = 800 / h_font_width;
  h_screen_rows = 600 / h_font_height;

  fg_pixel = def_fg_pixel;
  bg_pixel = def_bg_pixel;

  gc_setup.function = GXcopy;
  gc_setup.foreground = fg_pixel;
  gc_setup.background = bg_pixel;
  gc_setup.fill_style = FillSolid;
  gc_setup.font = current_font_info->fid;
  normal_gc = XCreateGC(dpy, main_window,
                        GCFunction | GCForeground | GCBackground |
                        GCFillStyle | GCFont, &gc_setup);
  gc_setup.foreground = bg_pixel;
  gc_setup.background = fg_pixel;
  reversed_gc = XCreateGC(dpy, main_window,
                          GCFunction | GCForeground | GCBackground |
                          GCFillStyle | GCFont, &gc_setup);
  gc_setup.foreground = ~0UL;
  gc_setup.background = 0UL;
  bw_gc = XCreateGC(dpy, main_window,
                    GCFunction | GCForeground | GCBackground |
                    GCFillStyle | GCFont, &gc_setup);
  gc_setup.background = 0UL;
  gc_setup.foreground = bg_pixel ^ fg_pixel;
  gc_setup.function = GXxor;
  cursor_gc = XCreateGC(dpy, main_window,
                        GCFunction | GCForeground | GCBackground |
                        GCFillStyle, &gc_setup);
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
  print_string("[Hit any key to exit.]");
  flush_buffer();
  os_read_key(0, FALSE);
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
    return time(0) & 0x7fff;
  else
    return user_random_seed;
}/* os_random_seed */

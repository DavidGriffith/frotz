


This is the source distribution of DOS Frotz 2.32. Sorry,
documentation is not complete yet, and this small readme
file is all you get at the moment...



The latest changes in the 2.32 interface are:

-> Text and keys (except for file names) are now presented
   using the new "zchar" type. This is a 1-byte value that
   contains an ISO Latin-1 character or a special Infocom
   character like code $09 (paragraph indentation). See
   FROTZ.H for possible character codes. Some Infocom codes
   have changed, too, since they collided with ISO Latin-1
   codes. I strongly recommend using the constants defined
   in FROTZ.H.

   If you want to support different character sets, look at

   * zchar definition (FROTZ.H),
   * z_print_unicode (TEXT.C),
   * z_check_unicode (TEXT.C),
   * translate_from_zscii (TEXT.C),
   * translate_to_zscii (TEXT.C),
   * conversion to lower case in z_read (INPUT.C),
   * script_char (FILES.C).

   If you can't read accented characters from the keyboard,
   make sure z_check_unicode stores 1 for codes from $0a to
   $ff. If you can't write accented characters, prints some
   suitable ASCII representation instead.

-> There are some differences between Amiga and DOS picture
   files, and an interpreter must take some extra care to
   combine Amiga story files with DOS picture files. So far
   the Amiga and DOS front-ends contained the necessary
   code to make this work. Since Frotz 2.32 this problem is
   dealt with in SCREEN.C, and interfaces no longer have to
   worry about this.

-> os_cursor_on and os_cursor_off are no more. Instead, an
   additional argument tells os_read_key to make the cursor
   visible or not.

   (I believe a game would not want to turn off the cursor
   for reading a string.)

-> os_wait_sample is no longer needed!

   SOUND.C uses the end_of_sound call to solve all problems
   with 'The Lurking Horror' more effectively than before.

-> os_start_sample should now ignore the play-once-or-loop-
   forever information in the sound files and rely on its
   arguments instead. The necessary information for 'The
   Lurking Horror' has been wired into SOUND.C. This should
   make it easier to use a more common sound format in the
   future.

-> 'Journey' asked for font #4 (fixed font) on most systems
   since it simply expected the interpreter to be unable to
   print a proportional font.  This unpleasant behaviour is
   now overwritten by Frotz.

-> Many extern declarations in FROTZ.H have been removed. In
   particular, this includes

   * end_of_sound,
   * restart_header,
   * resize_screen,
   * completion,
   * is_terminator,
   * read_yes_or_no,
   * read_string.

   It's fine to call these functions from your front-end,
   though. Just add their prototypes to your code.

-> For different reasons, several front-ends felt the need
   to interfere with the process of restarting a game.

   There is now an interface function os_restart_game that
   gives the front-end a chance to do its business while
   the game restarts. The function is called several times
   during the process, its sole argument indicating the
   current "stage" of the restart:

   * RESTART_BEGIN -- restart has just begun
   * RESTART_WPROP_SET -- window properties have been set
   * RESTART_END -- restart completed

   os_restart_game is also useful to display the 'Beyond
   Zork' title picture that is now available in MG1 format.



Final note: The FONT.DAT file in this distribution must be
appended to the DOS executable -- sorry about this!



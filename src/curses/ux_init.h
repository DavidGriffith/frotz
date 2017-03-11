/* Declarations for ux_init.
   Copyright (C) 1989, 1990, 1991, 1992, 1993 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef __UNISTD_LOADED
#ifndef _UX_INIT_H
#define _UX_INIT_H 1

#ifdef	__cplusplus
extern "C" {
#endif

void os_fatal (const char *s, ...);
void os_process_arguments (int argc, char *argv[]);
void os_init_screen (void);
void os_reset_screen (void);
void os_restart_game (int stage);
int os_random_seed (void);
FILE *os_path_open(const char *name, const char *mode);
FILE *os_load_story(void);
int os_storyfile_seek(FILE * fp, long offset, int whence);
int os_storyfile_tell(FILE * fp);
FILE *pathopen(const char *name, const char *p, const char *mode, char *fullname);
int getconfig(char *configfile);
int getbool(char *value);
int getcolor(char *value);
int geterrmode(char *value);
void sigwinch_handler(int sig);
void sigint_handler(int dummy);
void redraw(void);
void os_init_setup(void);
int ux_init_blorb(void);

#ifdef	__cplusplus
}
#endif

#endif /* _UX_INIT_H */
#endif /* __UNISTD_LOADED */

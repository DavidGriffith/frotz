/*
 * ux_sample.c
 *
 * Unix interface, sound support
 *
 */

#define __UNIX_PORT_FILE

#ifdef USE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "frotz.h"
#include "ux_frotz.h"

/*
 * os_beep
 *
 * Play a beep sound. Ideally, the sound should be high- (number == 1)
 * or low-pitched (number == 2).
 *
 */

void os_beep (int number)
{

    beep();

}/* os_beep */

/*
 * os_prepare_sample
 *
 * Load the sample from the disk.
 *
 */

void os_prepare_sample (int number)
{

    /* Not implemented */

}/* os_prepare_sample */

/*
 * os_start_sample
 *
 * Play the given sample at the given volume (ranging from 1 to 8 and
 * 255 meaning a default volume). The sound is played once or several
 * times in the background (255 meaning forever). In Z-code 3 the
 * repeats value is always 0 and the number of repeats is taken from
 * the sound file itself. The end_of_sound function is called as soon
 * as the sound finishes.
 *
 */

void os_start_sample (int number, int volume, int repeats)
{

    /* Not implemented */

}/* os_start_sample */

/*
 * os_stop_sample
 *
 * Turn off the current sample.
 *
 */

void os_stop_sample (void)
{

    /* Not implemented */

}/* os_stop_sample */

/*
 * os_finish_with_sample
 *
 * Remove the current sample from memory (if any).
 *
 */

void os_finish_with_sample (void)
{

    /* Not implemented */

}/* os_finish_with_sample */

/*
 * os_wait_sample
 *
 * Stop repeating the current sample and wait until it finishes.
 *
 */

void os_wait_sample (void)
{

    /* Not implemented */

}/* os_wait_sample */

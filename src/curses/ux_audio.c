/*
 * ux_audio_none.c - Unix interface, sound support
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#define __UNIX_PORT_FILE

#ifdef USE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ao/ao.h>
#include <sndfile.h>
#include <math.h>

#include "ux_frotz.h"
#include "ux_blorb.h"

enum {SFX_TYPE, MOD_TYPE};

typedef struct EFFECT {
    void (*destroy)(struct EFFECT *);
    int number;
    int type;
    int active;
    int voice;
    int *buffer;
    int buflen;
    int repeats;
    int volume;
    int ended;
    zword eos;
    ulong endtime;
} EFFECT;

/* no effects cache */

static EFFECT *e_sfx = NULL;
static EFFECT *e_mod = NULL;


int audiorunning = 0;

static ao_device *device;

static EFFECT *getaiff(FILE *, size_t, int, int);
static EFFECT *geteffect(int);
static EFFECT *new_effect(int, int);

/*
int ux_initsound(void)
{
    ao_sample_format format;

    int default_driver;

    if (audiorunning) return 1;
    if (!f_setup.sound) return 0;
    audiorunning = 1;

    ao_initialize();

    default_driver = ao_default_driver_id();

    memset(&format, 0, sizeof(format));

    format.bits = 16;
    format.channels = 2;
    format.rate = 44100;
    format.byte_format = AO_FMT_BIG;

    device = ao_open_live(default_driver, &format, NULL);
    if (device == NULL) {
	printf("Error opening audio device.\n");
	exit(1);
    }
    printf("Audio ready.\n");
    return 1;
}

void ux_stopsound(void)
{
    ao_close(device);
    ao_shutdown();
}
*/

static EFFECT *geteffect(int num)
{
    myresource res;
    EFFECT *result = NULL;
    unsigned int id;

    if ((e_sfx) && (e_sfx->number == num)) return e_sfx;
    if ((e_mod) && (e_mod->number == num)) return e_mod;

    if (ux_getresource(num,0,bb_method_FilePos,&res) != bb_err_None)
	return NULL;

    /* Look for a recognized format. */

    id = res.type;

    if (id == bb_make_id('F','O','R','M')) {
	result = getaiff(res.fp, res.bbres.data.startpos, res.bbres.length, num);
    }
/*
    else if (id == bb_make_id('M','O','D',' ') {
	result = getmodule(res.file, res.bbres.data.startpos, res.bbres.length, num);
    } else {
	result = getogg(res.file, res.bbres.data.startpos, res.bbres.length, num);
    }
*/
    ux_freeresource(&res);
    return result;
}


static void baredestroy(EFFECT * r)
{
    if (r) {
	if (r->buffer != NULL) free(r->buffer);
	free(r);
    }
}


static EFFECT *new_effect(int type, int num)
{
    EFFECT * reader = (EFFECT *)calloc(1, sizeof(EFFECT));

    if (reader) {
	reader->type = type;
	reader->number = num;
 	reader->destroy = baredestroy;
    }
    return (EFFECT *)reader;
}


static EFFECT *getaiff(FILE *fp, size_t pos, int len, int num)
{
    ao_device *device;
    ao_sample_format format;
    int default_driver;
    SNDFILE     *sndfile;
    SF_INFO     sf_info;

    EFFECT *sample;
    void *data;
    int size;
    int count;

    int frames_read;

    int *buffer;


    sample = new_effect(SFX_TYPE, num);
    if (sample == NULL || sample == 0)
	return sample;

    if (fseek(fp, pos, SEEK_SET) != 0)
       return NULL;

    ao_initialize();
    default_driver = ao_default_driver_id();

    sf_info.format = 0;
    sndfile = sf_open_fd(fileno(fp), SFM_READ, &sf_info, 1);

    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 32;
    format.channels = sf_info.channels;
    format.rate = sf_info.samplerate;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        printf("Error opening sound device.\n");
        exit(1);
    }

    buffer = malloc(sizeof(int) * sf_info.frames * sf_info.channels);

    sf_read_int(sndfile, buffer, sf_info.frames);

    ao_play(device, (char *)buffer, sf_info.frames * sizeof(int));
    ao_close(device);
    ao_shutdown();

    sf_close(sndfile);

/*
    while (count <= len) {
	fread(sample->buffer + count, 1, 1, f);
	count++;
    }

    sample->buflen = count;
*/
    return sample;
}

static void startsample()
{
/*
    if (e_sfx == NULL) return;
    ao_play(device, e_sfx->buffer, e_sfx->buflen);
*/
}

static void stopsample()
{
    /* nothing yet */
}

/*
 * os_beep
 *
 * Play a beep sound. Ideally, the sound should be high- (number == 1)
 * or low-pitched (number == 2).
 *
 */

void os_beep (int number)
{
//    beep();

    ao_sample_format format;
    char *buffer;
    int buf_size;
    int sample;
    float freq;
    int i;

    if (number == 1)
	freq = 880.0;
    else
	freq = 440.0;

    format.bits = 16;
    format.channels = 2;
    format.rate = 44100;
    format.byte_format = AO_FMT_LITTLE;

    buf_size = format.bits/8 * format.channels * format.rate;
    buffer = calloc(buf_size, sizeof(char));

    for (i = 0; i < format.rate; i++) {
	sample = (int)(0.75 * 32768.0 *
		sin(2 * M_PI * freq * ((float) i/format.rate)));

	/* Put the same stuff in left and right channel */
	buffer[4*i] = buffer[4*i+2] = sample & 0xff;
	buffer[4*i+1] = buffer[4*i+3] = (sample >> 8) & 0xff;
    }
    ao_play(device, buffer, buf_size);

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

void os_start_sample (int number, int volume, int repeats, zword eos)
{
    EFFECT *e;

//    if (!audiorunning) return;
    e = geteffect(number);
    if (e == NULL) return;
//    if (e->type == SFX_TYPE) stopsample();
    stopsample();
    if (repeats < 1) repeats = 1;
    if (repeats == 255) repeats = -1;
    if (volume < 0) volume = 0;
    if (volume > 8) volume = 8;
    if (e->type == SFX_TYPE && repeats > 0) repeats--;
    e->repeats = repeats;
    e->volume = 32*volume;
    e->eos = eos;
    e->ended = 0;
    if (e->type == SFX_TYPE) {
	if ((e_sfx) && (e_sfx != e)) e_sfx->destroy(e_sfx);
	e_sfx = e;
	startsample();
    }
}/* os_start_sample */

/*
 * os_stop_sample
 *
 * Turn off the current sample.
 *
 */

void os_stop_sample (int number)
{

    /* Not implemented */

}/* os_stop_sample */

/*
 * os_finish_with_sample
 *
 * Remove the current sample from memory (if any).
 *
 */

void os_finish_with_sample (number)
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


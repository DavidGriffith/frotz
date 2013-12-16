/*
 * ux_audio.c - Unix interface, sound support
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

#ifndef NO_SOUND

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#ifdef USE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include <ao/ao.h>
#include <sndfile.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <libmodplug/modplug.h>

#include "ux_frotz.h"

#define BUFFSIZE 4096

static int playaiff(FILE *, bb_result_t, int, int);
static int playmod(FILE *, bb_result_t, int, int);
static int playogg(FILE *, bb_result_t, int, int);

static int mypower(int, int);
static char *getfiledata(FILE *, long *);
static void sigchld_handler(int);

static pid_t sfx_pid;
static pid_t music_pid;

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
 * Actually it's more efficient these days to load and play a sound in
 * the same operation.  This function therefore does nothing.
 *
 */

void os_prepare_sample (int number)
{
    return;
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
    bb_result_t resource;


    if (bb_err_None != bb_load_resource(blorb_map, bb_method_FilePos, &resource, bb_ID_Snd, number))
	return;

    if (blorb_map->chunks[resource.chunknum].type == bb_make_id('F','O','R','M')) {
	playaiff(blorb_fp, resource, volume, repeats);
    }

    return;
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


/*
 * These functions are internal to ux_audio.c
 *
 */


/*
 * mypower
 *
 * Just a simple recursive integer-based power function because I don't
 * want to use the floating-point version from libm.
 *
 */
static int mypower(int base, int exp) {
    if (exp == 0)
        return 1;
    else if (exp % 2)
        return base * mypower(base, exp - 1);
    else {
        int temp = mypower(base, exp / 2);
        return temp * temp;
    }
}

static void sigchld_handler(int signal) {
    int status;
    struct sigaction sa;
    int dead_child;

    dead_child = wait(&status);

    if (dead_child == sfx_pid)
	sfx_pid = 0;
    else if (dead_child == music_pid)
	music_pid = 0;

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);
}

/*
 * playaiff
 *
 * This function takes a file pointer to a Blorb file and a bb_result_t
 * struct describing what chunk to play.  It's up to the caller to make
 * sure that an AIFF chunk is to be played.  Volume and repeats are also
 * handled here.
 *
 * This function should be able to play OGG chunks, but for some strange
 * reason, libsndfile refuses to load them.  Libsndfile is capable of
 * loading and playing OGGs when they're naked file.  I don't see what
 * the big problem is.
 *
 */
int playaiff(FILE *fp, bb_result_t result, int vol, int repeats)
{
    int default_driver;
    int frames_read;
    int count;
    int toread;
    int *buffer;
    long filestart;

    int volcount;
    int volfactor;

    ao_device *device;
    ao_sample_format format;

    SNDFILE     *sndfile;
    SF_INFO     sf_info;

    sigset_t sigchld_mask;
    struct sigaction sa;

    sfx_pid = fork();

    if (sfx_pid < 0) {
	perror("fork");
	return 1;
    }

    if (sfx_pid > 0) {

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGCHLD, &sa, NULL);
	return 0;
    }

    sigprocmask(SIG_UNBLOCK, &sigchld_mask, NULL);

    ao_initialize();
    default_driver = ao_default_driver_id();

    sf_info.format = 0;

    filestart = ftell(fp);
    lseek(fileno(fp), result.data.startpos, SEEK_SET);
    sndfile = sf_open_fd(fileno(fp), SFM_READ, &sf_info, 0);
    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 32;
    format.channels = sf_info.channels;
    format.rate = sf_info.samplerate;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
        return 1;
    }

    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;
    volfactor = mypower(2, -vol + 8);

    buffer = malloc(BUFFSIZE * sf_info.channels * sizeof(int));
    frames_read = 0;
    toread = sf_info.frames * sf_info.channels;

    while (toread > 0) {
	if (toread < BUFFSIZE * sf_info.channels)
	    count = toread;
	else
	    count = BUFFSIZE * sf_info.channels;
        frames_read = sf_read_int(sndfile, buffer, count);
	for (volcount = 0; volcount <= frames_read; volcount++)
	    buffer[volcount] /= volfactor;
        ao_play(device, (char *)buffer, frames_read * sizeof(int));
	toread = toread - frames_read;
    }

    free(buffer);
    fseek(fp, filestart, SEEK_SET);
    ao_close(device);
    sf_close(sndfile);
    ao_shutdown();

    exit(0);
}


/*
 * playmod
 *
 * This function takes a file pointer to a Blorb file and a bb_result_t
 * struct describing what chunk to play.  It's up to the caller to make
 * sure that a MOD chunk is to be played.  Volume and repeats are also
 * handled here.
 *
 */
static int playmod(FILE *fp, bb_result_t result, int vol, int repeats)
{
    unsigned char *buffer;
    int modlen;

    int default_driver;
    ao_device *device;
    ao_sample_format format;

    char *filedata;
    long size;
    ModPlugFile *mod;
    ModPlug_Settings settings;

    long original_offset;

    original_offset = ftell(fp);
    fseek(fp, result.data.startpos, SEEK_SET);

    ao_initialize();
    default_driver = ao_default_driver_id();

    ModPlug_GetSettings(&settings);

    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 16;
    format.channels = 2;
    format.rate = 44100;

    /* Note: All "Basic Settings" must be set before ModPlug_Load. */
    settings.mResamplingMode = MODPLUG_RESAMPLE_FIR; /* RESAMP */
    settings.mChannels = 2;
    settings.mBits = 16;
    settings.mFrequency = 44100;
    settings.mStereoSeparation = 128;
    settings.mMaxMixChannels = 256;

    /* insert more setting changes here */
    ModPlug_SetSettings(&settings);

    /* remember to free() filedata later */
    filedata = getfiledata(fp, &size);

    mod = ModPlug_Load(filedata, size);
    if (!mod) {
//	printf("Unable to load module\n");
	free(filedata);
	return 1;
    }

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
//        printf("Error opening sound device.\n");
        return 1;
    }

    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;

    ModPlug_SetMasterVolume(mod, mypower(2, vol));

    buffer = malloc(BUFFSIZE * sizeof(char));
    modlen = 1;
    while (modlen != 0) {
	if (modlen == 0) break;
	modlen = ModPlug_Read(mod, buffer, BUFFSIZE * sizeof(char));
	if (modlen > 0 && ao_play(device, (char *) buffer, modlen * sizeof(char)) == 0) {
	    perror("audio write");
	    exit(1);
	}
    }
    free(buffer);
    ao_close(device);
    ao_shutdown();

    fseek(fp, original_offset, SEEK_SET);

    exit(0);
}

/*
 * getfiledata
 *
 * libmodplug requires the whole file to be pulled into memory.
 * This function does that and then closes the file.
 */
static char *getfiledata(FILE *fp, long *size)
{
    char *data;
    long offset;

    offset = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    (*size) = ftell(fp);
    fseek(fp, offset, SEEK_SET);
    data = (char*)malloc(*size);
    fread(data, *size, sizeof(char), fp);
    fseek(fp, offset, SEEK_SET);
    return(data);
}


/*
 * playogg
 *
 * This function takes a file pointer to a Blorb file and a bb_result_t
 * struct describing what chunk to play.  It's up to the caller to make
 * sure that an OGG chunk is to be played.  Volume and repeats are also
 * handled here.
 *
 * This function invoked libvorbisfile directly rather than going
 * through libsndfile.  The reason for this is that libsndfile refuses
 * to load an OGG file that's embedded in a Blorb file.
 *
 */
static int playogg(FILE *fp, bb_result_t result, int vol, int repeats)
{
    ogg_int64_t toread;
    ogg_int64_t frames_read;
    ogg_int64_t count;

    vorbis_info *info;

    OggVorbis_File vf;
    int current_section;
    void *buffer;

    int default_driver;
    ao_device *device;
    ao_sample_format format;

    int volcount;
    int volfactor;

    ao_initialize();
    default_driver = ao_default_driver_id();

    fseek(fp, result.data.startpos, SEEK_SET);

    memset(&format, 0, sizeof(ao_sample_format));

    if (ov_open_callbacks(fp, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0) {
//	printf("Oops.\n");
	exit(1);
    }

    info = ov_info(&vf, -1);

    format.byte_format = AO_FMT_LITTLE;
    format.bits = 16;
    format.channels = info->channels;
    format.rate = info->rate;

    device = ao_open_live(default_driver, &format, NULL /* no options */);
    if (device == NULL) {
//        printf("Error opening sound device.\n");
	ov_clear(&vf);
        return 1;
    }

    if (vol < 1) vol = 1;
    if (vol > 8) vol = 8;
    volfactor = mypower(2, -vol + 8);

    buffer = malloc(BUFFSIZE * format.channels * sizeof(int16_t));

    frames_read = 0;
    toread = ov_pcm_total(&vf, -1) * 2 * format.channels;
    count = 0;

    while (count < toread) {
	frames_read = ov_read(&vf, (char *)buffer, BUFFSIZE, 0,2,1,&current_section);
	for (volcount = 0; volcount <= frames_read / 2; volcount++)
	    ((int16_t *) buffer)[volcount] /= volfactor;
	ao_play(device, (char *)buffer, frames_read * sizeof(char));
	count += frames_read;
    }

    ao_close(device);
    ao_shutdown();
    ov_clear(&vf);

    free(buffer);

    exit(0);
}

#endif /* NO_SOUND */

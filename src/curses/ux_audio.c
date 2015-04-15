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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#ifdef USE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include "ux_frotz.h"

#ifndef NO_SOUND

#include <ao/ao.h>
#include <sndfile.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <libmodplug/modplug.h>

//#define BUFFSIZE 4096
#define BUFFSIZE 512
#define SAMPLERATE 44100
#define MAX(x,y) ((x)>(y)) ? (x) : (y)
#define MIN(x,y) ((x)<(y)) ? (x) : (y)

typedef struct {
    FILE *fp;
    bb_result_t result;
    int vol;
    int repeats;
} EFFECT;

static void *playaiff(EFFECT *);
static int playmod(EFFECT);
static int playogg(EFFECT);
static void floattopcm16(short *, float *, int);
static void pcm16tofloat(float *, short *, int);

static int mypower(int, int);
static char *getfiledata(FILE *, long *);
static void *mixer(void *);

static pthread_t	mixer_id;
static pthread_t	playaiff_id;
static pthread_mutex_t	mutex;
static sem_t		audio_full;
static sem_t		audio_empty;

bool    bleep_playing = FALSE;
bool	bleep_stop = FALSE;

float	*musicbuffer;

float	*bleepbuffer;
int	bleepchannels;
int	bleeprate;

int	musiccount;
int	bleepcount;



/*
 * os_init_sound
 *
 * Do any required setup for sound output.
 * Here we start a thread to act as a mixer.
 *
 */
void os_init_sound(void)
{
    int err;

    musicbuffer = malloc(BUFFSIZE * 2 * sizeof(float));
    if (musicbuffer == NULL) {
	printf("Unable to malloc musicbuffer\n");
	exit(1);
    }

    bleepbuffer = malloc(BUFFSIZE * 2 * sizeof(float));
    if (bleepbuffer == NULL) {
	printf("Unable to malloc bleepbuffer\n");
	exit(1);
    }

    err = pthread_create(&(mixer_id), NULL, &mixer, NULL);
    if (err != 0) {
	printf("Can't create mixer thread :[%s]", strerror(err));
	exit(1);
    }

    sem_post(&audio_empty);
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
    EFFECT myeffect;
    int err;
    static pthread_attr_t attr;


    if (blorb_map == NULL) return;

    if (bb_err_None != bb_load_resource(blorb_map, bb_method_FilePos, &resource, bb_ID_Snd, number))
	return;

    myeffect.fp = blorb_fp;
    myeffect.result = resource;
    myeffect.vol = volume;
    myeffect.repeats = repeats;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (blorb_map->chunks[resource.chunknum].type == bb_make_id('F','O','R','M')) {
	err = pthread_create(&playaiff_id, &attr, (void *) &playaiff, &myeffect);
	if (err != 0) {
	    printf("Can't create playaiff thread :[%s]", strerror(err));
	    exit(1);
	}

    } else if (blorb_map->chunks[resource.chunknum].type == bb_make_id('M','O','D',' ')) {
	playmod(myeffect);
    } else if (blorb_map->chunks[resource.chunknum].type == bb_make_id('O','G','G','V')) {
	playogg(myeffect);
    } else {
	/* Something else was in there.  Ignore it. */
    }

    /* If I don't have this usleep() here, Frotz will segfault. Why?*/
    usleep(0);
}/* os_start_sample */

/*
 * os_stop_sample
 *
 * Turn off the current sample.
 *
 */

void os_stop_sample (int number)
{
    if (bleep_playing) {
	bleep_stop = TRUE;
    }
    return;
}/* os_stop_sample */

/*
 * os_finish_with_sample
 *
 * Remove the current sample from memory (if any).
 *
 */

void os_finish_with_sample (int number)
{
    os_stop_sample(number);

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
 * mixer
 *
 * In a classic producer/consumer arrangement, this mixer watches for audio
 * data to be placed in *bleepbuffer or *musicbuffer.  When a semaphore for
 * either is raised, the mixer processes the buffer.
 *
 * Data presented to the mixer must be floats at 44100hz
 *
 */

static void *mixer(void *arg)
{
    short *shortbuffer;
    int default_driver;
    ao_device *device;
    ao_sample_format format;

    ao_initialize();
    default_driver = ao_default_driver_id();

    shortbuffer = malloc(BUFFSIZE * sizeof(short) * 2);
    memset(&format, 0, sizeof(ao_sample_format));

    format.byte_format = AO_FMT_NATIVE;
    format.bits = 16;

    device = NULL;

    while (1) {
        sem_wait(&audio_full);          /* Wait until output buffer is full */
        pthread_mutex_lock(&mutex);     /* Acquire mutex */

	format.channels = bleepchannels;
	format.rate = bleeprate;
	if (bleep_playing && device == NULL) {
	    device = ao_open_live(default_driver, &format, NULL);
	    if (device == NULL) {
	        printf(" Error opening sound device.\n");
	    }
	}

        floattopcm16(shortbuffer, bleepbuffer, BUFFSIZE * 2);
        ao_play(device, (char *) shortbuffer, bleepcount * 2);

	if (!bleep_playing) {
	    ao_close(device);
	    device = NULL;
	}

        pthread_mutex_unlock(&mutex);   /* release the mutex lock */
        sem_post(&audio_empty);         /* signal empty */
    }
}


/* Convert back to shorts */
static void floattopcm16(short *outbuf, float *inbuf, int length)
{
    int   count;

    const float mul = (32768.0f);
    for (count = 0; count <= length; count++) {
	int32_t tmp = (int32_t)(mul * inbuf[count]);
	tmp = MAX( tmp, -32768 ); // CLIP < 32768
	tmp = MIN( tmp, 32767 );  // CLIP > 32767
	outbuf[count] = tmp;
    }
}


/* Convert the buffer to floats. (before resampling) */
void pcm16tofloat(float *outbuf, short *inbuf, int length)
{
    int   count;

    const float div = (1.0f/32768.0f);
    for (count = 0; count <= length; count++) {
	outbuf[count] = div * (float) inbuf[count];
    }
}


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
void *playaiff(EFFECT *raw_effect)
{
    int frames_read;
    int count;
    sf_count_t toread;
    long filestart;

    int volcount;
    int volfactor;

    SNDFILE     *sndfile;
    SF_INFO     sf_info;

    EFFECT myeffect = *raw_effect;

    sf_info.format = 0;

    filestart = ftell(myeffect.fp);
    lseek(fileno(myeffect.fp), myeffect.result.data.startpos, SEEK_SET);
    sndfile = sf_open_fd(fileno(myeffect.fp), SFM_READ, &sf_info, 0);

    if (myeffect.vol < 1) myeffect.vol = 1;
    if (myeffect.vol > 8) myeffect.vol = 8;
    volfactor = mypower(2, -myeffect.vol + 8);

    frames_read = 0;
    toread = sf_info.frames * sf_info.channels;

    bleepchannels = sf_info.channels;
    bleeprate = sf_info.samplerate;

    bleep_playing = TRUE;
    while (toread > 0) {
	if (bleep_stop) break;

	sem_wait(&audio_empty);
	pthread_mutex_lock(&mutex);

	if (toread < BUFFSIZE * sf_info.channels)
	    count = toread;
	else
	    count = BUFFSIZE * sf_info.channels;
        frames_read = sf_read_float(sndfile, bleepbuffer, count);
	for (volcount = 0; volcount <= frames_read; volcount++) {
	    bleepbuffer[volcount] /= volfactor;
	}

	bleepcount = frames_read;
	toread = toread - frames_read;

	pthread_mutex_unlock(&mutex);
	sem_post(&audio_full);
    }
    bleep_stop = FALSE;
    bleep_playing = FALSE;

    fseek(myeffect.fp, filestart, SEEK_SET);
    sf_close(sndfile);

    pthread_exit((void*) raw_effect);
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
static int playmod(EFFECT myeffect)
{
    unsigned char *buffer;
    unsigned char *mixbuffer;

    int modlen;
    int count;

    int default_driver;
    ao_device *device;
    ao_sample_format format;

    char *filedata;
    long size;
    ModPlugFile *mod;
    ModPlug_Settings settings;

    long original_offset;


    original_offset = ftell(myeffect.fp);
    fseek(myeffect.fp, myeffect.result.data.startpos, SEEK_SET);

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
    filedata = getfiledata(myeffect.fp, &size);

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

    if (myeffect.vol < 1) myeffect.vol = 1;
    if (myeffect.vol > 8) myeffect.vol = 8;

    ModPlug_SetMasterVolume(mod, mypower(2, myeffect.vol));

    buffer = malloc(BUFFSIZE * sizeof(char));
    mixbuffer = malloc(BUFFSIZE * sizeof(char));

    modlen = 1;
    while (modlen != 0) {
	if (modlen == 0) break;
	modlen = ModPlug_Read(mod, buffer, BUFFSIZE * sizeof(char));
	if (modlen > 0) {
	    if (ao_play(device, (char *) buffer, modlen * sizeof(char)) == 0) {
		perror("audio write");
		exit(1);
	    }
	}
    }
    free(buffer);
    ao_close(device);
    ao_shutdown();

    fseek(myeffect.fp, original_offset, SEEK_SET);

    return(2);
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
static int playogg(EFFECT myeffect)
{
    ogg_int64_t toread;
    ogg_int64_t frames_read;
    ogg_int64_t count;

    vorbis_info *info;

    OggVorbis_File vf;
    int current_section;
    short *buffer;
    float *floatbuffer;
    float *floatbuffer2;

    int default_driver;
    ao_device *device;
    ao_sample_format format;

    int volcount;
    int volfactor;

    ao_initialize();
    default_driver = ao_default_driver_id();

    fseek(myeffect.fp, myeffect.result.data.startpos, SEEK_SET);

    memset(&format, 0, sizeof(ao_sample_format));

    if (ov_open_callbacks(myeffect.fp, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0) {
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

    if (myeffect.vol < 1) myeffect.vol = 1;
    if (myeffect.vol > 8) myeffect.vol = 8;
    volfactor = mypower(2, -myeffect.vol + 8);

    buffer = malloc(BUFFSIZE * format.channels * sizeof(short));
    floatbuffer = malloc(BUFFSIZE * format.channels * sizeof(float));
    floatbuffer2 = malloc(BUFFSIZE * format.channels * sizeof(float));

    frames_read = 0;
    toread = ov_pcm_total(&vf, -1) * 2 * format.channels;
    count = 0;

    while (count < toread) {
	frames_read = ov_read(&vf, (char *)buffer, BUFFSIZE, 0,2,1,&current_section);
	pcm16tofloat(floatbuffer, buffer, frames_read);
	for (volcount = 0; volcount <= frames_read / 2; volcount++) {
	    ((float *) floatbuffer)[volcount] /= volfactor;
	}
	floattopcm16(buffer, floatbuffer, frames_read);

	ao_play(device, (char *)buffer, frames_read * sizeof(char));
	count += frames_read;
    }

    ao_close(device);
    ao_shutdown();
    ov_clear(&vf);

    free(buffer);

    return(2);
}

#endif /* NO_SOUND */

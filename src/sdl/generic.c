/**
 * @file generic.c
 *
 * The functions here are identical or nearly so in most ports.  They should
 * really be put somewhere so that they can be shared.  For now, this.
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../blorb/blorb.h"
#include "../common/frotz.h"
#include "generic.h"

FILE *blorb_fp;
bb_result_t blorb_res;
bb_map_t *blorb_map;

/*
 * isblorb
 *
 * Returns 1 if this file is a Blorb file, 0 if not.
 */
static int isblorb(FILE *fp)
{
    char mybuf[4];

    if (fp == NULL)
        return 0;

    fread(mybuf, 1, 4, fp);
    if (strncmp(mybuf, "FORM", 4))
        return 0;

    fseek(fp, 4, SEEK_CUR);
    fread(mybuf, 1, 4, fp);

    if (strncmp(mybuf, "IFRS", 4))
        return 0;

    return 1;
}


/*
 * gen_blorb_init
 *
 * Check if we're opening a Blorb file directly.  If not, check
 * to see if there's a seperate Blorb file that looks like it goes
 * along with this Zcode file.  If we have a Blorb file one way or the
 * other, make a Blorb map.  If we opened a Blorb file directly, that
 * means that our executable is in that file and therefore we will look
 * for a ZCOD chunk and record its location so os_load_story() can find it.
 * Make sure the Blorb file is opened and with the file pointer blorb_fp.
 */
bb_err_t gen_blorb_init(char *filename)
{
    FILE *fp;
    char *p;
    char *mystring;
    int  len1;
    int  len2;

    bb_err_t blorb_err;

    blorb_map = NULL;

    if ((fp = fopen(filename, "rb")) == NULL)
        return bb_err_Read;

    /* Is this really a Blorb file?  If not, maybe we're loading a naked
     * zcode file and our resources are in a seperate blorb file.
     */
    if (isblorb(fp)) {                  /* Now we know to look */
        f_setup.exec_in_blorb = 1;      /* for zcode in the blorb */
        blorb_fp = fp;
    } else {
        fclose(fp);
        len1 = strlen(filename) + strlen(EXT_BLORB);
        len2 = strlen(filename) + strlen(EXT_BLORB3);

        mystring = malloc(len2 * sizeof(char) + 1);
        strncpy(mystring, filename, len1 * sizeof(char));
        p = strrchr(mystring, '.');
        if (p != NULL)
            *p = '\0';

        strncat(mystring, EXT_BLORB, len1 * sizeof(char));

        /* Check if foo.blb is there. */
        if ((fp = fopen(mystring, "rb")) == NULL) {
            p = strrchr(mystring, '.');
            if (p != NULL)
                *p = '\0';
            strncat(mystring, EXT_BLORB3, len2 * sizeof(char));
            if (!(fp = fopen(mystring, "rb")))
                return bb_err_NoBlorb;
        }
        if (!isblorb(fp)) {
            fclose(fp);
            return bb_err_NoBlorb;
        }

        /* At this point we know that we're using a naked zcode file */
        /* with resources in a seperate Blorb file. */
        blorb_fp = fp;
        f_setup.use_blorb = 1;
    }

    /* Create a Blorb map from this file.
     * This will fail if the file is not a valid Blorb file.
     * From this map, we can now pick out any resource we need.
     */
    blorb_err = bb_create_map(blorb_fp, &blorb_map);
    if (blorb_err != bb_err_None)
        return bb_err_Format;

    /* Locate the EXEC chunk within the blorb file and record its
     * location so os_load_story() can find it.
     */
    if (f_setup.exec_in_blorb) {
        blorb_err = bb_load_chunk_by_type(blorb_map, bb_method_FilePos,
                &blorb_res, bb_make_id('Z','C','O','D'), 0);
        f_setup.exec_in_blorb = 1;
    }

    return blorb_err;
}

/*
 * os_load_story
 *
 * This is different from os_path_open() because we need to see if the
 * story file is actually a chunk inside a blorb file.  Right now we're
 * looking only at the exact path we're given on the command line.
 *
 * Open a file in the current directory.  If this fails, then search the
 * directories in the ZCODE_PATH environmental variable.  If that's not
 * defined, search INFOCOM_PATH.
 *
 */
FILE *os_load_story(void)
{
    FILE *fp;

    switch (gen_blorb_init(f_setup.story_file)) {
        case bb_err_NoBlorb:
//        printf("No blorb file found.\n\n");
          break;
        case bb_err_Format:
          printf("Blorb file loaded, but unable to build map.\n\n");
          break;
        case bb_err_NotFound:
          printf("Blorb file loaded, but lacks executable chunk.\n\n");
          break;
        case bb_err_None:
//        printf("No blorb errors.\n\n");
          break;
    }

    fp = fopen(f_setup.story_file, "rb");

    /* Is this a Blorb file containing Zcode? */
    if (f_setup.exec_in_blorb)
        fseek(fp, blorb_res.data.startpos, SEEK_SET);

    return fp;
}


/*
 * os_storyfile_seek
 *
 * Seek into a storyfile, either a standalone file or the
 * ZCODE chunk of a blorb file.
 *
 */
int os_storyfile_seek(FILE * fp, long offset, int whence)
{
    /* Is this a Blorb file containing Zcode? */
    if (f_setup.exec_in_blorb)
    {
        switch (whence)
        {
            case SEEK_END:
                return fseek(fp, blorb_res.data.startpos + blorb_res.length + offset, SEEK_SET);
                break;
            case SEEK_CUR:
                return fseek(fp, offset, SEEK_CUR);
                break;
            case SEEK_SET:
            default:
                return fseek(fp, blorb_res.data.startpos + offset, SEEK_SET);
                break;
        }
    }
    else
    {
        return fseek(fp, offset, whence);
    }

}


/*
 * os_storyfile_tell
 *
 * Tell the position in a storyfile, either a standalone file
 * or the ZCODE chunk of a blorb file.
 *
 */
int os_storyfile_tell(FILE * fp)
{
    /* Is this a Blorb file containing Zcode? */
    if (f_setup.exec_in_blorb)
    {
        return ftell(fp) - blorb_res.data.startpos;
    }
    else
    {
        return ftell(fp);
    }

}

/*
 * os_warn
 *
 * Display a warning message and continue with the game.
 *
 */
void os_warn (const char *s, ...)
{
    va_list va;
    char buf[1024];
    int len;

    //XXX Too lazy to do this right (try again with a bigger buf if necessary).
    va_start(va, s);
    len = vsnprintf(buf, sizeof(buf), s, va);
    va_end(va);
    /* Solaris 2.6's cc complains if the below cast is missing */
    os_display_string((zchar *)"\n\n");
    os_beep(BEEP_HIGH);
    os_set_text_style(BOLDFACE_STYLE);
    os_display_string((zchar *)"Warning: ");
    os_set_text_style(0);
    os_display_string((zchar *)buf);
    os_display_string((zchar *)"\n");
    if (len > sizeof(buf))
        os_display_string((zchar *)"(truncated)\n");
    new_line();
}

/* These are useful for circular buffers.
 */
#define RING_DEC( ptr, beg, end) (ptr > (beg) ? --ptr : (ptr = (end)))
#define RING_INC( ptr, beg, end) (ptr < (end) ? ++ptr : (ptr = (beg)))

#define MAX_HISTORY 256
static char *history_buffer[MAX_HISTORY];
static char **history_next = history_buffer; /* Next available slot. */
static char **history_view = history_buffer; /* What the user is looking at. */
#define history_end (history_buffer + MAX_HISTORY - 1)


/**
 * Add the given string to the next available history buffer slot.
 */
void gen_add_to_history(zchar *str)
{

    if (*history_next != NULL)
        free( *history_next);
    *history_next = (char *)malloc(strlen((char *)str) + 1);
    strcpy( *history_next, (char *)str);
    RING_INC( history_next, history_buffer, history_end);
    history_view = history_next; /* Reset user frame after each line */

    return;
}


/**
 * Reset the view to the end of history.
 */
void gen_history_reset()
{
    history_view = history_next;
}


/**
 * Copy last available string to str, if possible.  Return 1 if successful.
 * Only lines of at most maxlen characters will be considered.  In addition
 * the first searchlen characters of the history entry must match those of str.
 */
int gen_history_back(zchar *str, int searchlen, int maxlen)
{
    char **prev = history_view;

    do {
        RING_DEC(history_view, history_buffer, history_end);
        if ((history_view == history_next)
            || (*history_view == NULL)) {
            os_beep(BEEP_HIGH);
            history_view = prev;
            return 0;
        }
    } while (strlen(*history_view) > (size_t)maxlen
             || (searchlen != 0
                 && strncmp((char *)str, *history_view, searchlen)));
    strcpy((char *)str + searchlen, *history_view + searchlen);
    return 1;
}


/**
 * Opposite of gen_history_back, and works in the same way.
 */
int gen_history_forward(zchar *str, int searchlen, int maxlen)
{
    char **prev = history_view;

    do {
        RING_INC(history_view, history_buffer, history_end);
        if ((history_view == history_next)
            || (*history_view == NULL)) {

            os_beep(BEEP_HIGH);
            history_view = prev;
            return 0;
        }
    } while (strlen(*history_view) > (size_t) maxlen
             || (searchlen != 0
                 && strncmp((char *)str, *history_view, searchlen)));
    strcpy((char *)str + searchlen, *history_view + searchlen);
    return 1;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <errno.h>

#include "../common/frotz.h"
#include "../blorb/blorb.h"
#include "../blorb/blorblow.h"
#include "ux_blorb.h"

#define LOCAL_MEM       -1
#define LOCAL_FILE      -2

#define plong( b) (((int)((b)[3]) << 24) + ((int)((b)[2]) << 16) +\
        ((int)((b)[1]) << 8) + (int)((b)[0]))

#define pshort( b) (((int)((b)[1]) << 8) + (int)((b)[0]))


int m_localfiles = 0;
static bb_map_t *bmap = NULL;
static FILE *bfile = NULL;

static char *ResDir = "./";
static char *ResPict = "PIC%d";
static char *ResSnd = "SND%d";

char *findchunk(char *data, char *string, int length)
{
	char *mydata = data+12;
	while (TRUE) {
		if (strncmp((char*)mydata, string, 4) == 0)
			return mydata+8;

		mydata += ReadLong(mydata+4)+8;

		if ((mydata - data) >= length)
			break;
	}
	return NULL;
}


unsigned short ReadShort(const unsigned char *bytes)
{
	return (unsigned short)(
		((unsigned short)(bytes[0] & 0xFF) << 8) |
		((unsigned short)(bytes[1] & 0xFF)));
}

unsigned long ReadLong(const unsigned char *bytes)
{
	return (unsigned long)(
		((unsigned long)(bytes[0] & 0xFF) << 24) |
		((unsigned long)(bytes[1] & 0xFF) << 16) |
		((unsigned long)(bytes[2] & 0xFF) << 8) |
		((unsigned long)(bytes[3] & 0xFF)));
}

double ReadExtended(const unsigned char *bytes)
{
	double f;
	int expon;
	unsigned long hiMant, loMant;

	expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
	hiMant = ReadLong(bytes+2);
	loMant = ReadLong(bytes+6);

	if (expon == 0 && hiMant == 0 && loMant == 0)
		f = 0;
	else {
		if (expon == 0x7FFF) /* Infinity or NaN */
			f = -1;
		else {
			expon -= 16383;
			/* must #include <math.h> or these won't work */
			f = ldexp(UnsignedToFloat(hiMant),expon -= 31);
			f += ldexp(UnsignedToFloat(loMant),expon -= 32);
		}
	}

	if (bytes[0] & 0x80)
		return -f;
	return f;
}

/* here begins stuff pulled from sfrotz */

int getresource(int num, int ispic, int method, blorb_resource * res)
{
    int st; ulong usage;

    res->bbres.data.ptr = NULL;
    res->file = NULL;

    if (m_localfiles) {
	if ((st = loadlocal(num,ispic,method,res)) == bb_err_None) return st;
    }

    if (!bmap) return bb_err_NotFound;

    if (ispic) usage = bb_ID_Pict;
    else usage = bb_ID_Snd;
    st = bb_load_resource(bmap,method,(bb_result_t *)res,usage,num);
    if (st == bb_err_None) {
	res->type = bmap->chunks[res->bbres.chunknum].type;
	if (method == bb_method_FilePos) res->file = bfile;
    }
    return st;
}


void sf_freeresource( blorb_resource *res)
{
    int cnu;

    if (!res) return;

    cnu = res->bbres.chunknum;

    if (cnu == LOCAL_MEM) {
	if (res->bbres.data.ptr) free(res->bbres.data.ptr);
	return;
    }
    if (cnu == LOCAL_FILE) {
	if (res->file) fclose(res->file);
	return;
    }

    if ((bmap) && (cnu >= 0))
	bb_unload_chunk(bmap,cnu);
}



static FILE * findlocal(int ispic, int num, int *size)
{
    FILE *f;
    char *tpl, buf[MAX_FILE_NAME+1];

    tpl = ispic ? ResPict : ResSnd;
    strcpy(buf, ResDir);
    sprintf(buf + strlen(buf), tpl, num);
    f = fopen(buf, "rb");
    if (!f) return f;
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    return f;
}


static int loadlocal( int num, int ispic, int method, blorb_resource * res)
{
    FILE *f;
    int size;
    zbyte hd[4];

    f = findlocal( ispic, num, &size);
    if (!f) f = findfromlist( ispic, num, &size);
    if (!f) return bb_err_NotFound;

    fread(hd,1,4,f);
    fseek(f,0,SEEK_SET);
    res->type = 0;
    if (ispic) {
	if (hd[0] == 0xff && hd[1] == 0xd8) res->type = bb_make_id('J','P','E','G');
        else if (hd[0] == 0x89 && hd[1] == 0x50) res->type = bb_make_id('P','N','G',' ');
    } else {
	if (memcmp(hd,"FORM",4) == 0) res->type = bb_make_id('F','O','R','M');
	else if (memcmp(hd,"OggS",4) == 0) res->type = bb_make_id('O','G','G','V');
	else res->type = bb_make_id('M','O','D',' ');
    }

    if (!res->type) { fclose(f); return bb_err_NotFound;}

    res->bbres.data.startpos = 0;
    res->file = f;
    res->bbres.length = size;
    if (method == bb_method_FilePos)
	res->bbres.chunknum = LOCAL_FILE;
    else {
	void *ptr;
	res->bbres.chunknum = LOCAL_MEM;
	ptr = res->bbres.data.ptr = malloc(size);
	if (ptr) fread(ptr,1,size,f);
	fclose(f);
	if (!ptr) return bb_err_NotFound;
    }

    return bb_err_None;
}

static FILE * findfromlist( int ispic, int num, int *size)
{
    FILE *f; LLENTRY *l;
    char buf[MAX_FILE_NAME+1];

    if (ispic) l = Lpics;
    else l = Lsnds;
    while (l) {
	if (l->num == num) break;
	l = l->next;
    }
    if (!l) return NULL;

    strcpy(buf,ResDir);
    strcat(buf,l->name);

    f = fopen(buf,"rb");
    if (!f) return f;
    fseek(f,0,SEEK_END);
    *size = ftell(f);
    fseek(f,0,SEEK_SET);
    return f;
}

void sf_regcleanfunc(void *f, const char *p)
{
    cfrec *n = calloc(1, sizeof(cfrec));
    if (n) {
	if (!p) p = "";
	n->func = (CLEANFUNC) f;
	n->name = p;
	n->next = cflist;
	cflist = n;
    }
}




/* /home/dave/zcode/mystory.z5 */
/* /home/dave/zcode/mystory.blb */

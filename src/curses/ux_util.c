#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <zlib.h>
#include "../common/frotz.h"


#define plong( b) (((int)((b)[3]) << 24) + ((int)((b)[2]) << 16) +\
        ((int)((b)[1]) << 8) + (int)((b)[0]))

#define pshort( b) (((int)((b)[1]) << 8) + (int)((b)[0]))


static int myunzip( int csize, zbyte *cdata, zbyte *udata)
{
    zbyte window[32768];
    z_stream z;
    int st;

    unsigned myin( void *d, zbyte **b){return 0;}
    int myout( void *d, zbyte *b, unsigned n)
    {
	memcpy(udata,b,n); udata += n;
	return 0;
    }

    memset(&z,0,sizeof(z));

    st = inflateBackInit( &z, 15, window);
    if (st) return st;

    z.next_in = cdata;
    z.avail_in = csize;

    for (;;){
	st = inflateBack( &z, myin, NULL, myout, NULL);
	if (st == Z_STREAM_END) break;
	if (st) return st;
    }

    st = inflateBackEnd(&z);
    return st;
}


int sf_pkread( FILE *f, int foffs,  void ** out, int *size)
{
    zbyte hd[30], *dest;
    zbyte *data, *cdata;
    int csize, usize, cmet, skip, st;

    fseek(f,foffs,SEEK_SET);
    fread(hd,1,30,f);
    cmet = pshort(hd+8);
    if (cmet != 8) return -10;
    csize = plong(hd+18);
    usize = plong(hd+22);
    if (csize <= 0) return -11;
    if (usize <= 0) return -12;
    data = malloc(usize);
    if (!data) return -13;
    cdata = malloc(csize);
    if (!cdata){ free(data); return -14;}
    skip = pshort(hd+26) + pshort(hd+28);
    fseek(f,foffs+30+skip,SEEK_SET);
    fread(cdata,1,csize,f);
    //printf("%02x csize %d usize %d skip %d\n",cdata[0],csize,usize,skip);

    st = myunzip(csize,cdata,data);

    free(cdata);
    if (st) {
	free(data);
	return st;
    }
    *out = (void *)data;
    *size = usize;
    return st;
}

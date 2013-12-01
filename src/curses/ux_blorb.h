
#include "../blorb/blorb.h"
#include "../blorb/blorblow.h"


typedef struct sampledata_struct {
	unsigned short channels;
	unsigned long samples;
	unsigned short bits;
	double rate;
} sampledata_t;

/*
typedef struct blorb_data_struct {
	bb_map_t	map;
	bb_result_t	result;
} blorb_data_t;
*/

/*
 * The bb_result_t struct lacks a few members that would make things a
 * bit easier.  The myresource struct takes encapsulates the bb_result_t
 * struct and adds a type member and a filepointer.  I would like to
 * convince Andrew Plotkin to make a change in the reference Blorb code
 * to add these members.
 *
 */
typedef struct {
    bb_result_t bbres;
    ulong type;
    FILE *fp;
} myresource;

//bb_err_t	blorb_err;
bb_map_t	*blorb_map;
bb_result_t	blorb_res;

FILE *blorb_fp;


int sf_getresource( int num, int ispic, int method, myresource * res);
void sf_freeresource( myresource *res);

/* uint32 *findchunk(uint32 *data, char *chunkID, int length); */
char *findchunk(char *pstart, char *fourcc, int n);
unsigned short ReadShort(const unsigned char *bytes);
unsigned long ReadLong(const unsigned char *bytes);
double ReadExtended(const unsigned char *bytes);

#define UnsignedToFloat(u) (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)






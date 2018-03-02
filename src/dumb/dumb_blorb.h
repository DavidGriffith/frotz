
#include "../blorb/blorb.h"
#include "../blorb/blorblow.h"


typedef struct sampledata_struct {
	unsigned short channels;
	unsigned long samples;
	unsigned short bits;
	double rate;
} sampledata_t;


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
    unsigned long type;
    FILE *fp;
} myresource;

extern bb_err_t         blorb_err;
extern bb_map_t         *blorb_map;
extern bb_result_t      blorb_res;
extern FILE *blorb_fp;

bb_err_t dumb_blorb_init(char *);
void dumb_blorb_stop(void);

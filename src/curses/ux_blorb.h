
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
    ulong type;
    FILE *fp;
} myresource;


bb_err_t ux_blorb_init(char *);
void ux_blorb_stop(void);

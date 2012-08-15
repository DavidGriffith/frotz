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


/* /home/dave/zcode/mystory.z5 */
/* /home/dave/zcode/mystory.blb */

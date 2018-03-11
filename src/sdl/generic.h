/**
 * @file generic.h
 *
 * Generic useful user interface utility functions.
 */

#ifndef FROTZ_GENERIC_H_
#define FROTZ_GENERIC_H_

#include "../common/frotz.h"

void gen_add_to_history(zchar *buf);
void gen_history_reset(void);
int gen_history_back(zchar *str, int searchlen, int maxlen);
int gen_history_forward(zchar *str, int searchlen, int maxlen);

#endif

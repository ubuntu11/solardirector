
#ifndef __SB_STRINGS_H
#define __SB_STRINGS_H

#include "sb.h"

struct sb_string {
	char *tag;
	char *string;
};
typedef struct sb_string sb_string_t;

int sb_get_strings(sb_session_t *s);
int sb_read_strings(sb_session_t *s, char *filename);
int sb_write_strings(sb_session_t *s, char *filename);
char *sb_get_string(sb_session_t *s, int tag);

#endif

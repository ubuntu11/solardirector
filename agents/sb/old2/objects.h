
#ifndef __SB_OBJECTS_H
#define __SB_OBJECTS_H

#include "sb.h"

#define SMA_FMTUNK 0
#define SMA_FMTINT 1
#define SMA_FMTFLT 2
#define SMA_FMTSTR 3
#define SMA_FMTSER 4
#define SMA_FMTVER 5

enum SMA_OBJECT_FORMAT {
	SMA_OBJECT_FORMAT_UNK,
	SMA_OBJECT_FORMAT_INT,
	SMA_OBJECT_FORMAT_FLOAT,
	SMA_OBJECT_FORMAT_STRING,
	SMA_OBJECT_FORMAT_SERIAL,
	SMA_OBJECT_FORMAT_VERSION
};

struct sb_object {
	char *key;
	char *label;
	char *hier[8];
	char *unit;
	float mult;
	int format;
	bool min;
	bool max;
	bool sum;
	bool avg;
	bool cnt;
	bool sumd;
	struct sb_session *s;				/* Session backptr for JS */
};
typedef struct sb_object sb_object_t;

/* Objects */
int sb_get_objects(sb_session_t *s);
int sb_read_objects(sb_session_t *s, char *filename);
int sb_write_objects(sb_session_t *s, char *filename);
sb_object_t *sb_get_object(sb_session_t *s, char *key);

#endif

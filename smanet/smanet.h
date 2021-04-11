
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SMANET_H
#define __SMANET_H

enum SMANET_CHANCLASS {
	SMANET_CHANCLASS_UNKNOWN,
	SMANET_CHANCLASS_ANALOG,
	SMANET_CHANCLASS_DIGITAL,
	SMANET_CHANCLASS_COUNTER,
	SMANET_CHANCLASS_STATUS
};

enum SMANET_CHANTYPE {
	SMANET_CHANTYPE_UNKNOWN,
	SMANET_CHANTYPE_PARAMETER,
	SMANET_CHANTYPE_SPOTVALUE,
	SMANET_CHANTYPE_ARCHIVE,
	SMANET_CHANTYPE_TEST
};

#if 0
   /* Nur bei analogen Parameterkanaelen sinnvoll! */
   if ((chan->wCType & CH_PARA) &&
       (chan->wCType & CH_ANALOG))
   {
      *min = TChannel_GetGain( chan );
      *max = TChannel_GetOffset( chan );
      return YE_OK; /* alles ok */
   }
   else
   {
      /* es gibt keinen festgelegten Wertebereich! */
      *min = -10000000.0;
      *max =  10000000.0;
      return YE_NO_RANGE;
   }
#endif

/* Channel type */
#define CH_ANALOG	0x0001
#define CH_DIGITAL	0x0002
#define CH_COUNTER	0x0004
#define CH_STATUS	0x0008
#define CH_IN		0x0100
#define CH_OUT		0x0200
#define CH_PARA		0x0400
#define CH_SPOT		0x0800
#define CH_MEAN		0x1000
#define CH_TEST		0x2000

/* Channel format */
#define CH_BYTE		0x00
#define CH_SHORT	0x01
#define CH_LONG		0x02
#define CH_FLOAT	0x04
#define CH_DOUBLE	0x05
#define CH_ARRAY	0x08

struct smanet_value {
	long timestamp;
	uint8_t type;
	union {
		uint8_t bval;
		short wval;
		float lval;
		float fval;
		double dval;
	};
};
typedef struct smanet_value smanet_value_t;

struct smanet_channel {
	uint16_t id;
	uint8_t index;
	uint16_t mask;
	uint16_t format;
	uint16_t level;
	char name[17];
	char unit[9];
	/* Values according to class */
	union {
		float gain;		/* Analog & counter */
		char txtlo[17];		/* Digital */
		uint8_t size;		/* Status */
	};
	union {
		float offset;		/* Analog */
		char txthi[17];		/* Digital */
		list strings;		/* Status */
	};
	smanet_value_t value;
};
typedef struct smanet_channel smanet_channel_t;


typedef struct smanet_session smanet_session_t;

smanet_session_t *smanet_init(solard_module_t *, void *);

int smanet_get_channels(smanet_session_t *s);
int smanet_save_channels(smanet_session_t *s, char *);
int smanet_load_channels(smanet_session_t *s, char *);
smanet_channel_t *smanet_get_channelbyid(smanet_session_t *s, int);
smanet_channel_t *smanet_get_channelbyname(smanet_session_t *s, char *);
smanet_channel_t *smanet_get_channelbyindex(smanet_session_t *s, uint16_t, uint8_t);

int smanet_get_optionbyid(smanet_session_t *s, int, char *);
int smanet_set_optionbyid(smanet_session_t *s, char *, char *);
int smanet_get_optionbyname(smanet_session_t *s, char *, char *, int);
int smanet_set_optionbyname(smanet_session_t *s, char *, char *);

double smanet_get_value(smanet_value_t *);
int smanet_get_valuebyid(smanet_session_t *s, int, double *);
int smanet_set_valuebyid(smanet_session_t *s, int, double);
int smanet_get_valuebyname(smanet_session_t *s, char *, smanet_value_t *);
int smanet_set_valuebyname(smanet_session_t *s, char *, double);

#endif

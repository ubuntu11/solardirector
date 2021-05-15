
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "smanet_internal.h"

/*
Spot value
 ---------------------------------------------------------------------------------
 |          |            |              |      |           |                       |
 | KanalTyp | Kanalindex | DatensatzAnz | Zeit | Zeitbasis | Wert1 Wert2 ... Wertn |
 | ChannelType | Channel index | Record number | Time | Time base | Value1 value2 ... valuen |
 ---------------------------------------------------------------------------------

Params
 --------------------------------------------------------------------
 |          |            |              |                           |
 | KanalTyp | Kanalindex | DatensatzAnz | Wert1 Wert2 ... WertN     |
 | ChannelType | Channel index | Record number | Value1 value2 ... valueN |
 --------------------------------------------------------------------
*/

/* Channel format */
#define CH_BYTE		0x00
#define CH_SHORT	0x01
#define CH_LONG		0x02
#define CH_FLOAT	0x04
#define CH_DOUBLE	0x05
#define CH_ARRAY	0x08

#define dlevel 5

void smanet_clear_values(smanet_session_t *s) {
	smanet_channel_t *c;

	if (!s->values) return;
	list_reset(s->channels);
	while((c = list_get_next(s->channels)) != 0) memset(&s->values[c->id],0,sizeof(smanet_value_t));
}

int smanet_getval(smanet_session_t *s, smanet_channel_t *c) {
	smanet_packet_t *p;
	register uint8_t *sptr,*eptr;
	uint8_t data[3], index;
	uint16_t mask,count;
	time_t timestamp;
	long time_base;
	int r;
	smanet_channel_t *vc;
	smanet_value_t *v;

	dprintf(1,"id: %d, c->id, c->type: %04x, index: %02x\n", c->id, c->mask, c->index);

	p = smanet_alloc_packet(2048);
	if (!p) return 1;

	/* if no values allocated yet, do so now */
	if (!s->values) {
		s->values = calloc(1,list_count(s->channels)*sizeof(smanet_value_t));
		if (!s->values) {
			log_write(LOG_SYSERR,"_smanet_getvals: calloc(%d)\n",list_count(s->channels)*sizeof(smanet_value_t));
			smanet_free_packet(p);
			return 1;
		}
	}
	*((uint16_t *)&data[0]) = c->mask;
	*((uint8_t *)&data[2]) = c->index;
//	*((uint16_t *)&data[4]) = 1;
//	*((uint32_t *)&data[6]) = 0;
	r = smanet_command(s,CMD_GET_DATA,p,data,sizeof(data));
	if (r) return r;
	bindump("value",p->data,p->dataidx);

#if 0
	sptr = p->data;
	eptr = p->data + p->dataidx;
	mask = *(uint16_t *) sptr;
	sptr += 2;
	index = *(uint8_t *) sptr++;
	count = *(uint16_t *) sptr;
	sptr += 2;
	dprintf(1,"mask: %04x, index: %02x, count: %d\n",mask,index,count);
	if (mask & CH_SPOT) {
		timestamp = *(time_t *) sptr;
		sptr += 4;
		time_base = *(long *) sptr;
		sptr += 4;
		dprintf(1,"ts: %ld, time_base: %ld\n", timestamp, time_base);
	}
#endif
	return 0;
}

static int _smanet_getvals(smanet_session_t *s, smanet_channel_t *c) {
	smanet_packet_t *p;
	register uint8_t *sptr,*eptr;
	uint8_t data[3], index;
	uint16_t mask,count;
	time_t timestamp;
	long time_base;
	int r;
	smanet_channel_t *vc;
	smanet_value_t *v;

	dprintf(1,"id: %d, c->id, c->type: %04x, index: %02x\n", c->id, c->mask, c->index);

	p = smanet_alloc_packet(2048);
	if (!p) return 1;

	/* if no values allocated yet, do so now */
	if (!s->values) {
		s->values = calloc(1,list_count(s->channels)*sizeof(smanet_value_t));
		if (!s->values) {
			log_write(LOG_SYSERR,"_smanet_getvals: calloc(%d)\n",list_count(s->channels)*sizeof(smanet_value_t));
			smanet_free_packet(p);
			return 1;
		}
	}

//	smanet_syn_online(s);

//	*((uint16_t *)&data[0]) = c->mask;
	*((uint16_t *)&data[0]) = c->mask | 0x0f;
	/* My SI6048 wont return a single value - only for a group (index 0) wtf  */
//	*((uint8_t *)&data[2]) = c->index;
	*((uint8_t *)&data[2]) = 0;
	r = smanet_command(s,CMD_GET_DATA,p,data,sizeof(data));
	if (r) return r;
	if (debug >= dlevel+1) bindump("values",p->data,p->dataidx);

	sptr = p->data;
	eptr = p->data + p->dataidx;
	mask = *(uint16_t *) sptr;
	sptr += 2;
	index = *(uint8_t *) sptr++;
	count = *(uint16_t *) sptr;
	sptr += 2;
	dprintf(1,"mask: %04x, index: %02x, count: %d\n",mask,index,count);
	if (mask & CH_SPOT) {
		timestamp = *(time_t *) sptr;
		sptr += 4;
		time_base = *(long *) sptr;
		sptr += 4;
		dprintf(1,"ts: %ld, time_base: %ld\n", timestamp, time_base);
	}
	list_reset(s->channels);
	while(sptr < eptr) {
		vc = list_get_next(s->channels);
		if (!vc) break;
		if ((vc->mask & mask) == 0) continue;
		dprintf(1,"vc->name: %s, format: %04x\n", vc->name, vc->format);
		v = &s->values[vc->id];
		switch(vc->format & 0xf) {
		case CH_BYTE:
			v->type = DATA_TYPE_BYTE;
			v->bval = *(uint8_t *) sptr++;
			dprintf(1,"bval: %d (%02X)\n", v->bval, v->bval);
			break;
		case CH_SHORT:
			v->type = DATA_TYPE_SHORT;
			v->wval = *(short *) sptr;
			dprintf(1,"wval: %d (%04x)\n", v->wval, v->wval);
			sptr += 2;
			break;
		case CH_LONG:
			v->type = DATA_TYPE_LONG;
			v->lval = *(long *) sptr;
			dprintf(1,"lval: %ld (%08lx)\n", v->lval, v->lval);
			sptr += 4;
			break;
		case CH_FLOAT:
			v->type = DATA_TYPE_FLOAT;
			v->fval = *(float *) sptr;
			dprintf(1,"lval: %f\n", v->fval);
			sptr += 4;
			break;
		case CH_DOUBLE:
			v->type = DATA_TYPE_DOUBLE;
			v->dval = *(double *) sptr;
			dprintf(1,"lval: %lf\n", v->dval);
			sptr += 8;
			break;
		default:
			v->type = DATA_TYPE_UNKNOWN;
			dprintf(1,"unhandled format: %d\n", vc->format);
			break;
		}
#if 0
   if (!(me->wCType & CH_PARA ) && (dblValue != CHANVAL_INVALID)) {
      if (me->wCType & CH_COUNTER) dblValue = dblValue * (double)me->fGain;
      if (me->wCType & CH_ANALOG)  dblValue = dblValue * (double)me->fGain + (double)me->fOffset;
   }
#endif
		time(&v->timestamp);
	}
	smanet_free_packet(p);
	return 0;
}

static double _getval(smanet_value_t *v) {
	double val;

	dprintf(1,"ts: %d\n", v->timestamp);
	switch(v->type) {
	case DATA_TYPE_BYTE:
		val = v->bval;
		break;
	case DATA_TYPE_SHORT:
		val = v->wval;
		break;
	case DATA_TYPE_LONG:
		val = v->lval;
		break;
	case DATA_TYPE_FLOAT:
		val = v->fval;
		break;
	case DATA_TYPE_DOUBLE:
		val = v->dval;
		break;
	default:
		dprintf(1,"unhandled type(%d)!\n",v->type);
		val = 0.0;
	}
	dprintf(1,"returning: %lf\n", val);
	return val;
}

static int _smanet_setval(smanet_session_t *s, smanet_channel_t *c, smanet_value_t *nv) {
	smanet_packet_t *p;
	uint8_t data[16];
	double v1,v2;
	int i;

	dprintf(1,"id: %d, name: %s, mask: %04x, index: %02x\n", c->id, c->name, c->mask, c->index);

	/* Make sure we have a val to compare it to */
	if (!s->values && _smanet_getvals(s,c)) return 1;

	v1 = _getval(&s->values[c->id]);
	v2 = _getval(nv);
	dprintf(1,"v1: %lf, v2: %lf\n", v1, v2);
//	if (v1 == v2) return 0;

	p = smanet_alloc_packet(16);
	if (!p) return 1;

	*((uint16_t *)&data[0]) = c->mask;
	*((uint8_t *)&data[2]) = c->index;
	*((uint16_t *)&data[3]) = 1;
	i = 5;
	dprintf(1,"type: %d (%s)\n", nv->type, typestr(nv->type));
	switch(nv->type) {
	case DATA_TYPE_BYTE:
		*((uint8_t *)&data[i]) = nv->bval;
		i++;
		break;
	case DATA_TYPE_SHORT:
		*((short *)&data[i]) = nv->wval;
		i += 2;
		break;
	case DATA_TYPE_LONG:
		*((long *)&data[i]) = nv->lval;
		i += 4;
		break;
	case DATA_TYPE_FLOAT:
		*((float *)&data[i]) = nv->fval;
		i += 4;
		break;
	case DATA_TYPE_DOUBLE:
		*((double *)&data[i]) = nv->dval;
		i += 8;
		break;
	default:
		dprintf(1,"unhandled type: %d\n", nv->type);
		return 1;
	}
	bindump("data",data,i);
	if (smanet_command(s,CMD_SET_DATA,p,data,i)) return 1;
	smanet_free_packet(p);
	/* Only update our val if the write was succesful */
	s->values[c->id] = *nv;
	time(&s->values[c->id].timestamp);
	return 0;
}

smanet_value_t *smanet_get_value(smanet_session_t *s, smanet_channel_t *c) {
	if (!s->values && _smanet_getvals(s,c)) return 0;
	if (s->values[c->id].timestamp == 0 && _smanet_getvals(s,c)) return 0;
	return &s->values[c->id];
}

smanet_value_t *smanet_get_valuebyname(smanet_session_t *s, char *name) {
	smanet_channel_t *c;

	c = smanet_get_channelbyname(s,name);
	dprintf(1,"c: %p\n", c);
	if (!c) return 0;
	return smanet_get_value(s,c);
}

int smanet_get_optionval(smanet_session_t *s, smanet_channel_t *c, char *opt) {
	register char *p;
	register int i;
	int r;

	dprintf(1,"opt: %s\n", opt);

	if ((c->mask & CH_STATUS) == 0) return -1;

	i = 0;
	r = -1;
	list_reset(c->strings);
	while((p = list_get_next(c->strings)) != 0) {
		dprintf(1,"p: %s\n", p);
		if (strcmp(p,opt) == 0) {
			r = i;
			break;
		}
		i++;
	}
	dprintf(1,"returning: %d\n", r);
	return r;
}

char *smanet_get_optionbyname(smanet_session_t *s, char *name) {
	smanet_channel_t *c;
	register char *p;
	register int i;
	uint8_t index;

	dprintf(1,"name: %s\n", name);

	c = smanet_get_channelbyname(s,name);
	dprintf(1,"c: %p\n", c);
	if (!c) return 0;
	if ((c->mask & CH_STATUS) == 0) return 0;
	if (!s->values && _smanet_getvals(s,c)) return 0;
	dprintf(1,"timestamp: %d\n", s->values[c->id].timestamp);
	if (s->values[c->id].timestamp == 0 && _smanet_getvals(s,c)) return 0;
	index = _getval(&s->values[c->id]);
	dprintf(1,"index: %d\n", index);
	i = 0;
	list_reset(c->strings);
	while((p = list_get_next(c->strings)) != 0) {
		dprintf(1,"p: %s\n", p);
		if (i == index) return p;
		i++;
	}
	return 0;
}

int smanet_get_optionbyname_r(smanet_session_t *s, char *name, char *dest, int destlen) {
	char *p;

	p = smanet_get_optionbyname(s,name);
	if (p) {
		strncpy(dest,p,destlen);
		return 0;
	}
	return 1;
}

int smanet_set_value(smanet_session_t *s, smanet_channel_t *c, smanet_value_t *v) {
	return _smanet_setval(s,c,v);
}

int smanet_set_valuebyname(smanet_session_t *s, char *name, smanet_value_t *v) {
	smanet_channel_t *c;

	c = smanet_get_channelbyname(s,name);
	if (!c) return 1;
	return _smanet_setval(s,c,v);
}

int smanet_set_optionbyname(smanet_session_t *s, char *name, char *opt) {
	smanet_channel_t *c;
	register char *p;
	smanet_value_t v;
	int i,found;

	dprintf(1,"name: %s, opt: %s\n", name, opt);

	if (!s) return 1;
	/* My SI6048 simply refuses to set option 0 (no value/default) */
	if (strcmp(opt,"---") == 0) return 1;
	c = smanet_get_channelbyname(s,name);
	dprintf(1,"c: %p\n", c);
	if (!c) return 1;
	if ((c->mask & CH_STATUS) == 0) return 1;
	dprintf(1,"count: %d\n", list_count(c->strings));
	if (!list_count(c->strings)) return 1;
	i=found=0;
	list_reset(c->strings);
	while((p = list_get_next(c->strings)) != 0) {
		dprintf(1,"option: %s\n", p);
		if (strcmp(p,opt) == 0) {
			found = 1;
			break;
		}
		i++;
	}
	dprintf(1,"found: %d\n", found);
	if (!found) return 1;
	dprintf(1,"i: %d\n", i);
	time(&v.timestamp);
	v.type = DATA_TYPE_BYTE;
	v.bval = i;
	return _smanet_setval(s,c,&v);
}

#if 0
#define CHANVAL_INVALID -1
void TChannel_SetValue(struct _TChannel * me, TNetDevice * dev, double value)
{
   switch(me->wCType & 0x000f)
   {
      case CH_ANALOG:
         if (me->wCType & CH_PARA)
            TChannel_SetRawValue(me, dev, value, 0 );
         else
            TChannel_SetRawValue(me, dev, (value - me->fOffset) / me->fGain, 0);
         break;

      case CH_COUNTER:
         TChannel_SetRawValue(me, dev, value / me->fGain, 0);
         break;

      case CH_STATUS:
      case CH_DIGITAL:
         TChannel_SetRawValue(me, dev, value, 0 );
         break;

      default:
         break;
   }
}
double TChannel_GetValue(TChannel * me, TNetDevice * dev, int iValIndex)
{
   double dblValue;
   WORD wVal;
   DWORD dwVal;
   float fVal;
   void * value = TChanValRepo_GetValuePtr(dev->chanValRepo,me->Handle);

   if (iValIndex >= TChannel_GetValCnt(me)) goto err;

   /* Ist der Kanalwert ueberhaupt gueltig? */
   if (!TChannel_IsValueValid( me, dev )) return 0.0;

   /* Wert des Kanals holen... */
   /* Ach ja - (langer Seufzer...) Vererbung in 'C': "is nich..." */
   switch(me->wNType & CH_FORM)
   {
      case CH_BYTE :  dblValue =      *((BYTE*)  value + iValIndex); break;
      case CH_WORD :  MoveWORD (&wVal, ((WORD*)  value + iValIndex)); dblValue = wVal;  break;
      case CH_DWORD:  MoveDWORD(&dwVal,((DWORD*) value + iValIndex)); dblValue = dwVal; break;
      case CH_FLOAT4: MoveFLOAT(&fVal, ((float*) value + iValIndex)); dblValue = fVal; break;
      default: assert( 0 );
   }
   //YASDI_DEBUG((VERBOSE_BUGFINDER,"value is = %lf\n", dblValue));

   /*
   ** If the value is valid, calculate the gain and offset ....
   ** CAUTION (SMA?) TRAP:
   ** Parameter channels have no gain and offset!
   ** The two values ..are used here to record the maximum and minimum values
   ** (i.e. the range of values!)
   ** These are of course not allowed to be included in the channel value calculation
   ** include !!!!!!!!
   ** (The 3rd time fell in, 2 x SDC and now here ...)
   */
   if (!(me->wCType & CH_PARA ) && (dblValue != CHANVAL_INVALID))
   {
      //YASDI_DEBUG((VERBOSE_BUGFINDER,"Gain = %f\n", me->fGain));
      if (me->wCType & CH_COUNTER) dblValue = dblValue * (double)me->fGain;
      if (me->wCType & CH_ANALOG)  dblValue = dblValue * (double)me->fGain + (double)me->fOffset;
   }

   return dblValue;

   err:
   return 0.0;
}
#endif

#if 0
char *smanet_get_option(smanet_session_t *s, smanet_channel_t *c) {
	register char *p;
	register int i;

	if ((c->mask & CH_STATUS) == 0) return 0;

	i = 0;
	list_reset(c->strings);
	while((p = list_get_next(c->strings)) != 0) {
		dprintf(1,"p: %s\n", p);
		if (i == c->value.bval) return p;
		i++;
	}
	return 0;
}

static int _set_value(smanet_value_t *v, double val) {
	switch(v->type) {
	case DATA_TYPE_BYTE:
		v->bval = val;
		break;
	case DATA_TYPE_SHORT:
		v->wval = val;
		break;
	case DATA_TYPE_LONG:
		v->lval = val;
		break;
	case DATA_TYPE_FLOAT:
		v->fval = val;
		break;
	case DATA_TYPE_DOUBLE:
		v->dval = val;
		break;
	default:
		dprintf(1,"unhandled type!\n");
		return 1;
		break;
	}
	time(&v->timestamp);
	return 0;
}
#endif

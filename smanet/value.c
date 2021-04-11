
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "smanet_internal.h"

#define  CH_IN                  0x0100
#define  CH_OUT         0x0200
#define  CH_PARA                0x0400
#define  CH_SPOT     0x0800
#define  CH_MEAN     0x1000
#define  CH_TEST     0x2000

#define  CH_ALL      0x000f
#define  CH_PARA_ALL    (CH_PARA  | CH_ALL)
#define  CH_SPOT_ALL    (CH_SPOT  | CH_ALL)
#define  CH_MEAN_ALL    (CH_MEAN  | CH_ALL)
#define  CH_TEST_ALL    (CH_SPOT  | CH_IN  |  CH_ALL  | CH_TEST)

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
void smanet_reset_values(smanet_session_t *s) {
	smanet_channel_t *c;

	list_reset(s->channels);
	while((c = list_get_next(s->channels)) != 0) {
		c->value.timestamp = 0;
	}
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

	dprintf(1,"id: %d, c->id, c->type: %04x, index: %02x\n", c->id, c->mask, c->index);

	p = smanet_alloc_packet(2048);

//	smanet_syn_online(s);

	/* My SI6048 wont return a single value - only for a group (index 0)  */

//	*((uint16_t *)&data[0]) = c->mask;
	*((uint16_t *)&data[0]) = c->mask | 0x0f;
//	*((uint8_t *)&data[2]) = c->index;
	*((uint8_t *)&data[2]) = 0;
	r = smanet_command(s,CMD_GET_DATA,p,data,sizeof(data));
	if (r) return r;
	bindump("values",p->data,p->dataidx);

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
		switch(vc->format & 0xf) {
		case CH_BYTE:
			vc->value.bval = *(uint8_t *) sptr++;
			dprintf(1,"bval: %d (%02X)\n", vc->value.bval, vc->value.bval);
			break;
		case CH_SHORT:
			vc->value.wval = *(short *) sptr;
			dprintf(1,"wval: %d (%04x)\n", vc->value.wval, vc->value.wval);
			sptr += 2;
			break;
		case CH_LONG:
			vc->value.lval = *(long *) sptr;
			dprintf(1,"lval: %ld (%08lx)\n", vc->value.lval, vc->value.lval);
			sptr += 4;
			break;
		case CH_FLOAT:
			vc->value.fval = *(float *) sptr;
			dprintf(1,"lval: %f\n", vc->value.fval);
			sptr += 4;
			break;
		case CH_DOUBLE:
			vc->value.dval = *(double *) sptr;
			dprintf(1,"lval: %lf\n", vc->value.dval);
			sptr += 8;
			break;
		default:
			dprintf(1,"unhandled format: %d\n", vc->format);
			break;
		}
		vc->value.type = vc->format & 0xf;
		time(&vc->value.timestamp);
	}
	smanet_free_packet(p);
	return 0;
}

static int _smanet_setval(smanet_session_t *s, smanet_channel_t *c, smanet_value_t *v) {
	smanet_packet_t *p;
	uint8_t data[16];
	int i;

	dprintf(1,"id: %d, c->id, name: %s\n", c->id, c->name);

	p = smanet_alloc_packet(16);

	i = 0;
	*((uint16_t *)&data[i]) = c->mask;
	i += 2;
	*((uint8_t *)&data[i]) = c->index;
	i++;
	*((uint16_t *)&data[i]) = 1;
	i += 2;
#if 0
	switch(v->type) {
	case DATA_TYPE_BYTE:
		*((uint8_t *)&data[i]) = v->bval;
		i++;
		break;
	case DATA_TYPE_SHORT:
		*((short *)&data[i]) = v->wval;
		i += 2;
		break;
	case DATA_TYPE_LONG:
		*((long *)&data[i]) = v->lval;
		i += 4;
		break;
	case DATA_TYPE_FLOAT:
		*((float *)&data[i]) = v->fval;
		i += 4;
		break;
	case DATA_TYPE_DOUBLE:
		*((double *)&data[i]) = v->dval;
		i += 8;
		break;
	default:
		dprintf(1,"unhandled type: %d\n", v->type);
		return 1;
	}
#endif
	smanet_command(s,CMD_SET_DATA,p,data,i);
	smanet_free_packet(p);
	return 0;
}

int smanet_get_valuebyname(smanet_session_t *s, char *name, smanet_value_t *v) {
	smanet_channel_t *c;

	c = smanet_get_channelbyname(s,name);
	dprintf(1,"c: %p\n", c);
	if (!c) return 1;
	if (c->value.timestamp == 0) if (_smanet_getvals(s,c)) return 1;
	*v = c->value;
	return 0;
}

int smanet_get_optionbyname(smanet_session_t *s, char *name, char *dest, int destlen) {
	smanet_channel_t *c;
	register char *p;
	register int i;
	uint8_t index;

	dprintf(1,"name: %s\n", name);

	c = smanet_get_channelbyname(s,name);
	dprintf(1,"c: %p\n", c);
	if (!c) return 1;
	if ((c->mask & CH_STATUS) == 0) return 1;
	if (c->value.timestamp == 0) if (_smanet_getvals(s,c)) return 1;
	index = smanet_get_value(&c->value);
	dprintf(1,"index: %d\n", index);
	i = 0;
	list_reset(c->strings);
	while((p = list_get_next(c->strings)) != 0) {
		dprintf(1,"p: %s\n", p);
		if (i == index) {
			dest[0] = 0;
			strncat(dest,p,destlen);
			return 0;
		}
		i++;
	}
	return 1;
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

double smanet_get_value(smanet_value_t *v) {
	double val;

	dprintf(1,"ts: %d\n", v->timestamp);
	switch(v->type) {
	case CH_BYTE:
		val = v->bval;
		break;
	case CH_SHORT:
		val = v->wval;
		break;
	case CH_LONG:
		val = v->lval;
		break;
	case CH_FLOAT:
		val = v->fval;
		break;
	case CH_DOUBLE:
		val = v->dval;
		break;
	default:
		dprintf(1,"unhandled type!\n");
		val = 0.0;
	}
	dprintf(1,"returning: %lf\n", val);
	return val;
}
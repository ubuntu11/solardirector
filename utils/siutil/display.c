
#include "siutil.h"

void dint(char *label, char *format, int val) {
	char temp[128];

	dprintf(3,"dint: label: %s, val: %d\n", label, val);
	switch(outfmt) {
	case 2:
//		json_object_set_number(root_object, label, val);
		break;
	case 1:
		sprintf(temp,"%%s,%s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	default:
		sprintf(temp,"%%-25s %s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	}
}
#define _dint(l,v) dint(l,"%d",v)

void dfloat(char *label, char *format, float val) {
	char temp[128];

	dprintf(3,"dint: label: %s, val: %f\n", label, val);
	switch(outfmt) {
	case 2:
//		json_object_set_number(root_object, label, val);
		break;
	case 1:
		sprintf(temp,"%%s,%s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	default:
		sprintf(temp,"%%-25s %s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	}
}
#define _dfloat(l,v) dfloat(l,"%f",v)

void dstr(char *label, char *format, char *val) {
	char temp[128];

	dprintf(3,"dint: label: %s, val: %s\n", label, val);
	switch(outfmt) {
	case 2:
//		json_object_set_string(root_object, label, val);
		break;
	case 1:
		sprintf(temp,"%%s,%s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	default:
		sprintf(temp,"%%-25s %s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	}
}
#define _dstr(l,v) dstr(l,"%s",v)

static inline void _addstr(char *str,char *newstr) {
	dprintf(4,"str: %s, newstr: %s\n", str, newstr);
	if (strlen(str)) strcat(str,sepstr);
	dprintf(4,"str: %s\n", str);
	if (outfmt == 2) strcat(str,"\"");
	strcat(str,newstr);
	if (outfmt == 2) strcat(str,"\"");
	dprintf(4,"str: %s\n", str);
}

void _outpower(char *label, si_power_t *p) {
	char str[128],temp[196];

	str[0] = 0;
	sprintf(temp,"%3.2f",p->l1);
	_addstr(str,temp);
	sprintf(temp,"%3.2f",p->l2);
	_addstr(str,temp);
	sprintf(temp,"%3.2f",p->l3);
	_addstr(str,temp);
	switch(outfmt) {
	case 2:
		sprintf(temp,"[ %s ]",str);
		dprintf(1,"temp: %s\n", temp);
//		json_object_dotset_value(root_object, label, json_parse_string(temp));
		break;
	case 1:
		fprintf(outfp,"%s,%s\n",label,str);
		break;
	default:
		fprintf(outfp,"%-25s %s\n",label,str);
		break;
	}
}

void _outrelay(char *label, int r1, int r2) {
	char str[32],temp[36];

	str[0] = 0;
	_addstr(str,r1 ? "1" : "0");
	_addstr(str,r2 ? "1" : "0");
	switch(outfmt) {
	case 2:
		sprintf(temp,"[ %s ]",str);
		dprintf(1,"temp: %s\n", temp);
//		json_object_dotset_value(root_object, label, json_parse_string(temp));
		break;
	case 1:
		fprintf(outfp,"%s,%s\n",label,str);
		break;
	default:
		fprintf(outfp,"%-25s %s\n",label,str);
		break;
	}
}

void display_info(si_session_t *s) {
	si_info_t newinfo, *info;

	if (si_get_info(s,&newinfo)) return;
	info = &newinfo;

	_outpower("Active Grid",&info->active.grid);
	_outpower("Active SI",&info->active.si);
	_outpower("ReActive Grid",&info->reactive.grid);
	_outpower("ReActive SI",&info->reactive.si);
	_outpower("AC1 Voltage",&info->voltage);
	dfloat("AC1 Frequency","%2.2f",info->frequency);
	_outpower("AC2 Voltage",&info->ac2);
	dfloat("AC2 Frequency","%2.2f",info->ac2_frequency);
	dfloat("Battery Voltage","%3.2f",info->battery_voltage);
	dfloat("Battery Current","%3.2f",info->battery_current);
	dfloat("Battery Temp","%3.2f",info->battery_temp);
	dfloat("Battery SoC","%3.2f",info->battery_soc);
	dfloat("Battery CVSP","%3.2f",info->battery_cvsp);
	_outrelay("Master Relay",info->bits.relay1,info->bits.relay2);
	_outrelay("Slave1 Relays",info->bits.s1_relay1,info->bits.s1_relay2);
	_outrelay("Slave2 Relays",info->bits.s2_relay1,info->bits.s2_relay2);
	_outrelay("Slave3 Relays",info->bits.s3_relay1,info->bits.s3_relay2);
	_dstr("Generator Running",info->bits.GnRn ? "true" : "false");
	_dstr("Generator Autostart",info->bits.AutoGn ? "true" : "false");
	_dstr("AutoLodExt",info->bits.AutoLodExt ? "true" : "false");
	_dstr("AutoLodSoc",info->bits.AutoLodSoc ? "true" : "false");
	_dstr("Tm1",info->bits.Tm1 ? "true" : "false");
	_dstr("Tm2",info->bits.Tm2 ? "true" : "false");
	_dstr("ExtPwrDer",info->bits.ExtPwrDer ? "true" : "false");
	_dstr("ExtVfOk",info->bits.ExtVfOk ? "true" : "false");
	_dstr("Grid On",info->bits.GdOn ? "true" : "false");
	_dstr("Error",info->bits.Error ? "true" : "false");
	_dstr("Run",info->bits.Run ? "true" : "false");
	_dstr("Battery Fan On",info->bits.BatFan ? "true" : "false");
	_dstr("Acid Circulation On",info->bits.AcdCir ? "true" : "false");
	_dstr("MccBatFan",info->bits.MccBatFan ? "true" : "false");
	_dstr("MccAutoLod",info->bits.MccAutoLod ? "true" : "false");
	_dstr("CHP #1 On",info->bits.Chp ? "true" : "false");
	_dstr("CHP #2 On",info->bits.ChpAdd ? "true" : "false");
	_dstr("SiComRemote",info->bits.SiComRemote ? "true" : "false");
	_dstr("Overload",info->bits.OverLoad ? "true" : "false");
	_dstr("ExtSrcConn",info->bits.ExtSrcConn ? "true" : "false");
	_dstr("Silent",info->bits.Silent ? "true" : "false");
	_dstr("Current",info->bits.Current ? "true" : "false");
	_dstr("FeedSelfC",info->bits.FeedSelfC ? "true" : "false");
	_outpower("Load",&info->load);
	_dint("Charging procedure",info->charging_proc);
	_dint("State",info->state);
	_dint("Error Message",info->errmsg);
	dfloat("PVPwrAt","%3.2f",info->PVPwrAt);
	dfloat("GdCsmpPwrAt","%3.2f",info->GdCsmpPwrAt);
	dfloat("GdFeedPwr","%3.2f",info->GdFeedPwr);
}

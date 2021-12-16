
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
	_outpower("Active Grid",&s->info.active.grid);
	_outpower("Active SI",&s->info.active.si);
	_outpower("ReActive Grid",&s->info.reactive.grid);
	_outpower("ReActive SI",&s->info.reactive.si);
	_outpower("AC1 Voltage",&s->info.volt);
	dfloat("AC1 Frequency","%2.2f",s->info.frequency);
	_outpower("AC2 Voltage",&s->info.ac2_volt);
	dfloat("AC2 Frequency","%2.2f",s->info.ac2_frequency);
	dfloat("Battery Voltage","%3.2f",s->info.battery_voltage);
	dfloat("Battery Current","%3.2f",s->info.battery_current);
	dfloat("Battery Temp","%3.2f",s->info.battery_temp);
	dfloat("Battery SoC","%3.2f",s->info.battery_soc);
	dfloat("Battery CVSP","%3.2f",s->info.battery_cvsp);
	_outrelay("Master Relay",s->info.relay1,s->info.relay2);
	_outrelay("Slave1 Relays",s->info.s1_relay1,s->info.s1_relay2);
	_outrelay("Slave2 Relays",s->info.s2_relay1,s->info.s2_relay2);
	_outrelay("Slave3 Relays",s->info.s3_relay1,s->info.s3_relay2);
	_dstr("Generator Running",s->info.GnRn ? "true" : "false");
	_dstr("Generator Autostart",s->info.AutoGn ? "true" : "false");
	_dstr("AutoLodExt",s->info.AutoLodExt ? "true" : "false");
	_dstr("AutoLodSoc",s->info.AutoLodSoc ? "true" : "false");
	_dstr("Tm1",s->info.Tm1 ? "true" : "false");
	_dstr("Tm2",s->info.Tm2 ? "true" : "false");
	_dstr("ExtPwrDer",s->info.ExtPwrDer ? "true" : "false");
	_dstr("ExtVfOk",s->info.ExtVfOk ? "true" : "false");
	_dstr("Grid On",s->info.GdOn ? "true" : "false");
	_dstr("Error",s->info.Error ? "true" : "false");
	_dstr("Run",s->info.Run ? "true" : "false");
	_dstr("Battery Fan On",s->info.BatFan ? "true" : "false");
	_dstr("Acid Circulation On",s->info.AcdCir ? "true" : "false");
	_dstr("MccBatFan",s->info.MccBatFan ? "true" : "false");
	_dstr("MccAutoLod",s->info.MccAutoLod ? "true" : "false");
	_dstr("CHP #1 On",s->info.Chp ? "true" : "false");
	_dstr("CHP #2 On",s->info.ChpAdd ? "true" : "false");
	_dstr("SiComRemote",s->info.SiComRemote ? "true" : "false");
	_dstr("Overload",s->info.OverLoad ? "true" : "false");
	_dstr("ExtSrcConn",s->info.ExtSrcConn ? "true" : "false");
	_dstr("Silent",s->info.Silent ? "true" : "false");
	_dstr("Current",s->info.Current ? "true" : "false");
	_dstr("FeedSelfC",s->info.FeedSelfC ? "true" : "false");
	_dint("Charging procedure",s->info.charging_proc);
	_dint("State",s->info.state);
	_dint("Error Message",s->info.errmsg);
	dfloat("TotLodPwr","%3.2f",s->info.TotLodPwr);
	dfloat("PVPwrAt","%3.2f",s->info.PVPwrAt);
	dfloat("GdCsmpPwrAt","%3.2f",s->info.GdCsmpPwrAt);
	dfloat("GdFeedPwr","%3.2f",s->info.GdFeedPwr);
}

int monitor(si_session_t *s, int interval) {
	while(1) {
		if (si_read(s,(void *)0xDEADBEEF,0)) {
			log_error("unable to read data");
			return 1;
		}
#ifdef __WIN32
		system("cls");
		system("time /t");
#else
		system("clear; echo \"**** $(date) ****\"; echo \"\"");
#endif

		display_info(s);
		sleep(interval);
	}
	return 0;
}

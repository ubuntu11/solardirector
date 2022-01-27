
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <string.h>
#include "agent.h"
#include "jbd.h"
#include "jbd_regs.h"

#if 0
enum JBD_PARM_DT {
	JBD_PARM_DT_UNK,
	JBD_PARM_DT_INT,		/* 16 bit number */
	JBD_PARM_DT_FLOAT,		/* floating pt */
	JBD_PARM_DT_STR,		/* string */
	JBD_PARM_DT_TEMP,		/* temp */
	JBD_PARM_DT_DATE,		/* date */
	JBD_PARM_DT_DR,			/* discharge rate */
	JBD_PARM_DT_FUNC,		/* function bits */
	JBD_PARM_DT_NTC,		/* ntc bits */
	JBD_PARM_DT_B0,			/* byte 0 */
	JBD_PARM_DT_B1,			/* byte 1 */
	JBD_PARM_DT_DOUBLE,		/* Double hard cofnig */
	JBD_PARM_DT_SCVAL,
	JBD_PARM_DT_SCDELAY,
	JBD_PARM_DT_DSGOC2,
	JBD_PARM_DT_DSGOC2DELAY,
	JBD_PARM_DT_HCOVPDELAY,
	JBD_PARM_DT_HCUVPDELAY,
	JBD_PARM_DT_MAX
};

static char *parmtypes[] = {
	"unknown",
	"int",
	"float",
	"string",
	"temp",
	"date",
	"discharge rate",
	"function",
	"ntc",
	"byte 0",
	"byte 1",
	"double",
	"scval",
	"scdelay",
	"DSGOC2",
	"DSGOC2DELAY",
	"HCOVPDELAY",
	"HCUVPDELAY",
};

static char *parm_typestr(int dt) {
	if (dt < 0 || dt >= JBD_PARM_DT_MAX) return "invalid";
	return parmtypes[dt];
}

/* Selection values */
static int dsgoc2_vals[] = { 8,11,14,17,19,22,25,28,31,33,36,39,42,44,47,50 };
#define ndsgoc2_vals (sizeof(dsgoc2_vals)/sizeof(int))
static int dsgoc2delay_vals[] = { 8,20,40,80,160,320,640,1280 };
#define ndsgoc2delay_vals (sizeof(dsgoc2delay_vals)/sizeof(int))
static int sc_vals[] = { 22,33,44,56,67,78,89,100 };
#define nsc_vals (sizeof(sc_vals)/sizeof(int))
static int scdelay_vals[] = {  70,100,200,400 };
#define nscdelay_vals (sizeof(scdelay_vals)/sizeof(int))
static int hcovpdelay_vals[] = {  1,2,4,8 };
#define nhcovpdelay_vals (sizeof(hcovpdelay_vals)/sizeof(int))
static int hcuvpdelay_vals[] = {  1,4,8,16 };
#define nhcuvpdelay_vals (sizeof(hcuvpdelay_vals)/sizeof(int))
#endif

static int jbd_config_getset(void *ctx, config_property_t *p) {
	jbd_session_t *s = ctx;

	dprintf(1,"s: %p, p: %p\n", s, p);
	if (p) dprintf(1,"p->name: %s\n", p->name);
	return 1;
}

int jbd_config_init(jbd_session_t *s) {
config_property_t capacity_params[] = {
	/* Name, Type, dest, len, def flags, scope, nvalues, nlabels, labels, units, scale, precision */
	{ "DesignCapacity", DATA_TYPE_INT, jbd_config_getset, 0, s, 0,
		"range", 3, (int []){ 0, 300, 1 }, 1, (char *[]) { "Design Capacity" }, "S", 1, 0 },

#if 0
	{ "DesignCapacity", DATA_TYPE_FLOAT, jbd_config_getset, 0, "0", 0,
		"range", 3, (int []){ 0, 9999999, 1 }, 1, (char *[]) { "Design Capacity" }, "mAH", 100, 1 },
	{ "DesignCapacity", DATA_TYPE_FLOAT, 0, 0, 0, 1, (char *[]){ "Design Cap" }, "mAH", 100, "%.1f" },
	{ "CycleCapacity", DATA_TYPE_FLOAT, 0, 0, 0, 1, (char *[]){ "Cycle Cap" }, "mAH", 100, "%.1f" },
	{ "FullChargeVol", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "Full Chg Vol" }, "mV", 1000, "%.3f" },
	{ "ChargeEndVol", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "End of Dsg VOL" }, "mV", 1000, "%.3f" },
	{ "DischargingRate", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 100, .1 }, 1, (char *[]){ "DischargingRate" }, "%", 10, "%.1f" },
	{ "VoltageCap100", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Voltage at 100% Capacity" }, "mV", 1000, "%.3f" },
	{ "VoltageCap90", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Voltage at 90% capacity" }, "mV", 1000, "%.3f" },
	{ "VoltageCap80", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Voltage at 80% capacity" }, "mV", 1000, "%.3f" },
	{ "VoltageCap70", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Voltage at 70% capacity" }, "mV", 1000, "%.3f" },
	{ "VoltageCap60", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Voltage at 60% capacity" }, "mV", 1000, "%.3f" },
	{ "VoltageCap50", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Voltage at 50% capacity" }, "mV", 1000, "%.3f" },
	{ "VoltageCap40", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Voltage at 40% capacity" }, "mV", 1000, "%.3f" },
	{ "VoltageCap30", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Voltage at 30% capacity" }, "mV", 1000, "%.3f" },
	{ "VoltageCap20", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Voltage at 20% capacity" }, "mV", 1000, "%.3f" },
	{ "VoltageCap10", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Voltage at 10% capacity" }, "mV", 1000, "%.3f" },
	{ "fet_ctrl_time_set", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "Fet Control" }, "S", 0, 0 },
	{ "led_disp_time_set", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "LED Timer" }, "S", 0, 0 },
#endif
	{0}
};

config_property_t func_params[] = {
#if 0
	{ "BatteryConfig", DATA_TYPE_INT, "mask", 8,
 		(int []){ JBD_FUNC_SWITCH, JBD_FUNC_SCRL, JBD_FUNC_BALANCE_EN, JBD_FUNC_CHG_BALANCE, JBD_FUNC_LED_EN, JBD_FUNC_LED_NUM, JBD_FUNC_RTC, JBD_FUNC_EDV }, 8, (char *[]){ "Switch", "SCRL", "BALANCE_EN", "CHG_BALANCE", "LED_EN", "LED_NUM", "RTC", "EDV"  }, 0, 0 },
#endif
	{0}
};

config_property_t ntc_params[] = {
#if 0
	{ "NtcConfig", DATA_TYPE_INT, "mask",
		8, (int []){ JBD_NTC1, JBD_NTC2, JBD_NTC3, JBD_NTC4, JBD_NTC5, JBD_NTC6, JBD_NTC7, JBD_NTC8 },
		8, (char *[]){ "NTC1","NTC2","NTC3","NTC4","NTC5","NTC6","NTC7","NTC8" }, 0, 0 },
#endif
	{0}
};

config_property_t balance_params[] = {
#if 0
	{ "BalanceStartVoltage", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "Start Voltage" }, "mV", 1000, "%.3f" },
	{ "BalanceWindow", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "BalancPrecision" }, "mV", 1000, "%.3f" },
#endif
	{0}
};

config_property_t gps_params[] = {
#if 0
	{ "GPS_VOL", DATA_TYPE_INT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "GPS Off" }, "mV", 1000, "%.3f" },
	{ "GPS_TIME", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "GPS Time" }, "S", 0, 0 },
#endif
	{0}
};

config_property_t other_params[] = {
#if 0
	{ "SenseResistor", DATA_TYPE_FLOAT, 0, 0, 0, 1, (char *[]){ "Current Res" }, "mR", 10, "%.1f" },
	{ "PackNum", DATA_TYPE_INT, "select",
		28, (int []){ 3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30 },
		1, (char *[]){ "mR PackNum" }, 0, 0 },
	{ "CycleCount", DATA_TYPE_INT, 0, 0, 0, 0, 0, 0, 0 },
	{ "SerialNumber", DATA_TYPE_INT, 0, 0, 0, 0, 0, 0, 0 },
	{ "ManufacturerName", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 0 },
	{ "DeviceName", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 0 },
	{ "ManufactureDate", DATA_TYPE_DATE, 0, 0, 0, 0, 0, 0, 0 },
	{ "BarCode", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 0 },
#endif
	{0}
};

config_property_t basic_params[] = {
#if 0
	{ "CellOverVoltage", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "COVP" }, "mV", 1000, "%.3f" },
	{ "CellOVRelease", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "COVP Release" }, "mV", 1000, "%.3f" },
	{ "CellOVDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "COVP Delay" }, "S", 0 },
	{ "CellUnderVoltage", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "CUVP" }, "mV", 1000, "%.3f" },
	{ "CellUVRelease", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "CUVP Release" }, "mV", 1000, "%.3f" },
	{ "CellUVDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "CUVP Delay" }, "S", 0 },
	{ "PackOverVoltage", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 99999, 1 }, 1, (char *[]){ "POVP" }, "mV", 100, "%.1f" },
	{ "PackOVRelease", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 99999, 1 }, 1, (char *[]){ "POVP Release" }, "mV", 100, "%.1f" },
	{ "PackOVDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "POVP Delay" }, "S", 0 },
	{ "PackUnderVoltage", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 99999, 1 }, 1, (char *[]){ "PUVP" }, "mV", 100, "%.1f" },
	{ "PackUVRelease", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 99999, 1 }, 1, (char *[]){ "PUVP Release" }, "mV", 100, "%.1f" },
	{ "PackUVDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "PUVP Delay" }, "S", 0 },
	{ "ChgOverTemp", DATA_TYPE_INT, "range", 3, (int []){ -500, 500, 1 }, 1, (char *[]){ "CHGOT" }, "C", 0 },
	{ "ChgOTRelease", DATA_TYPE_INT, "range", 3, (int []){ -500, 500, 1 }, 1, (char *[]){ "CHGOT Release" }, "C", 0 },
	{ "ChgOTDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "CHGOT Delay" }, "S", 0 },
	{ "ChgLowTemp", DATA_TYPE_INT, "range", 3, (int []){ -500, 500, 1 }, 1, (char *[]){ "CHGUT" }, "C", 0 },
	{ "ChgUTRelease", DATA_TYPE_INT, "range", 3, (int []){ -500, 500, 1 }, 1, (char *[]){ "CHGUT Release" }, "C", 0 },
	{ "ChgUTDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "CHGUT Delay" }, "S", 0 },
	{ "DisOverTemp", DATA_TYPE_INT, "range", 3, (int []){ -500, 500, 1 }, 1, (char *[]){ "DSGOT" }, "C", 0 },
	{ "DsgOTRelease", DATA_TYPE_INT, "range", 3, (int []){ -500, 500, 1 }, 1, (char *[]){ "DSGOT Release" }, "C", 0 },
	{ "DsgOTDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "DSGOT Delay" }, "S", 0 },
	{ "DisLowTemp", DATA_TYPE_INT, "range", 3, (int []){ -500, 500, 1 }, 1, (char *[]){ "DSGUT" }, "C", 0 },
	{ "DsgUTRelease", DATA_TYPE_INT, "range", 3, (int []){ -500, 500, 1 }, 1, (char *[]){ "DSGUT Release" }, "C", 0 },
	{ "DsgUTDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "DSGUT Delay" }, "S", 0 },
	{ "OverChargeCurrent", DATA_TYPE_INT, "range", 3, (int []){ 0, 99999, 1 }, 1, (char *[]){ "CHGOC" }, "mA", .1 },
	{ "ChgOCRDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "CHGOC Release" }, "S", 0 },
	{ "ChgOCDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "CHGOC Delay" }, "S", 0 },
	{ "OverDisCurrent", DATA_TYPE_INT, "range", 3, (int []){ 0, 99999, 1 }, 1, (char *[]){ "DSGOC" }, "mA", .1 },
	{ "DsgOCRDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "DSGOC Release" }, "S", 0 },
	{ "DsgOCDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "DSGOC Delay" }, "S", 0 },
#endif
	{0}
};

config_property_t hard_params[] = {
#if 0
	{ "DoubleOCSC", DATA_TYPE_BOOL, 0, 0, 0, 1, (char *[]){ "Doubled Overcurrent and short-circuit value" }, 0, 0 },
	{ "DSGOC2", DATA_TYPE_INT, "select", ndsgoc2_vals, dsgoc2_vals, ndsgoc2_vals, 0, "mV", 0 },
	{ "DSGOC2Delay", DATA_TYPE_INT, "select", ndsgoc2delay_vals, dsgoc2delay_vals, ndsgoc2delay_vals, 0, "mS", 0 },
	{ "SCValue", DATA_TYPE_INT, "select", nsc_vals, sc_vals, nsc_vals, 0, "mV", 0 },
	{ "SCDelay", DATA_TYPE_INT, "select", nscdelay_vals, scdelay_vals, nscdelay_vals, 0, "uS", 0 },
	{ "HardCellOverVoltage", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "High COVP" }, "mV", 1000, "%.3f" },
	{ "HCOVPDelay", DATA_TYPE_INT, "select", nhcovpdelay_vals, hcovpdelay_vals, nhcovpdelay_vals, 0, "S", 0 },
	{ "HardCellUnderVoltage", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 10000, 1 }, 1, (char *[]){ "High COVP" }, "mV", 1000, "%.3f" },
	{ "HCUVPDelay", DATA_TYPE_INT, "select", nhcuvpdelay_vals, hcuvpdelay_vals, nhcuvpdelay_vals, 0, "S", 0 },
	{ "SCRelease", DATA_TYPE_INT, "range", 3, (int []){ 0, 199, 1 }, 1, (char *[]){ "SC Release Time" }, "S", 0 },
#endif
	{0}
};

config_property_t control_params[] = {
#if 0
	{ "charge", DATA_TYPE_BOOL, 0, 0, 0, 1, (char *[]){ "Charging" }, 0, 0 },
	{ "discharge", DATA_TYPE_BOOL, 0, 0, 0, 1, (char *[]){ "Discharging" }, 0, 0 },
	{ "balance", DATA_TYPE_INT, "select", 3, (int []){ 0, 1, 2 }, 3, (char *[]){ "Off","On","Only when charging" }, 0, 0 },
#endif
	{0}
};

	struct parmdir {
		char *name;
		config_property_t *parms;
	} *pd, allparms[] = {
		{ "Capacity Config", capacity_params },
		{ "Function Configuration", func_params },
		{ "NTC Configuration", ntc_params },
		{ "Balance Configuration", balance_params },
		{ "GPS Configuration", gps_params },
		{ "Other Configuration", other_params },
		{ "Basic Parameters", basic_params },
		{ "Hard Parameters", hard_params },
		{ "Controls", control_params },
		{ 0 }
	};

	for(pd=allparms; pd->name; pd++)
		config_add_props(s->ap->cp, pd->name, pd->parms, CONFIG_FLAG_NOSAVE | CONFIG_FLAG_NOID);

	return 0;
}

#if 0

config_property_t *_getd(char *label) {
	config_property_t *dp;
	register int i,j;

	dprintf(1,"label: %s\n", label);

	for(i=0; i < NALL; i++) {
		dp = allparms[i].parms;
		dprintf(1,"section: %s\n", allparms[i].name);
		for(j=0; j < allparms[i].count; j++) {
			dprintf(1,"dp->name: %s\n", dp[j].name);
			if (strcmp(dp[j].name,label)==0)
				return &dp[j];
		}
	}
	return 0;
}

static struct jbd_params {
	uint8_t reg;
	char *label;
	int dt;
} params[] = {
	{ JBD_REG_DCAP,"DesignCapacity", JBD_PARM_DT_FLOAT },
	{ JBD_REG_CCAP,"CycleCapacity", JBD_PARM_DT_FLOAT },
	{ JBD_REG_FULL,"FullChargeVol", JBD_PARM_DT_FLOAT },
	{ JBD_REG_EMPTY,"ChargeEndVol", JBD_PARM_DT_FLOAT },
	{ JBD_REG_RATE,"DischargingRate", JBD_PARM_DT_FLOAT },
	{ JBD_REG_MFGDATE,"ManufactureDate", JBD_PARM_DT_DATE },
	{ JBD_REG_SERIAL,"SerialNumber", JBD_PARM_DT_INT },
	{ JBD_REG_CYCLE,"CycleCount", JBD_PARM_DT_INT },
	{ JBD_REG_CHGOT,"ChgOverTemp", JBD_PARM_DT_TEMP },
	{ JBD_REG_RCHGOT,"ChgOTRelease", JBD_PARM_DT_TEMP },
	{ JBD_REG_CHGUT,"ChgLowTemp", JBD_PARM_DT_TEMP },
	{ JBD_REG_RCHGUT,"ChgUTRelease", JBD_PARM_DT_TEMP },
	{ JBD_REG_DSGOT,"DisOverTemp", JBD_PARM_DT_TEMP },
	{ JBD_REG_RDSGOT,"DsgOTRelease", JBD_PARM_DT_TEMP },
	{ JBD_REG_DSGUT,"DisLowTemp", JBD_PARM_DT_TEMP },
	{ JBD_REG_RDSGUT,"DsgUTRelease", JBD_PARM_DT_TEMP },
	{ JBD_REG_POVP,"PackOverVoltage", JBD_PARM_DT_FLOAT },
	{ JBD_REG_RPOVP,"PackOVRelease", JBD_PARM_DT_FLOAT },
	{ JBD_REG_PUVP,"PackUnderVoltage", JBD_PARM_DT_FLOAT },
	{ JBD_REG_RPUVP,"PackUVRelease", JBD_PARM_DT_FLOAT },
	{ JBD_REG_COVP,"CellOverVoltage", JBD_PARM_DT_FLOAT },
	{ JBD_REG_RCOVP,"CellOVRelease", JBD_PARM_DT_FLOAT },
	{ JBD_REG_CUVP,"CellUnderVoltage", JBD_PARM_DT_FLOAT },
	{ JBD_REG_RCUVP,"CellUVRelease", JBD_PARM_DT_FLOAT },
	{ JBD_REG_CHGOC,"OverChargeCurrent", JBD_PARM_DT_INT },
	{ JBD_REG_DSGOC,"OverDisCurrent", JBD_PARM_DT_INT },
	{ JBD_REG_BALVOL,"BalanceStartVoltage", JBD_PARM_DT_FLOAT },
	{ JBD_REG_BALPREC,"BalanceWindow", JBD_PARM_DT_FLOAT },
	{ JBD_REG_CURRES,"SenseResistor", JBD_PARM_DT_FLOAT },
	{ JBD_REG_FUNCMASK,"BatteryConfig", JBD_PARM_DT_FUNC },
	{ JBD_REG_NTCMASK,"NtcConfig", JBD_PARM_DT_NTC },
	{ JBD_REG_STRINGS,"PackNum", JBD_PARM_DT_INT },
	{ JBD_REG_FETTIME,"fet_ctrl_time_set", JBD_PARM_DT_INT },
	{ JBD_REG_LEDTIME,"led_disp_time_set", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP80,"VoltageCap80", JBD_PARM_DT_FLOAT },
	{ JBD_REG_VOLCAP60,"VoltageCap60", JBD_PARM_DT_FLOAT },
	{ JBD_REG_VOLCAP40,"VoltageCap40", JBD_PARM_DT_FLOAT },
	{ JBD_REG_VOLCAP20,"VoltageCap20", JBD_PARM_DT_FLOAT },
	{ JBD_REG_HCOVP,"HardCellOverVoltage", JBD_PARM_DT_FLOAT },
	{ JBD_REG_HCUVP,"HardCellUnderVoltage", JBD_PARM_DT_FLOAT },
	{ JBD_REG_HCOC,"DoubleOCSC", JBD_PARM_DT_DOUBLE },
	{ JBD_REG_HCOC,"SCValue", JBD_PARM_DT_SCVAL },
	{ JBD_REG_HCOC,"SCDelay", JBD_PARM_DT_SCDELAY },
	{ JBD_REG_HCOC,"DSGOC2", JBD_PARM_DT_DSGOC2 },
	{ JBD_REG_HCOC,"DSGOC2Delay", JBD_PARM_DT_DSGOC2DELAY },
	{ JBD_REG_HTRT,"HCOVPDelay", JBD_PARM_DT_HCOVPDELAY },
	{ JBD_REG_HTRT,"HCUVPDelay",JBD_PARM_DT_HCUVPDELAY },
	{ JBD_REG_HTRT,"SCRelease",JBD_PARM_DT_B1 },
	{ JBD_REG_CHGDELAY,"ChgUTDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_CHGDELAY,"ChgOTDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_DSGDELAY,"DsgUTDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_DSGDELAY,"DsgOTDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_PVDELAY,"PackUVDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_PVDELAY,"PackOVDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_CVDELAY,"CellUVDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_CVDELAY,"CellOVDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_CHGOCDELAY,"ChgOCDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_CHGOCDELAY,"ChgOCRDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_DSGOCDELAY,"DsgOCDelay", JBD_PARM_DT_B0 },
	{ JBD_REG_DSGOCDELAY,"DsgOCRDelay", JBD_PARM_DT_B1 },
	{ JBD_REG_MFGNAME,"ManufacturerName", JBD_PARM_DT_STR },
	{ JBD_REG_MODEL,"DeviceName", JBD_PARM_DT_STR },
	{ JBD_REG_BARCODE,"BarCode", JBD_PARM_DT_STR },
	{ JBD_REG_GPSOFF,"GPS_VOL", JBD_PARM_DT_FLOAT },
	{ JBD_REG_GPSOFFTIME,"GPS_TIME", JBD_PARM_DT_INT },
	{ JBD_REG_VOLCAP90,"VoltageCap90", JBD_PARM_DT_FLOAT },
	{ JBD_REG_VOLCAP70,"VoltageCap70", JBD_PARM_DT_FLOAT },
	{ JBD_REG_VOLCAP50,"VoltageCap50", JBD_PARM_DT_FLOAT },
	{ JBD_REG_VOLCAP30,"VoltageCap30", JBD_PARM_DT_FLOAT },
	{ JBD_REG_VOLCAP10,"VoltageCap10", JBD_PARM_DT_FLOAT },
	{ JBD_REG_VOLCAP100,"VoltageCap100", JBD_PARM_DT_FLOAT },
	{ 0,"charge", 0 },
	{ 0,"discharge", 0 },
	{ 0,"balance", 0 },
	{ 0,0,0 }
};
typedef struct jbd_params jbd_params_t;

struct jbd_params *_getp(char *label) {
	register struct jbd_params *pp;

	dprintf(3,"label: %s\n", label);
	for(pp = params; pp->label; pp++) {
		dprintf(3,"pp->label: %s\n", pp->label);
		if (strcmp(pp->label,label)==0) {
			return pp;
		}
	}
	return 0;
}

#define _dint(l,v) sprintf(temp,"%d",v);
#define _dbool(l,v) sprintf(temp,"%s",(v ? "true" : "false"));
#define _dfloat(l,v) sprintf(temp,"%f",v);
#define _dstr(l,v) strncat(temp,v,sizeof(temp)-1)
#define CTEMP(v) ( (v - 2731) / 10 )

static inline void _addstr(char *str,char *newstr) {
	dprintf(4,"str: %s, newstr: %s\n", str, newstr);
	if (strlen(str)) strcat(str,",");
	strcat(str,newstr);
	dprintf(4,"str: %s\n", str);
}

struct jbd_config_ctx {
	jbd_session_t *s;
	char *action;
	char errmsg[256];
	int status;
};

static int jbd_get_config(jbd_session_t *s, struct jbd_params *pp, config_property_t *dp, uint8_t *data, int len) {
	char topic[200];
	char str[64],temp[72];
	uint16_t val;
	float fval;
	int i,ival;

	dprintf(1,"label: %s, dt: %d (%s)\n", pp->label, pp->dt, parm_typestr(pp->dt));
	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);

	temp[0] = 0;
	switch(pp->dt) {
	case JBD_PARM_DT_INT:
		ival = jbd_getshort(data);
		dprintf(1,"ival: %d\n", ival);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) ival /= dp->scale;
		dprintf(1,"format: %s\n", dp->format);
		if (dp->format) sprintf(temp,dp->format,ival);
		else sprintf(temp,"%d",ival);
		break;
	case JBD_PARM_DT_TEMP:
		val = jbd_getshort(data);
		_dint(pp->label,CTEMP(val));
		break;
	case JBD_PARM_DT_DATE:
		{
			unsigned short mfgdate;
			int day,mon,year;

			mfgdate = jbd_getshort(data);
			dprintf(2,"mfgdate: %04x\n", mfgdate);
			day = mfgdate & 0x1f;
			mon = (mfgdate >> 5) & 0x0f;
			year = 2000 + (mfgdate >> 9);
			dprintf(2,"year: %d, mon: %d, day: %d\n", year, mon, day);
			sprintf(temp,"%04d%02d%02d",year,mon,day);
		}
		break;
	case JBD_PARM_DT_B0:
		val = jbd_getshort(data);
		dprintf(1,"val: %04x\n", val);
		dprintf(1,"data[0]: %02x, [1]: %02x\n", data[0], data[1]);
		_dint(pp->label,data[0]);
		break;
	case JBD_PARM_DT_B1:
		val = jbd_getshort(data);
		dprintf(1,"val: %04x\n", val);
		dprintf(1,"data[0]: %02x, [1]: %02x\n", data[0], data[1]);
		_dint(pp->label,data[1]);
		break;
	case JBD_PARM_DT_FUNC:
		val = jbd_getshort(data);
		dprintf(1,"val: %04x\n", val);
		str[0] = 0;
		if (val & JBD_FUNC_SWITCH) _addstr(str,"Switch");
		if (val & JBD_FUNC_SCRL)  _addstr(str,"SCRL");
		if (val & JBD_FUNC_BALANCE_EN) _addstr(str,"BALANCE_EN");
		if (val & JBD_FUNC_CHG_BALANCE) _addstr(str,"CHG_BALANCE");
		if (val & JBD_FUNC_LED_EN) _addstr(str,"LED_EN");
		if (val & JBD_FUNC_LED_NUM) _addstr(str,"LED_NUM");
		if (val & JBD_FUNC_RTC) _addstr(str,"RTC");
		if (val & JBD_FUNC_EDV) _addstr(str,"EDV");
		_dstr(pp->label,str);
		break;
	case JBD_PARM_DT_NTC:
		val = jbd_getshort(data);
		str[0] = 0;
		if (val & JBD_NTC1) _addstr(str,"NTC1");
		if (val & JBD_NTC2) _addstr(str,"NTC2");
		if (val & JBD_NTC3) _addstr(str,"NTC3");
		if (val & JBD_NTC4) _addstr(str,"NTC4");
		if (val & JBD_NTC5) _addstr(str,"NTC5");
		if (val & JBD_NTC6) _addstr(str,"NTC6");
		if (val & JBD_NTC7) _addstr(str,"NTC7");
		if (val & JBD_NTC8) _addstr(str,"NTC8");
		_dstr(pp->label,str);
		break;
	case JBD_PARM_DT_FLOAT:
		fval = jbd_getshort(data);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) fval /= dp->scale;
		if (dp->format) sprintf(temp,dp->format,fval);
		else sprintf(temp,"%f",fval);
		break;
	case JBD_PARM_DT_STR:
		data[len] = 0;
		temp[0] = 0;
		strncat(temp,(char *)data,sizeof(temp)-1);
		trim(temp);
		break;
	case JBD_PARM_DT_DOUBLE:
		_dbool(pp->label,((data[0] & 0x80) != 0));
		break;
	case JBD_PARM_DT_DSGOC2:
		i = data[1] & 0x0f;
		dprintf(1,"data[1]: %02x\n", data[1]);
		dprintf(1,"i: %d\n", i);
		_dint(pp->label,dsgoc2_vals[i]);
		break;
	case JBD_PARM_DT_DSGOC2DELAY:
		i = (data[1] >> 4) & 0x07;
		dprintf(1,"data[1]: %02x\n", data[1]);
		dprintf(1,"i: %d\n", i);
		_dint(pp->label,dsgoc2delay_vals[i]);
		break;
	case JBD_PARM_DT_SCVAL:
		i = data[0] & 0x07;
		dprintf(1,"data[0]: %02x\n", data[0]);
		dprintf(1,"i: %d\n", i);
		_dint(pp->label,sc_vals[i]);
		break;
	case JBD_PARM_DT_SCDELAY:
		i = (data[0] >> 3) & 0x03;
		dprintf(1,"data[0]: %02x\n", data[0]);

		dprintf(1,"i: %d\n", i);
		_dint(pp->label,scdelay_vals[i]);
		break;
	case JBD_PARM_DT_HCOVPDELAY:
		i = (data[0] >> 4) & 0x03;
		dprintf(1,"data[0]: %02x\n", data[0]);

		dprintf(1,"i: %d\n", i);
		_dint(pp->label,hcovpdelay_vals[i]);
		break;
	case JBD_PARM_DT_HCUVPDELAY:
		i = (data[0] >> 6) & 0x03;
		dprintf(1,"data[0]: %02x\n", data[0]);

		dprintf(1,"i: %d\n", i);
		_dint(pp->label,hcuvpdelay_vals[i]);
		break;
	default:
		sprintf(temp,"unhandled switch for: %d",pp->dt);
		break;
	}
	sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,SOLARD_ROLE_BATTERY,s->name,SOLARD_FUNC_CONFIG,pp->label);
	dprintf(1,"topic: %s, temp: %s\n", topic, temp);
	return mqtt_pub(s->ap->m,topic,temp,1,1);
}

/* TODO: check value for limits in desc */
static int jbd_set_config(jbd_session_t *s, uint8_t *data, struct jbd_params *pp, config_property_t *dp, char *value) {
	char *p;
	int bytes,len,i,ival;
	uint16_t val;
	float fval;

	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);

	/* Len = 2 (16 bit size) for most */
	len = 2;

	/* Controls */
	if (strcmp(dp->name,"charge") == 0) {
		conv_type(DATA_TYPE_BOOL,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"ival: %d\n", ival);
		return (charge_control(s,ival) ? -1 : 1);
	} else if (strcmp(dp->name,"discharge") == 0) {
		conv_type(DATA_TYPE_BOOL,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"ival: %d\n", ival);
		return (discharge_control(s,ival) ? -1 : 1);
	} else if (strcmp(dp->name,"balance") == 0) {
		conv_type(DATA_TYPE_INT,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"ival: %d\n", ival);
		return (balance_control(s,ival) ? -1 : 1);
	} 

	dprintf(1,"pp->dt: %d, DATA_TYPE_STRING: %d\n", pp->dt, DATA_TYPE_STRING);
	switch(pp->dt) {
	case JBD_PARM_DT_INT:
		conv_type(DATA_TYPE_INT,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) ival *= dp->scale;
		val = ival;
		dprintf(1,"val: %d\n", val);
		jbd_putshort(data,val);
		break;
	case JBD_PARM_DT_TEMP:
		conv_type(DATA_TYPE_SHORT,&val,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"val: %d\n", val);
//		( (v - 2731) / 10 )
		val = ((val * 10) + 2731);
		dprintf(1,"val: %d\n", val);
		jbd_putshort(data,val);
		break;
	case JBD_PARM_DT_DATE:
		break;
	case JBD_PARM_DT_B0:
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return -1;
		conv_type(DATA_TYPE_SHORT,&val,0,DATA_TYPE_STRING,value,0);
		data[0] = val;
		break;
	case JBD_PARM_DT_B1:
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return -1;
		conv_type(DATA_TYPE_SHORT,&val,0,DATA_TYPE_STRING,value,0);
		data[1] = val;
		break;
	case JBD_PARM_DT_FUNC:
		val = 0;
		dprintf(1,"value: %s\n", value);
		for(i=0; i < 99; i++) {
			p = strele(i,",",value);
			if (!strlen(p)) break;
			dprintf(1,"p: %s\n", p);
			trim(p);
			if (strcasecmp(p,"Switch")==0)
				val |= JBD_FUNC_SWITCH;
			else if (strcasecmp(p,"SCRL")==0)
				val |= JBD_FUNC_SCRL;
			else if (strcasecmp(p,"BALANCE_EN")==0)
				val |= JBD_FUNC_BALANCE_EN;
			else if (strcasecmp(p,"CHG_BALANCE")==0)
				val |= JBD_FUNC_CHG_BALANCE;
			else if (strcasecmp(p,"LED_EN")==0)
				val |= JBD_FUNC_LED_EN;
			else if (strcasecmp(p,"LED_NUM")==0)
				val |= JBD_FUNC_LED_NUM;
		}
		if (val & JBD_FUNC_CHG_BALANCE)
			s->balancing = 2;
		else if (val & JBD_FUNC_BALANCE_EN)
			s->balancing = 1;
		else
			s->balancing = 0;
		dprintf(1,"val: %d\n", val);
		jbd_putshort(data,val);
		break;
	case JBD_PARM_DT_NTC:
		val = 0;
		for(i=0; i < 99; i++) {
			p = strele(i,",",value);
			if (!strlen(p)) break;
			dprintf(1,"p: %s\n", p);
			trim(p);
			if (strcasecmp(p,"NTC1")==0)
				val |= JBD_NTC1;
			else if (strcasecmp(p,"NTC2")==0)
				val |= JBD_NTC2;
			else if (strcasecmp(p,"NTC3")==0)
				val |= JBD_NTC3;
			else if (strcasecmp(p,"NTC4")==0)
				val |= JBD_NTC4;
			else if (strcasecmp(p,"NTC5")==0)
				val |= JBD_NTC5;
			else if (strcasecmp(p,"NTC6")==0)
				val |= JBD_NTC6;
			else if (strcasecmp(p,"NTC7")==0)
				val |= JBD_NTC7;
			else if (strcasecmp(p,"NTC8")==0)
				val |= JBD_NTC8;
		}
		dprintf(1,"val: %d\n", val);
		jbd_putshort(data,val);
		break;
	case JBD_PARM_DT_FLOAT:
		/* The COVP and CUVP release values must be 100mV diff  */
		conv_type(DATA_TYPE_FLOAT,&fval,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"fval: %f\n", fval);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) fval *= dp->scale;
		val = fval;
		dprintf(1,"val: %d\n", val);
		jbd_putshort(data,val);
		break;
	case JBD_PARM_DT_STR:
		data[0] = 0;
		/* XXX */
		strncat((char *)data,value,255);
		trim((char *)data);
		len = strlen((char *)data);
		break;
	case JBD_PARM_DT_DOUBLE:
		dprintf(1,"sval: %s\n", value);
		conv_type(DATA_TYPE_BOOL,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"ival: %d\n", ival);
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return -1;
		dprintf(1,"data[0]: %x\n", data[0]);
		if (ival) data[0] |= 0x80;
		else data[0] &= 0x7F;
		dprintf(1,"data[0]: %x\n", data[0]);
		break;
	case JBD_PARM_DT_DSGOC2:
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return -1;
		conv_type(DATA_TYPE_INT,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"ival: %d\n", ival);
		dprintf(1,"data[1]: %02x\n", data[1]);
		for(i=0; i < ndsgoc2_vals; i++) {
			if (dsgoc2_vals[i] == ival) {
				dprintf(1,"i: %d\n", i);
				data[1] &= ~0x0f;
				data[1] |= i;
				break;
			}
		}
		if (i >= ndsgoc2_vals) return 0;
		dprintf(1,"data[1]: %02x\n", data[1]);
		break;
	case JBD_PARM_DT_DSGOC2DELAY:
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return -1;
		conv_type(DATA_TYPE_INT,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"ival: %d\n", ival);
		dprintf(1,"data[1]: %02x\n", data[1]);
		for(i=0; i < ndsgoc2delay_vals; i++) {
			if (dsgoc2delay_vals[i] == ival) {
				dprintf(1,"i: %d\n", i);
				data[1] &= 0xF0;
				data[1] |= (i << 4);
				break;
			}
		}
		if (i >= ndsgoc2delay_vals) return 0;
		dprintf(1,"data[1]: %02x\n", data[1]);
		break;
	case JBD_PARM_DT_SCVAL:
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return -1;
		conv_type(DATA_TYPE_INT,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"ival: %d\n", ival);
		dprintf(1,"data[0]: %02x\n", data[0]);
		for(i=0; i < nsc_vals; i++) {
			if (sc_vals[i] == ival) {
				dprintf(1,"i: %d\n", i);
				data[0] &= ~0x07;
				data[0] |= i;
				break;
			}
		}
		if (i >= nsc_vals) return 0;
		dprintf(1,"data[0]: %02x\n", data[0]);
		break;
	case JBD_PARM_DT_SCDELAY:
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return -1;
		conv_type(DATA_TYPE_INT,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"ival: %d\n", ival);
		dprintf(1,"data[0]: %02x\n", data[0]);
		for(i=0; i < nscdelay_vals; i++) {
			if (scdelay_vals[i] == ival) {
				dprintf(1,"i: %d\n", i);
				data[0] &= ~0x18;
				data[0] |= (i << 3);
				break;
			}
		}
		if (i >= nscdelay_vals) return 0;
		dprintf(1,"data[0]: %02x\n", data[0]);
		break;
	case JBD_PARM_DT_HCOVPDELAY:
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return -1;
		conv_type(DATA_TYPE_INT,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"ival: %d\n", ival);
		dprintf(1,"data[0]: %02x\n", data[0]);
		for(i=0; i < nhcovpdelay_vals; i++) {
			if (hcovpdelay_vals[i] == ival) {
				dprintf(1,"i: %d\n", i);
				data[0] &= 0xCF;
				data[0] |= (i << 4);
				break;
			}
		}
		if (i >= nhcovpdelay_vals) return 0;
		dprintf(1,"data[0]: %02x\n", data[0]);
                break;
	case JBD_PARM_DT_HCUVPDELAY:
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return -1;
		conv_type(DATA_TYPE_INT,&ival,0,DATA_TYPE_STRING,value,0);
		dprintf(1,"ival: %d\n", ival);
		dprintf(1,"data[0]: %02x\n", data[0]);
		for(i=0; i < nhcuvpdelay_vals; i++) {
			if (hcuvpdelay_vals[i] == ival) {
				dprintf(1,"i: %d\n", i);
				data[0] &= 0x3F;
				data[0] |= (i << 6);
				break;
			}
		}
		if (i >= nhcuvpdelay_vals) return 0;
		dprintf(1,"data[0]: %02x\n", data[0]);
		break;
	default:
		dprintf(1,"unhandled case: %d\n", pp->dt);
		return 0;
		break;
	}
	jbd_get_config(s,pp,dp,data,len);
	return len;
}

static int jbd_doconfig(void *ctx, char *action, char *label, char *value, char *errmsg) {
	jbd_session_t *s = ctx;
	struct jbd_params *pp;
	config_property_t *dp;
	uint8_t data[256];
	int r,bytes;

	r = 1;
	dprintf(1,"label: %s, value: %s\n", label, value);
	if (!label) {
		strcpy(errmsg,"invalid request");
		goto jbd_doconfig_error;
	}
	pp = _getp(label);
	if (!pp) {
		sprintf(errmsg,"%s: not found",label);
		goto jbd_doconfig_error;
	}
	dp = _getd(label);
	if (!dp) {
		sprintf(errmsg,"%s: not found",label);
		goto jbd_doconfig_error;
	}


	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);
	dprintf(1,"action: %s\n", action);
	if (strcmp(action,"Get")==0) {
		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, sizeof(data));
		if (bytes < 0) {
			sprintf(errmsg,"read error");
			goto jbd_doconfig_error;
		}
		r = jbd_get_config(s,pp,dp,data,bytes);
	} else if (strcmp(action,"Set")==0) {
		int len;

		if (!value) {
			strcpy(errmsg,"invalid request");
			goto jbd_doconfig_error;
		}
		len = jbd_set_config(s,data,pp,dp,value);
		if (len < 0) {
			sprintf(errmsg,"I/O error");
			goto jbd_doconfig_error;
		} else if (len == 0) {
			sprintf(errmsg,"bad data");
			goto jbd_doconfig_error;
		}
		bytes = jbd_rw(s, JBD_CMD_WRITE, pp->reg, data, len);
		if (bytes < 0) {
			sprintf(errmsg,"write error");
			goto jbd_doconfig_error;
		}
	} else {
		strcpy(errmsg,"invalid request");
		goto jbd_doconfig_error;
	}

jbd_doconfig_error:
	dprintf(1,"returning: %d\n",r);
	return r;
}

static int jbd_config_getmsg(jbd_session_t *s, solard_message_t *msg) {
        char errmsg[SOLARD_ERRMSG_LEN];
        int status,isopen,isfw;
        long start;

        start = mem_used();

	isopen = 0;
	if (jbd_open(s) < 0) {
		strncpy(errmsg,strerror(errno),sizeof(errmsg)-1);
		goto jbd_config_error;
	} else {
		isopen = 1;
	}

	isfw = 0;
	if (jbd_eeprom_start(s) < 0) {
		strcpy(errmsg,"unable to open eeprom");
		goto jbd_config_error;
	} else {
		isfw = 1;
	}

        status = agent_config_process(msg,jbd_doconfig,s,errmsg);
        if (status) goto jbd_config_error;

        status = 0;
        strcpy(errmsg,"success");

jbd_config_error:
	if (isopen) jbd_close(s);
	if (isfw) jbd_eeprom_end(s);
        dprintf(1,"msg->replyto: %s", msg->replyto);
        if (msg->replyto) agent_reply(s->ap, msg->replyto, status, errmsg);
        dprintf(1,"used: %ld\n", mem_used() - start);
        return status;
}
#endif

#if 0
int jbd_config_add_params(json_value_t *j) {
	int x,y;
	json_value_t *ca,*o,*a;
	struct parmdir *dp;

	/* Configuration array */
	ca = json_create_array();

	/* We use sections */
	for(x=0; x < NALL; x++) {
		o = json_create_object();
		dp = &allparms[x];
//		if (dp->count > 1) {
			a = json_create_array();
			for(y=0; y < dp->count; y++) json_array_add_descriptor(a,dp->parms[y]);
			json_add_value(o,dp->name,a);
//		} else if (dp->count == 1) {
//			json_add_descriptor(o,dp->name,dp->parms[0]);
//		}
		json_array_add_value(ca,o);
	}

	json_add_value(j,"configuration",ca);
	return 0;
}
#endif

int jbd_get(void *h, char *name, char *value, char *errmsg) {
//	jbd_session_t *s = h;

	dprintf(1,"name: %s, value: %s\n", name, value);
	return 1;
}

int jbd_getset(void *h, list args, char *errmsg) {
//	jbd_session_t *s = h;

	return 1;
}

int jbd_config(void *h, int req, ...) {
	jbd_session_t *s = h;
	va_list va;
	void **vp;
	int r;

	r = 1;
	va_start(va,req);
	dprintf(1,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_INIT:
	    {
		config_function_t jbd_funcs[] = {
			{ "get", jbd_getset, s, 1 },
			{ "set", jbd_getset, s, 2 },
//			{ "reset", jbd_reset, s, 0 },
			{0}
		};

		dprintf(1,"**** CONFIG INIT *******\n");

		/* 1st arg is AP */
		s->ap = va_arg(va,solard_agent_t *);
		dprintf(1,"ap: %p\n", s->ap);

		config_add_funcs(s->ap->cp, jbd_funcs);

#ifdef JS
		/* Init JS */
		r = jbd_jsinit(s);
#endif
	    }
	    break;
#ifndef JS
	case SOLARD_CONFIG_GET_INFO:
		{
			json_value_t **vpp = va_arg(va,json_value_t **);
			dprintf(1,"vpp: %p\n", vpp);
			if (vpp) {
				r = jbd_get_info(s);
				dprintf(1,"s->info: %p\n", s->info);
				if (!r) *vpp = s->info;
			}
		}
#endif
		break;
	case SOLARD_CONFIG_GET_DRIVER:
		dprintf(1,"GET_DRIVER called!\n");
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->tp;
			r = 0;
		}
		break;
	case SOLARD_CONFIG_GET_HANDLE:
		dprintf(1,"GET_HANDLE called!\n");
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->tp_handle;
			r = 0;
		}
		break;
	}
	dprintf(1,"returning: %d\n", r);
	va_end(va);
	return r;
}

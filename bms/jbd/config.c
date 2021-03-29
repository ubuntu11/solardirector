
#include <string.h>
#include "agent.h"
#include "jbd.h"
#include "jbd_regs.h"

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
};

/* Selection values */
static int dsgoc2_vals[] = { 8,11,14,17,19,22,25,28,31,33,36,39,42,44,47,50 };
#define ndsgoc2_vals (sizeof(dsgoc2_vals)/sizeof(int))
static char *dsgoc2_labels[] = { "8","11","14","17","19","22","25","28","31","33","36","39","42","44","47","50" };
static int dsgoc2delay_vals[] = { 8,20,40,80,160,320,640,1280 };
#define ndsgoc2delay_vals (sizeof(dsgoc2delay_vals)/sizeof(int))
static char *dsgoc2delay_labels[] = { "8","20","40","80","160","320","640","1280" };
static int sc_vals[] = { 22,33,44,56,67,78,89,100 };
#define nsc_vals (sizeof(sc_vals)/sizeof(int))
static char *sc_labels[] = { "22","33","44","56","67","78","89","100" };
static int scdelay_vals[] = {  70,100,200,400 };
#define nscdelay_vals (sizeof(scdelay_vals)/sizeof(int))
static char *scdelay_labels[] = { "70","100","200","400" };
static int hcovpdelay_vals[] = {  1,2,4,8 };
#define nhcovpdelay_vals (sizeof(hcovpdelay_vals)/sizeof(int))
static char *hcovpdelay_labels[] = { "1","2","4","8" };
static int hcuvpdelay_vals[] = {  1,4,8,16 };
#define nhcuvpdelay_vals (sizeof(hcuvpdelay_vals)/sizeof(int))
static char *hcuvpdelay_labels[] = { "1","4","8","16" };

static json_descriptor_t capacity_params[] = {
	/* Name, Type, Scope, nValues, Values, nLabels, Labels, Units, Scale */
	{ "DesignCapacity", DATA_TYPE_FLOAT, 0, 0, 0, 1, (char *[]){ "Design Cap" }, "mAH", 100, "%.1f" },
	{ "CycleCapacity", DATA_TYPE_FLOAT, 0, 0, 0, 1, (char *[]){ "Cycle Cap" }, "mAH", 100, "%.1f" },
	{ "FullChargeVol", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "Full Chg Vol" }, "mV", 1000, "%.3f" },
	{ "ChargeEndVol", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "End of Dsg VOL" }, "mV", 1000, "%.3f" },
	{ "DischargingRate", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 100, .1 }, 1, (char *[]){ "DischargingRate" }, "%", 10, "%.1f" },
	{ "VoltageCap100", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "100%CapOfVol" }, "mV", 1000, "%.3f" },
	{ "VoltageCap90", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "90%CapOfVol" }, "mV", 1000, "%.3f" },
	{ "VoltageCap80", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "80%CapOfVol" }, "mV", 1000, "%.3f" },
	{ "VoltageCap70", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "70%CapOfVol" }, "mV", 1000, "%.3f" },
	{ "VoltageCap60", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "60%CapOfVol" }, "mV", 1000, "%.3f" },
	{ "VoltageCap50", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "50%CapOfVol" }, "mV", 1000, "%.3f" },
	{ "VoltageCap40", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "40%CapOfVol" }, "mV", 1000, "%.3f" },
	{ "VoltageCap30", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "30%CapOfVol" }, "mV", 1000, "%.3f" },
	{ "VoltageCap20", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "20%CapOfVol" }, "mV", 1000, "%.3f" },
	{ "VoltageCap10", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "10%CapOfVol" }, "mV", 1000, "%.3f" },
	{ "fet_ctrl_time_set", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "Fet Control" }, "S", 0, 0 },
	{ "led_disp_time_set", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "LED Timer" }, "S", 0, 0 },
};
#define NCAP (sizeof(capacity_params)/sizeof(json_descriptor_t))

static json_descriptor_t func_params[] = {
	{ "BatteryConfig", DATA_TYPE_INT, "mask", 8,
 		(int []){ JBD_FUNC_SWITCH, JBD_FUNC_SCRL, JBD_FUNC_BALANCE_EN, JBD_FUNC_CHG_BALANCE, JBD_FUNC_LED_EN, JBD_FUNC_LED_NUM, JBD_FUNC_RTC, JBD_FUNC_EDV }, 8, (char *[]){ "Switch", "SCRL", "BALANCE_EN", "CHG_BALANCE", "LED_EN", "LED_NUM", "RTC", "EDV"  }, 0, 0 },
};
#define NFUNC (sizeof(func_params)/sizeof(json_descriptor_t))

static json_descriptor_t ntc_params[] = {
	{ "NtcConfig", DATA_TYPE_INT, "mask",
		8, (int []){ JBD_NTC1, JBD_NTC2, JBD_NTC3, JBD_NTC4, JBD_NTC5, JBD_NTC6, JBD_NTC7, JBD_NTC8 },
		8, (char *[]){ "NTC1","NTC2","NTC3","NTC4","NTC5","NTC6","NTC7","NTC8" }, 0, 0 },
};
#define NNTC (sizeof(ntc_params)/sizeof(json_descriptor_t))

static json_descriptor_t balance_params[] = {
	{ "BalanceStartVoltage", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "Start Voltage" }, "mV", 1000, "%.3f" },
	{ "BalanceWindow", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "BalancPrecision" }, "mV", 1000, "%.3f" },
};
#define NBAL (sizeof(balance_params)/sizeof(json_descriptor_t))

static json_descriptor_t gps_params[] = {
	{ "GPS_VOL", DATA_TYPE_INT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "GPS Off" }, "mV", 1000, "%.3f" },
	{ "GPS_TIME", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "GPS Time" }, "S", 0, 0 },
};
#define NGPS (sizeof(gps_params)/sizeof(json_descriptor_t))

static json_descriptor_t other_params[] = {
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
};
#define NOTHER (sizeof(other_params)/sizeof(json_descriptor_t))

static json_descriptor_t basic_params[] = {
	{ "CellOverVoltage", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "COVP" }, "mV", 1000, "%.3f" },
	{ "CellOVRelease", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "COVP Release" }, "mV", 1000, "%.3f" },
	{ "CellOVDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "COVP Delay" }, "S", 0 },
	{ "CellUnderVoltage", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "CUVP" }, "mV", 1000, "%.3f" },
	{ "CellUVRelease", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "CUVP Release" }, "mV", 1000, "%.3f" },
	{ "CellUVDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "CUVP Delay" }, "S", 0 },
	{ "PackOverVoltage", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 99999, 1 }, 1, (char *[]){ "POVP" }, "mV", 100, "%.1f" },
	{ "PackOVRelease", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 99999, 1 }, 1, (char *[]){ "POVP Release" }, "mV", 100, "%.1f" },
	{ "PackOVDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "POVP Delay" }, "S", 0 },
	{ "PackUnderVoltage", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 99999, 1 }, 1, (char *[]){ "PUVP" }, "mV", 100, "%.1f" },
	{ "PackUVRelease", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 99999, 1 }, 1, (char *[]){ "PUVP Release" }, "mV", 100, "%.1f" },
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
};
#define NBASIC (sizeof(basic_params)/sizeof(json_descriptor_t))

static json_descriptor_t hard_params[] = {
	{ "DoubleOCSC", DATA_TYPE_BOOL, 0, 0, 0, 1, (char *[]){ "Doubled Overcurrent and short-circuit value" }, 0, 0 },
	{ "DSGOC2", DATA_TYPE_INT, "select", ndsgoc2_vals, dsgoc2_vals, ndsgoc2_vals, dsgoc2_labels, "mV", 0 },
	{ "DSGOC2Delay", DATA_TYPE_INT, "select", ndsgoc2delay_vals, dsgoc2delay_vals, ndsgoc2delay_vals, dsgoc2delay_labels, "mS", 0 },
	{ "SCValue", DATA_TYPE_INT, "select", nsc_vals, sc_vals, nsc_vals, sc_labels, "mV", 0 },
	{ "SCDelay", DATA_TYPE_INT, "select", nscdelay_vals, scdelay_vals, nscdelay_vals, scdelay_labels, "uS", 0 },
	{ "HardCellOverVoltage", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "High COVP" }, "mV", 1000, "%.3f" },
	{ "HCOVPDelay", DATA_TYPE_INT, "select", nhcovpdelay_vals, hcovpdelay_vals, nhcovpdelay_vals, hcovpdelay_labels, "S", 0 },
	{ "HardCellUnderVoltage", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "High COVP" }, "mV", 1000, "%.3f" },
	{ "HCUVPDelay", DATA_TYPE_INT, "select", nhcuvpdelay_vals, hcuvpdelay_vals, nhcuvpdelay_vals, hcuvpdelay_labels, "S", 0 },
	{ "SCRelease", DATA_TYPE_INT, "range", 3, (int []){ 0, 199, 1 }, 1, (char *[]){ "SC Release Time" }, "S", 0 },
};
#define NHARD (sizeof(hard_params)/sizeof(json_descriptor_t))

static json_descriptor_t misc_params[] = {
	{ "Mosfet", DATA_TYPE_INT, "range", 3, (int []){ 0, 3, 1 }, 1, (char *[]){ "Mosfet" }, 0, 0 },
};
#define NMISC (sizeof(misc_params)/sizeof(json_descriptor_t))

static struct parmdir {
	char *name;
	json_descriptor_t *parms;
	int count;
} allparms[] = {
	{ "Capacity Config", capacity_params, NCAP },
	{ "Function Configuration", func_params, NFUNC },
	{ "NTC Configuration", ntc_params, NNTC },
	{ "Balance Configuration", balance_params, NBAL },
	{ "GPS Configuration", gps_params, NBAL },
	{ "Other Configuration", other_params, NOTHER },
	{ "Basic Parameters", basic_params, NBASIC },
	{ "Hard Parameters", hard_params, NHARD },
	{ "Misc Parameters", misc_params, NMISC },
};
#define NALL (sizeof(allparms)/sizeof(struct parmdir))

json_descriptor_t *_getd(char *label) {
	json_descriptor_t *dp;
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

int jbd_config_add_params(json_value_t *j) {
	int x,y;
	json_value_t *ca,*o,*a;
	struct parmdir *dp;

	/* Configuration array */
	ca = json_create_array();

	for(x=0; x < NALL; x++) {
		o = json_create_object();
		dp = &allparms[x];
		if (dp->count > 1) {
			a = json_create_array();
			for(y=0; y < dp->count; y++) json_array_add_descriptor(a,dp->parms[y]);
			json_add_value(o,dp->name,a);
		} else if (dp->count == 1) {
			json_add_descriptor(o,dp->name,dp->parms[0]);
		}
		json_array_add_value(ca,o);
	}

	json_add_value(j,"configuration",ca);
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
	{ JBD_REG_MOSFET,"Mosfet", JBD_PARM_DT_INT },
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

static void jbd_get_config(jbd_session_t *s, struct jbd_params *pp, json_descriptor_t *dp, uint8_t *data, int len) {
	char topic[200];
	char str[64],temp[72];
	uint16_t val;
	float fval;
	int i,ival;

	dprintf(1,"label: %s, dt: %d\n", pp->label, pp->dt);
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
	sprintf(topic,"%s/%s/Config/%s",s->conf->topic,s->conf->name,pp->label);
	dprintf(1,"topic: %s, temp: %s\n", topic, temp);
	mqtt_pub(s->conf->m,topic,temp,1);
}

/* TODO: check value for limits in desc */
static int jbd_set_config(jbd_session_t *s, uint8_t *data, struct jbd_params *pp, json_descriptor_t *dp, solard_confreq_t *req) {
	char *p;
	int bytes,len,i,ival;
	uint16_t val;
	float fval;

	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);

	/* Len = 2 (16 bit size) for most */
	len = 2;

	dprintf(1,"pp->dt: %d, req->type: %d\n", pp->dt, req->type);
	switch(pp->dt) {
	case JBD_PARM_DT_INT:
		conv_type(DATA_TYPE_INT,&ival,0,req->type,&req->sval,0);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) ival *= dp->scale;
		val = ival;
		dprintf(1,"val: %d\n", val);
		jbd_putshort(data,val);
		break;
	case JBD_PARM_DT_TEMP:
		conv_type(DATA_TYPE_SHORT,&val,0,req->type,&req->sval,0);
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
		if (bytes < 0) return 1;
		conv_type(DATA_TYPE_SHORT,&val,0,req->type,&req->sval,0);
		data[0] = val;
		break;
	case JBD_PARM_DT_B1:
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return 1;
		conv_type(DATA_TYPE_SHORT,&val,0,req->type,&req->sval,0);
		data[1] = val;
		break;
	case JBD_PARM_DT_FUNC:
		val = 0;
		for(i=0; i < 99; i++) {
			p = strele(i,",",req->sval);
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
			p = strele(i,",",req->sval);
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
		conv_type(DATA_TYPE_FLOAT,&fval,0,req->type,&req->sval,0);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) fval *= dp->scale;
		val = fval;
		dprintf(1,"val: %d\n", val);
		jbd_putshort(data,val);
		break;
	case JBD_PARM_DT_STR:
		data[0] = 0;
		/* XXX */
		strncat((char *)data,req->sval,255);
		trim((char *)data);
		len = strlen((char *)data);
		break;
	case JBD_PARM_DT_DOUBLE:
		dprintf(1,"sval: %s\n", req->sval);
		conv_type(DATA_TYPE_BOOL,&ival,0,req->type,&req->sval,0);
		dprintf(1,"ival: %d\n", ival);
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return 1;
		dprintf(1,"data[0]: %x\n", data[0]);
		if (ival) data[0] |= 0x80;
		else data[0] &= 0x7F;
		dprintf(1,"data[0]: %x\n", data[0]);
		break;
	case JBD_PARM_DT_DSGOC2:
       		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, 2);
		if (bytes < 0) return 1;
		conv_type(DATA_TYPE_INT,&ival,0,req->type,&req->sval,0);
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
		if (bytes < 0) return 1;
		conv_type(DATA_TYPE_INT,&ival,0,req->type,&req->sval,0);
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
		if (bytes < 0) return 1;
		conv_type(DATA_TYPE_INT,&ival,0,req->type,&req->sval,0);
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
		if (bytes < 0) return 1;
		conv_type(DATA_TYPE_INT,&ival,0,req->type,&req->sval,0);
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
		if (bytes < 0) return 1;
		conv_type(DATA_TYPE_INT,&ival,0,req->type,&req->sval,0);
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
		if (bytes < 0) return 1;
		conv_type(DATA_TYPE_INT,&ival,0,req->type,&req->sval,0);
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
		return 0;
		break;
	}
	jbd_get_config(s,pp,dp,data,len);
	return len;
}

int jbd_config(void *handle, char *op, char *id, list lp) {
	jbd_session_t *s = handle;
	uint8_t data[256];
	solard_confreq_t *req;
	struct jbd_params *pp;
	json_descriptor_t *dp;
	char message[128],*p;
	int status,bytes,len;
	long start;

	dprintf(1,"op: %s\n", op);

	status = 1;

	if (jbd_eeprom_start(s) < 0) {
		strcpy(message,"unable to open eeprom");
		goto jbd_config_error;
	}

	start = mem_used();
	list_reset(lp);
	if (strcmp(op,"Get") == 0) {
		while((p = list_get_next(lp)) != 0) {
			dprintf(1,"p: %s\n", p);
			pp = _getp(p);
			if (!pp) {
				sprintf(message,"%s: not found",p);
				goto jbd_config_error;
			}
			dp = _getd(p);
			if (!dp) {
				sprintf(message,"%s: not found",p);
				goto jbd_config_error;
			}
			dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);
        		bytes = jbd_rw(s, JBD_CMD_READ, pp->reg, data, sizeof(data));
			if (bytes < 0) {
				sprintf(message,"read error");
				goto jbd_config_error;
			}
			jbd_get_config(s,pp,dp,data,bytes);
		}
	} else {
		while((req = list_get_next(lp)) != 0) {
			dprintf(1,"req: name: %s, type: %d\n", req->name, req->type);
			dprintf(1,"req value: %s\n", req->sval);
			pp = _getp(req->name);
			if (!pp) {
				sprintf(message,"%s: not found",req->name);
				goto jbd_config_error;
			}
			dp = _getd(req->name);
			if (!dp) {
				sprintf(message,"%s: not found",req->name);
				goto jbd_config_error;
			}
			len = jbd_set_config(s,data,pp,dp,req);
			if (len < 1) {
				sprintf(message,"bad data");
				goto jbd_config_error;
			}
			bytes = jbd_rw(s, JBD_CMD_WRITE, pp->reg, data, len);
			if (bytes < 0) {
				sprintf(message,"write error");
				goto jbd_config_error;
			}
		}
	}

	status = 0;
	strcpy(message,"success");
jbd_config_error:
        jbd_eeprom_end(s);
	/* If the agent is jbd_control, dont send status */
	if (strcmp(id,"jbd_control")!=0) agent_send_status(s->conf, s->conf->name, "Config", op, id, status, message);
	dprintf(1,"used: %ld\n", mem_used() - start);
	return status;
}

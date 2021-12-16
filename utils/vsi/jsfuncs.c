
#include "si.h"

#ifdef JS
enum SI_PROPERTY_ID {
	SI_PROPERTY_ID_NONE,
	SI_PROPERTY_ID_CAN_TRANSPORT,
	SI_PROPERTY_ID_CAN_TARGET,
	SI_PROPERTY_ID_CAN_TOPTS,
	SI_PROPERTY_ID_SMANET_TRANSPORT,
	SI_PROPERTY_ID_SMANET_TARGET,
	SI_PROPERTY_ID_SMANET_TOPTS,
	SI_PROPERTY_ID_SMANET_CHANNELS,
	SI_PROPERTY_ID_STATE,
	SI_PROPERTY_ID_RELAY1,
	SI_PROPERTY_ID_RELAY2,
	SI_PROPERTY_ID_S1_RELAY1,
	SI_PROPERTY_ID_S1_RELAY2,
	SI_PROPERTY_ID_S2_RELAY1,
	SI_PROPERTY_ID_S2_RELAY2,
	SI_PROPERTY_ID_S3_RELAY1,
	SI_PROPERTY_ID_S3_RELAY2,
	SI_PROPERTY_ID_GNRN,
	SI_PROPERTY_ID_S1_GNRN,
	SI_PROPERTY_ID_S2_GNRN,
	SI_PROPERTY_ID_S3_GNRN,
	SI_PROPERTY_ID_AUTOGN,
	SI_PROPERTY_ID_AUTOLODEXT,
	SI_PROPERTY_ID_AUTOLODSOC,
	SI_PROPERTY_ID_ERRCODE,
	SI_PROPERTY_ID_ERRMSG,
	SI_PROPERTY_ID_LAST_UPDATE,
};
#if 0
	char ExtSrc[16]; 	/* PvOnly, Gen, Grid, GenGrid */
	uint16_t state;
	struct {
		unsigned relay1: 1;
		unsigned relay2: 1;
		unsigned s1_relay1: 1;
		unsigned s1_relay2: 1;
		unsigned s2_relay1: 1;
		unsigned s2_relay2: 1;
		unsigned s3_relay1: 1;
		unsigned s3_relay2: 1;
		unsigned GnRn: 1;
		unsigned s1_GnRn: 1;
		unsigned s2_GnRn: 1;
		unsigned s3_GnRn: 1;
		unsigned AutoGn: 1;
		unsigned AutoLodExt: 1;
		unsigned AutoLodSoc: 1;
		unsigned Tm1: 1;
		unsigned Tm2: 1;
		unsigned ExtPwrDer: 1;
		unsigned ExtVfOk: 1;
		unsigned GdOn: 1;
		unsigned Error: 1;
		unsigned Run: 1;
		unsigned BatFan: 1;
		unsigned AcdCir: 1;
		unsigned MccBatFan: 1;
		unsigned MccAutoLod: 1;
		unsigned Chp: 1;
		unsigned ChpAdd: 1;
		unsigned SiComRemote: 1;
		unsigned OverLoad: 1;
		unsigned ExtSrcConn: 1;
		unsigned Silent: 1;
		unsigned Current: 1;
		unsigned FeedSelfC: 1;
	} bits;
	int have_battery_temp;
	float charge_voltage;
	float charge_amps;
	float charge_min_amps;
	float grid_min_amps;
	float save_charge_amps;
	int charge_mode;
	int charge_method;
	float start_temp;
	float user_soc;
	float tvolt;
	int sim;
	float sim_step;
	int interval;
	int startup_charge_mode;
	float last_battery_voltage, last_soc;
	float last_charge_voltage, last_battery_amps;
	time_t cv_start_time;
	int cv_method;
	int cv_cutoff;
	int cv_time;
	struct {
		int state;
		time_t grid_op_time;
		time_t gen_op_time;
		char grid_save[32];
		char gen_save[32];
	} ec;
	int gen_start_timeout;
	int gen_stop_timeout;
	int grid_start_timeout;
	int grid_stop_timeout;
	char notify_path[256];
	int charge_creep;
	int run_state;
	int readonly;
	struct {
		unsigned startup: 1;
		unsigned grid_state: 1;
		unsigned gen_state: 1;
		unsigned charge_from_grid: 1;
		unsigned grid_started: 1;
		unsigned gen_started: 1;
#endif
static JSBool si_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	si_session_t *bp;

	bp = JS_GetPrivate(cx,obj);
	dprintf(4,"bp: %p\n", bp);
	if (!bp) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(4,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(4,"prop_id: %d\n", prop_id);
		switch(prop_id) {
#if 0
		case SI_PROPERTY_ID_NAME:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,bp->name));
			break;
		case SI_PROPERTY_ID_CAPACITY:
			JS_NewDoubleValue(cx, bp->capacity, rval);
			break;
		case SI_PROPERTY_ID_VOLTAGE:
			JS_NewDoubleValue(cx, bp->voltage, rval);
			break;
		case SI_PROPERTY_ID_CURRENT:
			JS_NewDoubleValue(cx, bp->current, rval);
			break;
		case SI_PROPERTY_ID_NTEMPS:
			dprintf(4,"ntemps: %d\n", bp->ntemps);
			*rval = INT_TO_JSVAL(bp->ntemps);
			break;
		case SI_PROPERTY_ID_TEMPS:
		       {
				JSObject *rows;
				jsval val;

//				dprintf(4,"ntemps: %d\n", bp->ntemps);
				rows = JS_NewArrayObject(cx, bp->ntemps, NULL);
				for(i=0; i < bp->ntemps; i++) {
//					dprintf(4,"temps[%d]: %f\n", i, bp->temps[i]);
					JS_NewDoubleValue(cx, bp->temps[i], &val);
					JS_SetElement(cx, rows, i, &val);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,bp->name));
			break;
		case SI_PROPERTY_ID_NCELLS:
			dprintf(4,"ncells: %d\n", bp->ncells);
			*rval = INT_TO_JSVAL(bp->ncells);
			break;
		case SI_PROPERTY_ID_CELLVOLT:
		       {
				JSObject *rows;
				jsval val;

//				dprintf(4,"ncells: %d\n", bp->ncells);
				rows = JS_NewArrayObject(cx, bp->ncells, NULL);
				for(i=0; i < bp->ncells; i++) {
//					dprintf(4,"cellvolt[%d]: %f\n", i, bp->cellvolt[i]);
					JS_NewDoubleValue(cx, bp->cellvolt[i], &val);
					JS_SetElement(cx, rows, i, &val);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
		case SI_PROPERTY_ID_CELLRES:
		       {
				JSObject *rows;
				jsval val;

//				dprintf(1,"ncells: %d\n", bp->ncells);
				rows = JS_NewArrayObject(cx, bp->ncells, NULL);
				for(i=0; i < bp->ncells; i++) {
//					dprintf(1,"cellres[%d]: %f\n", i, bp->cellres[i]);
					JS_NewDoubleValue(cx, bp->cellres[i], &val);
					JS_SetElement(cx, rows, i, &val);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
		case SI_PROPERTY_ID_CELL_MIN:
			JS_NewDoubleValue(cx, bp->cell_min, rval);
			break;
		case SI_PROPERTY_ID_CELL_MAX:
			JS_NewDoubleValue(cx, bp->cell_max, rval);
			break;
		case SI_PROPERTY_ID_CELL_DIFF:
			JS_NewDoubleValue(cx, bp->cell_diff, rval);
			break;
		case SI_PROPERTY_ID_CELL_AVG:
			JS_NewDoubleValue(cx, bp->cell_avg, rval);
			break;
		case SI_PROPERTY_ID_CELL_TOTAL:
			JS_NewDoubleValue(cx, bp->cell_total, rval);
			break;
		case SI_PROPERTY_ID_BALANCEBITS:
			*rval = INT_TO_JSVAL(bp->balancebits);
			break;
		case SI_PROPERTY_ID_ERRCODE:
			*rval = INT_TO_JSVAL(bp->errcode);
			break;
		case SI_PROPERTY_ID_ERRMSG:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,bp->errmsg));
			break;
		case SI_PROPERTY_ID_STATE:
			*rval = INT_TO_JSVAL(bp->state);
			break;
		case SI_PROPERTY_ID_LAST_UPDATE:
			*rval = INT_TO_JSVAL(bp->last_update);
			break;
#endif
		default:
			JS_ReportError(cx, "not a property");
			*rval = JSVAL_NULL;
		}
	}
	return JS_TRUE;
}

static JSClass si_class = {
	"si",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	si_getprop,
	JS_PropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSSunnyIsland(JSContext *cx, void *priv) {
	JSPropertySpec si_props[] = { 
#if 0
		{ "name",		SI_PROPERTY_ID_NAME,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "capacity",		SI_PROPERTY_ID_CAPACITY,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "voltage",		SI_PROPERTY_ID_VOLTAGE,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "current",		SI_PROPERTY_ID_CURRENT,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "ntemps",		SI_PROPERTY_ID_NTEMPS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "temps",		SI_PROPERTY_ID_TEMPS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "ncells",		SI_PROPERTY_ID_NCELLS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cellvolt",		SI_PROPERTY_ID_CELLVOLT,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cellres",		SI_PROPERTY_ID_CELLRES,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cell_min",		SI_PROPERTY_ID_CELL_MIN,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cell_max",		SI_PROPERTY_ID_CELL_MAX,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cell_diff",		SI_PROPERTY_ID_CELL_DIFF,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cell_avg",		SI_PROPERTY_ID_CELL_AVG,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cell_total",		SI_PROPERTY_ID_CELL_TOTAL,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "balancebits",	SI_PROPERTY_ID_BALANCEBITS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "errcode",		SI_PROPERTY_ID_ERRCODE,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "errmsg",		SI_PROPERTY_ID_ERRMSG,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "state",		SI_PROPERTY_ID_STATE,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "last_update",	SI_PROPERTY_ID_LAST_UPDATE,	JSPROP_ENUMERATE | JSPROP_READONLY },
#endif
		{0}
	};
	JSFunctionSpec si_funcs[] = {
		{ 0 }
	};
	JSObject *obj;

	dprintf(5,"defining %s object\n",si_class.name);
#if 1
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &si_class, 0, 0, si_props, si_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize si class");
		return 0;
	}
#else
	if ((obj = JS_DefineObject(cx, JS_GetGlobalObject(cx), si_class.name, &si_class, NULL, JSPROP_ENUMERATE)) == 0) return 0;
	if (!JS_DefineProperties(cx, obj, si_props)) return 0;
	if (!JS_DefineFunctions(cx, obj, si_funcs)) return 0;
#endif

	/* Pre-create the JSVALs */
	JS_SetPrivate(cx,obj,priv);
	dprintf(5,"done!\n");
	return obj;
}

int si_jsinit(si_session_t *s) {
	return JS_EngineAddInitFunc(s->ap->js, "si", JSSunnyIsland, s);
}

#endif

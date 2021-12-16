#define SOLARD_AGENTINFO_SETPROP \
		case SOLARD_AGENTINFO_PROPERTY_ID_INDEX:\
			strcpy(info->index,JS_GetStringBytes(JSVAL_TO_STRING(*vp)));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_ID:\
			strcpy(info->id,JS_GetStringBytes(JSVAL_TO_STRING(*vp)));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_AGENT:\
			strcpy(info->agent,JS_GetStringBytes(JSVAL_TO_STRING(*vp)));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_PATH:\
			strcpy(info->path,JS_GetStringBytes(JSVAL_TO_STRING(*vp)));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_ROLE:\
			strcpy(info->role,JS_GetStringBytes(JSVAL_TO_STRING(*vp)));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_NAME:\
			strcpy(info->name,JS_GetStringBytes(JSVAL_TO_STRING(*vp)));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_TRANSPORT:\
			strcpy(info->transport,JS_GetStringBytes(JSVAL_TO_STRING(*vp)));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_TARGET:\
			strcpy(info->target,JS_GetStringBytes(JSVAL_TO_STRING(*vp)));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_TOPTS:\
			strcpy(info->topts,JS_GetStringBytes(JSVAL_TO_STRING(*vp)));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_MANAGED:\
			info->managed = JSVAL_TO_INT(*vp);\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_STATE:\
			info->state = JSVAL_TO_INT(*vp);\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_STARTED:\
			info->started = JSVAL_TO_INT(*vp);\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_MAX:\
			break;

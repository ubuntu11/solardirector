#define SOLARD_AGENTINFO_GETPROP \
		case SOLARD_AGENTINFO_PROPERTY_ID_INDEX:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,info->index));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_ID:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,info->id));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_AGENT:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,info->agent));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_PATH:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,info->path));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_ROLE:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,info->role));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_NAME:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,info->name));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_TRANSPORT:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,info->transport));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_TARGET:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,info->target));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_TOPTS:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,info->topts));\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_MANAGED:\
			*rval = INT_TO_JSVAL(info->managed);\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_STATE:\
			*rval = INT_TO_JSVAL(info->state);\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_STARTED:\
			*rval = INT_TO_JSVAL(info->started);\
			break;\
		case SOLARD_AGENTINFO_PROPERTY_ID_MAX:\
			break;

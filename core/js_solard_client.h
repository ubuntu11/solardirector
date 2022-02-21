#define SOLARD_CLIENT_PROPIDS \
	SOLARD_CLIENT_PROPERTY_ID_NAME=127,\
	SOLARD_CLIENT_PROPERTY_ID_SECTION_NAME=126,\
	SOLARD_CLIENT_PROPERTY_ID_RTSIZE=125

#define SOLARD_CLIENT_GETPROP \
		case SOLARD_CLIENT_PROPERTY_ID_NAME:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,c->name));\
			break;\
		case SOLARD_CLIENT_PROPERTY_ID_SECTION_NAME:\
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,c->section_name));\
			break;\
		case SOLARD_CLIENT_PROPERTY_ID_RTSIZE:\
			*rval = INT_TO_JSVAL(c->rtsize);\
			break;

#define SOLARD_CLIENT_SETPROP \
		case SOLARD_CLIENT_PROPERTY_ID_NAME:\
			jsvaltotype(DATA_TYPE_STRING,&c->name,sizeof(c->name)-1,cx,*vp);\
			break;\
		case SOLARD_CLIENT_PROPERTY_ID_SECTION_NAME:\
			jsvaltotype(DATA_TYPE_STRING,&c->section_name,sizeof(c->section_name)-1,cx,*vp);\
			break;\
		case SOLARD_CLIENT_PROPERTY_ID_RTSIZE:\
			jsvaltotype(DATA_TYPE_S32,&c->rtsize,0,cx,*vp);\
			break;

#define SOLARD_CLIENT_PROPSPEC \
		{ "name",SOLARD_CLIENT_PROPERTY_ID_NAME,JSPROP_ENUMERATE },\
		{ "section_name",SOLARD_CLIENT_PROPERTY_ID_SECTION_NAME,JSPROP_ENUMERATE },\
		{ "rtsize",SOLARD_CLIENT_PROPERTY_ID_RTSIZE,JSPROP_ENUMERATE }


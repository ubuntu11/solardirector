
#include "sb.h"

// Download endpoint:data/ObjectMetadata_Istl.json and process it
int getsbobjects(sb_session_t *s) {
	sb_request(s,"data/ObjectMetadata_Istl.json");
}

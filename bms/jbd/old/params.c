

static struct agent_caps my_caps[] = {
	0,
};

static struct agent_params my_params[] = {
	{ "Interval", DATA_TYPE_INT, 0, 300, 1, UNIT_SECONDS },
	{ 0,0,0,0,0,0 }
}

static struct agent_info my_info = {
	"jbdagent",
	"SolarD agent for JBD BMS",
	"1.0",
	"Stephen Shoecraft",
	"JBD",
	info->model,
	my_caps,
	my_params
}

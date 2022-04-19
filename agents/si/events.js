
function events_init() {
	si.register(script_name,"si_event_handler");
}

function si_event_handler(name,value) {
	dprintf(0,"name: %s, value: %s\n", name, value);
}

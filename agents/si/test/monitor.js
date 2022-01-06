
printf("*********************** MONITOR ***************************\n");

var dlevel = 0;
var smanet = null;
var smanet_initialized = false;
var smanet_info = false;

// Are we still starting up?
dprintf(dlevel,"startup: %d\n", si.startup);
if (si.startup) exit(0);

function init_smanet() {
	if (typeof(smanet) == "undefined") {
		dprintf(dlevel,"transport: %s, target: %s, topts: %s\n",
			si.smanet_transport, si.smanet_target, si.smanet_topts);
		if (si.smanet_transport.length && si.smanet_target.length && si.smanet_topts.length) {
			var smanet = SMANET(si.smanet_transport, si.smanet_target, si.smanet_topts);
			dprintf(dlevel,"smanet: %s\n", smanet);
			dprintf(dlevel,"smanet type: %s\n", typeof(smanet));
			if (typeof(smanet) != "undefined") {
				smanet_initialized = true;
				return smanet;
			}
		}
	} else {
		smanet_initialized = true;
	}
}

// Init SMANET 
if (!smanet_info) {
	dprintf(dlevel,"smanet_initialized: %s\n", smanet_initialized);
	dprintf(dlevel,"smanet_connnected: %s\n", smanet.connected);
	if (!smanet_initialized) smanet = init_smanet();
	else if (!smanet.connected) {
		dprintf(dlevel,"transport: %s, target: %s, topts: %s\n",
			si.smanet_transport, si.smanet_target, si.smanet_topts);
		if (si.smanet_transport.length && si.smanet_target.length && si.smanet_topts.length)
			if (smanet.connect()) dprintf(dlevel,"unable to connect\n");
	}
	if (smanet.connected) {
		dprintf(dlevel,"type: %s\n", smanet.type);
		var chanfile = si.libdir + "/" + smanet.type + ".dat";
		dprintf(dlevel,"chanfile: %s\n", chanfile);
		var have_chans = true;
		if (smanet.load_channels(chanfile)) {
			dprintf(dlevel,"error loading channels, getting chans\n");
			if (smanet.get_channels()) {
				log_warning("unable to get SMANET channels: %s\n",smanet.errmsg);
				have_chans = false;
			} else {
				if (smanet.save_channels(chanfile))
					log_warning("unable to save SMANET channels: %s\n", smanet.errmsg);
			}
		}
		if (have_chans) {
			// Get some info
			var GdManStr = smanet.get("GdManStr");
			smanet_info = true;
		}
	}
}

printf("gd: %s\n", smanet.get("GdManStr"));

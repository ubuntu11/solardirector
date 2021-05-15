
#include "client.h"

int main(int argc, char **argv) {
	char *args[32];
	int i;

	client_init(argc,argv,0,"t");

	debug = 9;

	i = 0;
	args[i++] = "c:/sd/bin/jbd.exe";
	args[i++] = "-t";
	args[i++] = "ip,pack_01";
	args[i++] = "-n";
	args[i++] = "pack_01";
	args[i++] = "-m";
	args[i++] = "192.168.1.5";
	args[i] = 0;
	solard_exec(args[0],args,0,0);
}


sd.interval = 3;

dprintf(1,"%s started\n", script_name);

if (typeof test1 == "undefined") test1 = 2;

dprintf(1,"test1: %d\n", test1);
if (test1 == 1) test1 = 2;
else if (test1 != 2) test1 = 1;

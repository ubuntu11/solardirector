

Preh cell monitor linux-based command line utility

requires MYBMM for the modules and utils

only supports CAN bus

this CMU came with my BMW battery packs.  Together with the smart contactors you can monitor and control the battery packs.  The contactors have shunts for measuring current.  they have CAN bus output for voltage, contactor status, etc.  good stuff!

this util just dumps what CMUs have what cells


For CAN:

preh_dump -t can:<device>[,speed]

example:

	preh_dump -t can:can0,500000


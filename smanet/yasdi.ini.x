[DriverModules]
Driver0=yasdi_drv_serial
Driver1=yasdi_drv_ttyip

[IP0.Protocol]
Device0=192.168.2.2:3900,/dev/ttyS0,9600
Media=tty_ip
Protocol=SMANet

[COM1]
Device=/dev/ttyS0
Media=RS232
Baudrate=9600
Protocol=SMANet

[IP1]
Protocol=SMANet
Device0=192.168.2.111

[Misc]
DebugOutput=/dev/stdout

[Plant]
deviceCount=1

[Log]
level=0

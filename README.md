
IMPORTANT: INSTALL THE FOLLOWING DEPS FIRST
paho.mqtt.c (https://github.com/eclipse/paho.mqtt.c.git)
	mkdir -p build && cd build
	cmake -DPAHO_BUILD_SHARED=TRUE -DPAHO_BUILD_STATIC=TRUE -DPAHO_ENABLE_TESTING=FALSE -DPAHO_BUILD_SAMPLES=TRUE ..
	make && make install
gattlib (https://github.com/labapart/gattlib.git)
	mkdir -p build && cd build
	cmake -DGATTLIB_BUILD_EXAMPLES=NO -DGATTLIB_SHARED_LIB=NO -DGATTLIB_BUILD_DOCS=NO -DGATTLIB_PYTHON_INTERFACE=NO ..
	make && make install
libcurl4-openssl-dev
libreadline-gplv2-dev


IF USING BLUETOOTH, YOU MUST PAIR THE DEVICE FIRST
bluetoothctl 
	# scan on
	(look for your device)
	[NEW] Device XX:XX:XX:XX:XX:XX name
	# pair XX:XX:XX:XX:XX:XX
	(it may ask for your passkey)

This project is intended to manage my entire solar system


bms/jbd - jbd bms agent
bms/jk - jbd bms agent
inverter/si - sma sunny island agent
inverter/sb - sma sunny boy agent

agents will report to mqtt at regular interval
can set agent config with mqtt messages

util/sdconfig can be used to get/set 

sdconfig Battery/pack_01 Get BalanceWindow
sdconfig Battery/pack_01 Set BalanceWindow 0.15

sdconfig Inverter/"Sunny Island" Get GdManStr
sdconfig Inverter/"Sunny Island" Set GdManStr Start




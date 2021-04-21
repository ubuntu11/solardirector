
This project is intended to manage my entire solar system

bms/jbd - jbd bms agent
inverter/si - sma sunny island agent

agents will report to mqtt at regular interval
can set agent config with mqtt messages

util/sdconfig can be used to get/set 

sdconfig Battery/pack_01 Get BalanceWindow
sdconfig Battery/pack_01 Set BalanceWindow 0.15

sdconfig Inverter/"Sunny Island" Get GdManStr
sdconfig Inverter/"Sunny Island" Set GdManStr Start

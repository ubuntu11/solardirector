#!/usr/bin/python3

#from sma_sunnyboy import WebConnect, Key, Right
from sma import WebConnect, Key, Right
import json
import time

#address = "192.168.2.111" 			# address of SMA
address = "192.168.1.5" 			# address of SMA
password = "Bgt56yhn!"		# your user password
right = Right.USER				# the connexion level

# create object
print("creating client...");
client = WebConnect(address, right, password)

# authenticate with credentials
print("autyh...");
client.auth()

# check connection state
if not client.check_connection():
	print("[!] Cannot authenticate to the server, check your credentials")
else:
	print("[+] Connected to SMA")
	print("[*] getting some data")

	for number in range(100):
		pow_current = client.get_value(Key.power_current)
		print("[*] Current production: %d%s" % (pow_current, Key.power_current["unit"]))
		time.sleep(.5)

#	all = client.get_all_values()
#	all = client.get_all_keys()
#	print(json.dumps(all,indent=4))

	if 0 == 1:
		# get the production counter from solar panel
		power_total = client.get_value(Key.power_total)
		print("[*] Production Counter: %d%s" % (power_total, Key.power_total["unit"]))

		# get the current production from solar panel
		pow_current = client.get_value(Key.power_current)
		print("[*] Current production: %d%s" % (pow_current, Key.power_current["unit"]))

	# Don't forget to disconnect from web server
	print("[+] Disconnecting..")
	if client.logout() is False:
		print("[!] Error in logout!")
	print("[+] Done.")

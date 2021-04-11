
Sunny Island linux-based command line utility

For CAN:

sitool -t can:<device>[,speed]

example:

	sitool -t can:can0,500000


for RDEV/rCAN:

sitool -t can_ip:<ip addr>:[port],<interface>

example:

	sitool -t rdev:10.0.0.1:3930,can0


To start the Sunny Island remotey:

sitool -t can:<device>[,speed] -s

example:

	sitool -t can:can0,500000 -s


To stop the Sunny Island remotey:

sitool -t can:<device>[,speed] -S

example:

	sitool -t can:can0,500000 -S

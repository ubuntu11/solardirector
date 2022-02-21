#ifndef __INSTANT_H
#define __INSTANT_H

#define ALL 1

static char *instant_keys[] = {
#if ALL == 1
	"6100_00416600",	/* Status -> Operation -> Waiting time until feed-in */
#endif
	"6180_08214800",	/* Status -> Operation -> Condition */
#if ALL == 1
	"6180_08412800",	/* Status -> Operation -> General operating status */
	"6180_08416400",	/* Status -> Operation -> Grid relay status */
	"6180_08416500",	/* Status -> Operation -> Reason for derating */
	"6100_00411E00",	/* Status -> Operation -> Device status -> Ok */
	"6100_00411F00",	/* Status -> Operation -> Device status -> Warning */
	"6100_00412000",	/* Status -> Operation -> Device status -> Fault */
#endif
	"6180_08414C00",	/* Status -> Operation -> Inverter -> Condition */
#if ALL == 1
	"6100_00418000",	/* Status -> Operation -> Current event -> Event number manufacturer */
	"6180_08414900",	/* Status -> Operation -> Current event -> Message */
	"6180_08414A00",	/* Status -> Operation -> Current event -> Recommended action */
	"6180_08414B00",	/* Status -> Operation -> Current event -> Fault correction measure */
	"6180_08413300",	/* Status -> Operation -> PV system control -> Status */
	"6180_08413200",	/* Status -> Operation -> Device control -> Status */
	"6180_08412900",	/* Status -> Update -> Status */
	"6180_08436800",	/* Device -> Operation -> Backup mode status */
#endif
	"6200_40451E00",	/* DC Side -> DC measurements -> Power */
	"6380_40451F00",	/* DC Side -> DC measurements -> Voltage */
	"6380_40452100",	/* DC Side -> DC measurements -> Current */
//	"6400_00453300",	/* DC Side -> DC measurements -> Energy released by string */
//	"6180_08452D00",	/* DC Side -> SunSpec signal -> SunSpec keepalive signal */
	"6100_40263F00",	/* AC Side -> Grid measurements -> Power */
	"6100_00465700",	/* AC Side -> Grid measurements -> Grid frequency */
//	"6180_08465A00",	/* AC Side -> Grid measurements -> Excitation type of cos φ */
	"6100_40465300",	/* AC Side -> Grid measurements -> Phase currents -> Phase L1 */
	"6100_40465400",	/* AC Side -> Grid measurements -> Phase currents -> Phase L2 */
	"6100_40465500",	/* AC Side -> Grid measurements -> Phase currents -> Phase L3 */
	"6100_00464800",	/* AC Side -> Grid measurements -> Phase voltage -> Phase L1 */
	"6100_00464900",	/* AC Side -> Grid measurements -> Phase voltage -> Phase L2 */
	"6100_00464A00",	/* AC Side -> Grid measurements -> Phase voltage -> Phase L3 */
#if ALL == 1
	"6100_00464B00",	/* AC Side -> Grid measurements -> Phase voltage -> Phase L1 against L2 */
	"6100_00464C00",	/* AC Side -> Grid measurements -> Phase voltage -> Phase L2 against L3 */
	"6100_00464D00",	/* AC Side -> Grid measurements -> Phase voltage -> Phase L3 against L1 */
	"6100_40571F00",	/* AC Side -> Grid measurements -> Power -> Photovoltaics */
	"6100_40464000",	/* AC Side -> Grid measurements -> Active power -> Phase L1 */
	"6100_40464100",	/* AC Side -> Grid measurements -> Active power -> Phase L2 */
	"6100_40464200",	/* AC Side -> Grid measurements -> Active power -> Phase L3 */
#endif
	"6400_00260100",	/* AC Side -> Measured values -> Total yield */
	"6400_00262200",	/* AC Side -> Measured values -> Day yield */
#if ALL == 1
	"6400_00462E00",	/* AC Side -> Measured values -> Operating time */
	"6400_00462F00",	/* AC Side -> Measured values -> Feed-in time */
	"6100_40463600",	/* AC Side -> Measured values -> Grid measurements -> Supplied power */
	"6100_40463700",	/* AC Side -> Measured values -> Grid measurements -> Power absorbed */
	"6100_00464E00",	/* AC Side -> Measured values -> Grid measurements -> Displacement power factor */
	"6100_00468100",	/* AC Side -> Measured values -> Grid measurements -> Grid frequency */
	"6100_40468F00",	/* AC Side -> Measured values -> Grid measurements -> Apparent power */
	"6100_40469900",	/* AC Side -> Measured values -> Grid measurements -> EEI displacement power factor */
	"6100_4046F100",	/* AC Side -> Measured values -> Grid measurements -> Reactive power */
	"6400_00462400",	/* AC Side -> Measured values -> Grid measurements -> Total yield */
	"6400_00462500",	/* AC Side -> Measured values -> Grid measurements -> Absorbed energy */
	"6400_00469100",	/* AC Side -> Measured values -> Grid measurements -> Total counter reading, feed-in counter */
	"6400_00469200",	/* AC Side -> Measured values -> Grid measurements -> Total energy absorbed from the grid by the device */
	"6100_40466500",	/* AC Side -> Measured values -> Grid measurements -> Phase currents -> Phase L1 */
	"6100_40466600",	/* AC Side -> Measured values -> Grid measurements -> Phase currents -> Phase L2 */
	"6100_40466B00",	/* AC Side -> Measured values -> Grid measurements -> Phase currents -> Phase L3 */
	"6100_00467700",	/* AC Side -> Measured values -> Grid measurements -> Phase voltage -> Phase L3 against L1 */
	"6100_00467800",	/* AC Side -> Measured values -> Grid measurements -> Phase voltage -> Phase L1 against L2 */
	"6100_00467900",	/* AC Side -> Measured values -> Grid measurements -> Phase voltage -> Phase L2 against L3 */
	"6100_0046E500",	/* AC Side -> Measured values -> Grid measurements -> Phase voltage -> Phase L1 */
	"6100_0046E600",	/* AC Side -> Measured values -> Grid measurements -> Phase voltage -> Phase L2 */
	"6100_0046E700",	/* AC Side -> Measured values -> Grid measurements -> Phase voltage -> Phase L3 */
	"6100_40466C00",	/* AC Side -> Measured values -> Grid measurements -> Apparent power -> Phase L1 */
	"6100_40466D00",	/* AC Side -> Measured values -> Grid measurements -> Apparent power -> Phase L2 */
	"6100_40466E00",	/* AC Side -> Measured values -> Grid measurements -> Apparent power -> Phase L3 */
	"6100_4046EE00",	/* AC Side -> Measured values -> Grid measurements -> Reactive power -> Phase L1 */
	"6100_4046EF00",	/* AC Side -> Measured values -> Grid measurements -> Reactive power -> Phase L2 */
	"6100_4046F000",	/* AC Side -> Measured values -> Grid measurements -> Reactive power -> Phase L3 */
	"6100_0046E800",	/* AC Side -> Measured values -> Grid measurements -> Active power -> Phase L1 */
	"6100_0046E900",	/* AC Side -> Measured values -> Grid measurements -> Active power -> Phase L2 */
	"6100_0046EA00",	/* AC Side -> Measured values -> Grid measurements -> Active power -> Phase L3 */
	"6100_0046EB00",	/* AC Side -> Measured values -> Grid measurements -> Active power consumed -> Phase L1 */
	"6100_0046EC00",	/* AC Side -> Measured values -> Grid measurements -> Active power consumed -> Phase L2 */
	"6100_0046ED00",	/* AC Side -> Measured values -> Grid measurements -> Active power consumed -> Phase L3 */
	"6180_0846A600",	/* AC Side -> Operation -> Mains connection */
#endif
	"6100_0046C200",	/* AC Side -> PV generation -> PV generation power */
#if ALL == 1
	"6400_0046C300",	/* AC Side -> PV generation -> Meter count and PV gen. meter */
	"6100_0046C600",	/* AC Side -> Emergency power measurements -> Voltage */
	"6100_0046C700",	/* AC Side -> Emergency power measurements -> Current */
	"6100_0046C800",	/* AC Side -> Emergency power measurements -> Power */
	"6180_084A2C00",	/* System communication -> Measured values -> Modbus -> Energy meter used */
	"6180_084A2E00",	/* System communication -> Measured values -> Modbus -> Condition */
	"6180_084AAA00",	/* System communication -> Measured values -> Meter on Speedwire -> Condition */
	"6180_104A9A00",	/* System communication -> Speedwire -> Current IP address */
	"6180_104A9B00",	/* System communication -> Speedwire -> Current subnet mask */
	"6180_104A9C00",	/* System communication -> Speedwire -> Current gateway address */
	"6180_104A9D00",	/* System communication -> Speedwire -> Current DNS server address */
	"6180_084A9600",	/* System communication -> Speedwire -> SMACOM A -> Status */
	"6180_084A9700",	/* System communication -> Speedwire -> SMACOM A -> Connection speed */
	"6180_084A9800",	/* System communication -> Speedwire -> SMACOM B -> Status */
	"6180_084A9900",	/* System communication -> Speedwire -> SMACOM B -> Connection speed */
	"6100_004AB600",	/* System communication -> WLAN -> Signal strength of the selected network */
	"6180_084A6400",	/* System communication -> WLAN -> Soft Access Point status */
	"6180_104AB700",	/* System communication -> WLAN -> Current IP address */
	"6180_104AB800",	/* System communication -> WLAN -> Current subnet mask */
	"6180_104AB900",	/* System communication -> WLAN -> Current gateway address */
	"6180_104ABA00",	/* System communication -> WLAN -> Current DNS server address */
	"6180_084ABB00",	/* System communication -> WLAN -> Status of the scan */
	"6180_084ABC00",	/* System communication -> WLAN -> Connection status */
	"6180_084ABD00",	/* System communication -> WLAN -> Antenna type */
	"6180_084B2300",	/* External Communication -> Operation -> EnnexOS -> Condition */
	"6180_084B1E00",	/* External Communication -> Webconnect -> Status */
	"6180_00522700",	/* System and device control -> Operation -> Specification -> Active power limitation P */
	"6180_40522800",	/* System and device control -> Operation -> Specification -> Reactive power mode */
	"6180_00522900",	/* System and device control -> Operation -> Specification -> cos φ */
	"6180_08522A00",	/* System and device control -> Operation -> Specification -> Excitation type cos φ */
	"6180_08522F00",	/* System and device control -> Operation -> Active power limitation P -> Status */
#endif
};
#define instant_keys_count (sizeof(instant_keys)/sizeof(char *))


#endif

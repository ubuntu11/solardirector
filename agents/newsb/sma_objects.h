
#ifndef True
#define True 1
#endif

static struct _sma_object {
	char *id;
	int tag;
	int taghier[8];
	char *units;
	float mult;
} sma_objects[] = {
	{ "6180_084B2300", 240, { 4246 }, },
	{ "6180_08452D00", 4823, { 4821 } },
	{ "6180_08436800", 1837, { 50, 832, 309 } },

	{ "6180_08412800", 239, { 50, 309 } },
	{ "6180_08414C00", 240, { 50, 267 } },

	{ "6100_00464E00", 1401, { 835, 290, 230 } },
	{ "6400_00469100", 419, { 835, 290, 230 } },
	{ "6400_00469200", 420, { 835, 290, 230 } },

	{ "6100_40466500", 325, { 835, 290, 230, 20 } },
	{ "6100_40466600", 327, { 835, 290, 230, 20 } },
	{ "6100_40466B00", 329, { 835, 290, 230, 20 } },
	{ "6100_40466C00", 325, { 835, 290, 230, 436 } },
	{ "6100_40466D00", 327, { 835, 290, 230, 436 } },
	{ "6100_40466E00", 329, { 835, 290, 230, 436 } },
	{ "6100_00467700", 2098, { 835, 290, 230, 331 } },
	{ "6100_00467800", 1562, { 835, 290, 230, 331 } },
	{ "6100_00467900", 2097, { 835, 290, 230, 331 } },
	{ "6100_0046E500", 325, { 835, 290, 230, 331 } },
	{ "6100_0046E600", 325, { 835, 290, 230, 331 } },
	{ "6100_0046E700", 325, { 835, 290, 230, 331 } },

	{ "6100_00468100", 254, { 835, 290, 230, } },
	{ "6100_40468F00", 412, { 835, 290, 230, } },
	{ "6100_40469900", 3497, { 835, 290, 230, } },

	{ "6100_0046C600", 441, { 835, 2484 } },
	{ "6100_0046C700", 38, { 835, 2484 } },
	{ "6100_0046C800", 416, { 835, 2484 } },

	{ "6100_4046EE00", 325, { 835, 290, 230, 413 } },
	{ "6100_4046EF00", 327, { 835, 290, 230, 413 } },
	{ "6100_4046F000", 329, { 835, 290, 230, 413 } },

	{ "6100_0046E800", 325, { 835, 290, 230, 448 } },
	{ "6100_0046E900", 327, { 835, 290, 230, 448 } },
	{ "6100_0046EA00", 329, { 835, 290, 230, 448 } },

	{ "6100_0046EB00", 325, { 835, 290, 230, 824 } },
	{ "6100_0046EC00", 327, { 835, 290, 230, 824 } },
	{ "6100_0046ED00", 329, { 835, 290, 230, 824 } },

	{ "6100_4046F100", 413, { 835,290,230, }  },

	{ "6100_40571F00", 333, { 835,230,416 }  },

	{ "6800_108A5400", 717, { 839,3313, }  },
	{ "6180_084A6400", 3317, { 839,3313, }  },
	{ "6180_084ABD00", 3323, { 839,3313, }  },
	{ "6A12_40923A00", 2043, { 847,267,1024,1192, }  },
	{ "6100_40666800", 325, { 835,230,436, }  },
	{ "6800_40A63B00", 2475, { 835,290,230, }  },
	{ "6200_40263F00", 416, { 835,230, }  },
	{ "6180_08465A00", 1040, { 835,230 } },
	{ "6802_40879400", 36, { 836,229,53, }  },
	{ "6802_00876000", 3484, { 836,229,53,442, }  },
	{ "6802_00875200", 245, { 836,229,53,442, }  },
	{ "6802_08925900", 1023, { 847,267,1052, }  },
	{ "6200_00665900", 1401, { 835,230, }  },
	{ "6100_00465000", 325, { 835,230,20, }  },
	{ "6802_00919900", 393, { 846,299,921, }  },
	{ "6202_40254E00", 217, { 834,268, }  },
	{ "6800_00832A00", 314, { 832,267, }  },
	{ "6802_00927700", 3509, { 847,267,1602,2139, }  },
	{ "6800_088B5200", 733, { 840,1716, }  },
	{ "6100_40666100", 327, { 835,230,437, }  },
	{ "6802_00912300", 348, { 846,299,900, }  },
	{ "6200_00464C00", 2097, { 835,230,331, }  },
	{ "6180_08522A00", 2512, { 847,309,2508, }  },
	{ "6800_088A5100", 3321, { 839,3313, }  },
	{ "6800_10841E00", 591, { 833,584, }  },
	{ "6100_00411F00", 455, { 830,309,241, }  },
	{ "6180_084A2C00", 250, { 839,290,1628 }  },
	{ "6180_084A2E00", 240, { 839,290,1628 }  },
	{ "6800_008AA200", 2244, { 839,290,2314, }  },
	{ "6802_00926D00", 1224, { 847,267,1024,1043, }  },
	{ "6802_00874D00", 449, { 836,229,53, }  },
	{ "6802_00855D00", 1202, { 834,62,1201, }  },
	{ "6802_00924200", 448, { 847,267,1048,1049, }  },
	{ "6802_00877F00", 1103, { 836,229,53, }  },
	{ "6802_00877800", 1101, { 836,229,53,442, }  },
	{ "6800_08871E00", 53, { 836,229, }  },
	{ "6400_0046C300", 2366, { 835,2360, }  },
	{ "6802_00927400", 1990, { 847,267,1024,1043, }  },
	{ "6802_00833300", 380, { 832,267, }  },
	{ "6A02_08923600", 1040, { 847,267,1024,1194, }  },
	{ "6802_008B8A00", 2010, { 840,2018,2019, }  },
	{ "6A12_40924B00", 2042, { 847,267,1048,1193, }  },
	{ "6802_00878400", 245, { 836,229,53,222, }  },
	{ "6800_00912100", 393, { 846,299,900, }  },
	{ "6100_40463700", 890, { 835,290,230, }  },
	{ "6200_00464D00", 2098, { 835,230,331, }  },
	{ "6802_00832E00", 1061, { 832,267, }  },
	{ "6802_00877500", 2111, { 836,229,53,442, }  },
	{ "6200_40465500", 329, { 835,230,20, }  },
	{ "6802_00927100", 1047, { 847,267,1024,1043, }  },
	{ "6400_00462E00", 409, { 835,290, }  },
	{ "6802_4092A500", 2282, { 847,267,1602,2280, }  },
	{ "6A02_40924B00", 2042, { 847,267,1048,1193, }  },
	{ "6300_40251E00", 450, { 834,67, }  },
	{ "6400_00453300", 459, { 834,67, }  },
	{ "6100_40465400", 327, { 835,230,20, }  },
	{ "6802_00832F00", 1062, { 832,267, }  },
	{ "6100_00465100", 327, { 835,230,20, }  },
	{ "6A02_40923700", 3481, { 847,267,1024,1194, }  },
	{ "6802_00877600", 351, { 836,229,53,442, }  },
	{ "6802_00912B00", 348, { 846,299,901, }  },
	{ "6802_08872B00", 323, { 836,229,53, }  },
	{ "6200_00543200", 1343, { 849,2362, }  },
	{ "6180_0846A600", 1459, { 835,309, }  },
	{ "6180_104AB900", 1714, { 839,3313, }  },
	{ "6200_40665B00", 3497, { 835,230, }  },
	{ "6802_00864000", 256, { 835,267,304, }  },
	{ "6802_00874E00", 287, { 836,229,53,442, }  },
	{ "6800_08837800", 725, { 832,56, }  },
	{ "6802_00926500", 1329, { 847,267,1024,1025, }  },
	{ "6800_0092D700", 315, { 847,2113, }  },
	{ "680E_10821E00", 284, { 831,299, }  },
	{ "6802_00878500", 246, { 836,229,53,222, }  },
	{ "6200_00543800", 2375, { 849,2362, }  },
	{ "6200_40463600", 1408, { 835,290,230, }  },
	{ "6802_00878F00", 1102, { 836,229,53,222, }  },
	{ "6100_00464B00", 1562, { 835,230,331, }  },
	{ "6802_108B8B00", 2112, { 840,2018,2019, }  },
	{ "6180_104A9D00", 1715, { 839,1708, }  },
	{ "6180_08416400", 238, { 830,309, }  },
	{ "6802_08833900", 3217, { 832,394, }  },
	{ "6802_4092A300", 2282, { 847,267,1602,2279, }  },
	{ "6A02_00923500", 1039, { 847,267,1024,1194, }  },
	{ "6100_00543800", 2375, { 849,2362, }  },
	{ "6100_00411E00", 307, { 830,309,241, }  },
	{ "6802_0092E700", 2502, { 847,267,2500,1079, }  },
	{ "6400_00543B00", 1343, { 849,2361, }  },
	{ "6802_00878800", 281, { 836,229,53,222, }  },
	{ "6802_08879900", 50, { 836,229,53,28,1332, }  },
	{ "6200_40651F00", 441, { 834,67, }  },
	{ "6802_4092EE00", 2504, { 847,267,2500,1075, }  },
	{ "6802_00851E00", 387, { 834,62, }  },
	{ "6400_00462F00", 411, { 835,290, }  },
	{ "6180_104A9A00", 1712, { 839,1708, }  },
	{ "6802_40923100", 1039, { 847,267,1024,1038, }  },
	{ "6180_084AAA00", 240, { 839,290,2314, }  },
	{ "6802_00879300", 2014, { 836,229,53,222, }  },
	{ "6200_40666100", 327, { 835,230,437, }  },
	{ "6100_00543300", 2368, { 849,2362, }  },
	{ "6200_40665F00", 413, { 835,230, }  },
	{ "6800_00922600", 2140, { 847,2113,3360, }  },
	{ "6802_00B12200", 880, { 846,299,900, }  },
	{ "6802_00926300", 1219, { 847,267,1024,1025, }  },
	{ "6800_008B7100", 330, { 840,1628,1629, }  },
	{ "6802_00873500", 234, { 836,229,53, }  },
	{ "6200_40464000", 325, { 835,230,448, }  },
	{ "6802_00874F00", 288, { 836,229,53,442, }  },
	{ "6322_00851F00", 385, { 834,62, }  },
	{ "6802_00875600", 281, { 836,229,53,442, }  },
	{ "6100_00416600", 350, { 830,309, }  },
	{ "6100_40464000", 325, { 835,230,448, }  },
	{ "680E_08822000", 294, { 831,299, }  },
	{ "6802_08923200", 1040, { 847,267,1024,1038, }  },
	{ "6800_088E7B00", 1590, { 843,309, }  },
	{ "6200_40666200", 329, { 835,230,437, }  },
	{ "6800_108A5300", 716, { 839,3313, }  },
	{ "6802_00873600", 233, { 836,229,53, }  },
	{ "6100_00543600", 2374, { 849,2362, }  },
	{ "6800_00A21E00", 372, { 831,299, }  },
	{ "6102_00254F00", 272, { 834,268, }  },
	{ "6802_00927800", 1227, { 847,267,1230, }  },
	{ "6200_40666800", 325, { 835,230,436, }  },
	{ "6A12_08923600", 1040, { 847,267,1024,1194, }  },
	{ "6100_00464900", 327, { 835,230,331, }  },
	{ "6200_00464800", 325, { 835,230,331, }  },
	{ "6180_084ABB00", 3365, { 839,3313, }  },
	{ "6180_08414A00", 335, { 830,309,888, }  },
	{ "6802_00876300", 3483, { 836,229,53,442, }  },
	{ "6A12_0892B000", 1690, { 847,267, }  },
	{ "6200_00465200", 329, { 835,230,20, }  },
	{ "6322_00851E00", 387, { 834,62, }  },
	{ "6802_00875300", 246, { 836,229,53,442, }  },
	{ "6802_00878E00", 1101, { 836,229,53,222, }  },
	{ "6802_00832B00", 315, { 832,267, }  },
	{ "6180_08671E00", 53, { 836,229, }  },
	{ "6180_08214800", 240, { 830,309, }  },
	{ "6802_00879000", 2011, { 836,229,53,222, }  },
	{ "6802_08872000", 1369, { 836,229,53, }  },
	{ "6200_00465700", 254, { 835,230, }  },
	{ "6802_00876200", 3482, { 836,229,53,442, }  },
	{ "6802_00875000", 242, { 836,229,53,442, }  },
	{ "6802_08924100", 316, { 847,267,1048, }  },
	{ "6100_40666000", 325, { 835,230,437, }  },
	{ "6100_00412000", 35, { 830,309,241, }  },
	{ "6A02_40924900", 448, { 847,267,1048,1193, }  },
	{ "6800_088A5200", 1718, { 839,3313, }  },
	{ "6100_00543100", 2367, { 849,2362, }  },
	{ "6180_08522F00", 50, { 847,309,2509, }  },
	{ "6802_00928700", 1195, { 847,267,1080, }  },
	{ "6800_008AA300", 3199, { 839,290,2314, }  },
	{ "6180_104ABA00", 1715, { 839,3313, }  },
	{ "6802_00879500", 37, { 836,229,53, }  },
	{ "6802_00925A00", 1054, { 847,267,1052,1053, }  },
	{ "6100_00465200", 329, { 835,230,20, }  },
	{ "6802_00922000", 1065, { 847,267, }  },
	{ "6802_00877900", 1102, { 836,229,53,442, }  },
	{ "6802_08922900", 1022, { 847,267,1024, }  },
	{ "6800_00823400", 875, { 831,299, }  },
	{ "6200_40666A00", 329, { 835,230,436, }  },
	{ "6802_08925D00", 1057, { 847,267,1052,1053, }  },
	{ "6100_00543200", 1343, { 849,2362, }  },
	{ "6A12_40923900", 1031, { 847,267,1024,1192, }  },
	{ "6A12_40924900", 448, { 847,267,1048,1193, }  },
	{ "6800_108A5600", 714, { 839,3313, }  },
	{ "6A12_40924A00", 1050, { 847,267,1048,1193, }  },
	{ "6802_00823300", 533, { 831,299, }  },
	{ "6200_00543600", 2374, { 849,2362, }  },
	{ "6802_08822200", 574, { 831,299, }  },
	{ "6802_00864100", 255, { 835,267,304, }  },
	{ "6802_0892E200", 1973, { 847,267,1602,2139, }  },
	{ "6100_40251E00", 450, { 834,67, }  },
	{ "6800_08838C00", 3258, { 832,703, }  },
	{ "6802_00922300", 1621, { 847,267, }  },
	{ "6200_40465400", 327, { 835,230,20, }  },
	{ "6802_08926F00", 1044, { 847,267,1024,1043, }  },
	{ "6802_0092E000", 1606, { 847,267,1602,2139, }  },
	{ "6200_00465100", 327, { 835,230,20, }  },
	{ "6800_108A5A00", 3320, { 839,3313,817, }  },
	{ "6400_00462400", 418, { 835,290,230, }  },
	{ "6400_00260100", 418, { 835,290, }  },
	{ "6100_40465500", 329, { 835,230,20, }  },
	{ "6180_40522800", 2510, { 847,309,2508, }  },
	{ "6800_00A63500", 530, { 835,290, }  },
	{ "6800_088B7200", 733, { 840,1628,1630, }  },
	{ "6180_084ABC00", 3316, { 839,3313, }  },
	{ "6100_40666A00", 329, { 835,230,436, }  },
	{ "6802_08834D00", 52, { 832,309, }  },
	{ "6200_00543300", 2368, { 849,2362, }  },
	{ "6180_08413300", 50, { 830,309,1080, }  },
	{ "6180_104AB800", 1713, { 839,3313, }  },
	{ "6200_00543400", 2372, { 849,2362, }  },
	{ "6100_40665B00", 3497, { 835,230, }  },
	{ "6402_00666F00", 237, { 835,309, }  },
	{ "6802_00926400", 1220, { 847,267,1024,1025, }  },
	{ "6800_108A5900", 3319, { 839,3313,817, }  },
	{ "6802_08861E00", 312, { 835,267, }  },
	{ "6100_00465700", 254, { 835,230, }  },
	{ "6802_00925B00", 1055, { 847,267,1052,1053, }  },
	{ "6800_0092D800", 2140, { 847,2113, }  },
	{ "6800_08836E00", 731, { 832,703, }  },
	{ "6100_00464A00", 329, { 835,230,331, }  },
	{ "6800_088A5B00", 1826, { 839,3313,817, }  },
	{ "6800_10AA6300", 715, { 839,1708, }  },
	{ "6802_08672200", 453, { 836,229, }  },
	{ "6180_08416500", 277, { 830,309, }  },
	{ "6380_40251E00", 450, { 834,67, }  },
	{ "6180_08412900", 50, { 830,704, }  },
	{ "6400_00543C00", 2368, { 849,2361, }  },
	{ "6802_00878700", 279, { 836,229,53,222, }  },
	{ "6802_40926100", 2009, { 847,267,1052,1053, }  },
	{ "6800_0892D600", 2135, { 847,2113, }  },
	{ "6802_4092EB00", 2503, { 847,267,2500,1072, }  },
	{ "6802_00926700", 1223, { 847,267,1024,1025, }  },
	{ "6200_00664F00", 406, { 835,230, }  },
	{ "6802_40922B00", 1031, { 847,267,1024,1067, }  },
	{ "6400_00262200", 563, { 835,290, }  },
	{ "6100_00464D00", 2098, { 835,230,331, }  },
	{ "6802_00839E00", 393, { 832,2497,3197, }  },
	{ "6180_104A9B00", 1713, { 839,1708, }  },
	{ "6800_08811F00", 3145, { 830,394, }  },
	{ "6402_00618D00", 103, { 830,309, }  },
	{ "6A02_40923900", 1031, { 847,267,1024,1192, }  },
	{ "6802_08833A00", 3133, { 832,394, }  },
	{ "6800_088AA500", 3342, { 839,3313, }  },
	{ "6800_08822000", 294, { 831,299, }  },
	{ "6800_08855C00", 733, { 834,62,1201, }  },
	{ "6802_00926200", 3183, { 847,267,1052,1053, }  },
	{ "6310_40652100", 38, { 834,67, }  },
	{ "6802_00926E00", 1045, { 847,267,1024,1043, }  },
	{ "6200_40464100", 327, { 835,230,448, }  },
	{ "6802_00A62B00", 410, { 835,290, }  },
	{ "680E_088E7B00", 1590, { 843,309, }  },
	{ "6802_00875700", 282, { 836,229,53,442, }  },
	{ "6802_4092A000", 2281, { 847,267,1602,2278, }  },
	{ "6800_08821F00", 286, { 831,299, }  },
	{ "6802_0092CB00", 1601, { 847,267,1602,2273, }  },
	{ "6802_00839B00", 393, { 832,2497,3200, }  },
	{ "6100_40263F00", 416, { 835,230, }  },
	{ "6800_108AA400", 3315, { 839,3313, }  },
	{ "6802_00876100", 3485, { 836,229,53,442, }  },
	{ "6200_40652100", 38, { 834,67, }  },
	{ "6200_40451E00", 450, { 834,67, }  },
	{ "6802_00873700", 232, { 836,229,53, }  },
	{ "6802_00927900", 1228, { 847,267,1230, }  },
	{ "6A02_40923A00", 2043, { 847,267,1024,1192, }  },
	{ "6200_40666900", 327, { 835,230,436, }  },
	{ "6200_00464900", 327, { 835,230,331, }  },
	{ "6180_08414B00", 76, { 830,309,888, }  },
	{ "6100_40464200", 329, { 835,230,448, }  },
	{ "6802_00925600", 1055, { 847,267,1052,3543, }  },
	{ "6800_08834300", 3275, { 832,309, }  },
	{ "6200_40464200", 329, { 835,230,448, }  },
	{ "6800_00852A00", 2134, { 834,340, }  },
	{ "6802_00875400", 278, { 836,229,53,442, }  },
	{ "6802_00878D00", 1100, { 836,229,53,222, }  },
	{ "6310_40651E00", 450, { 834,67, }  },
	{ "6802_00879100", 2012, { 836,229,53,222, }  },
	{ "6802_00925E00", 1620, { 847,267,1052,1053, }  },
	{ "6100_40666900", 327, { 835,230,436, }  },
	{ "6180_08414900", 297, { 830,309,888, }  },
	{ "6180_084A9600", 50, { 839,1708,1709, }  },
	{ "6180_084A9700", 1711, { 839,1708,1709, }  },
	{ "6182_084A9E00", 1728, { 839,1708,1709, }  },
	{ "6180_084A9800", 50, { 839,1708,1710, }  },
	{ "6180_084A9900", 1711, { 839,1708,1710, }  },
	{ "6802_00875100", 243, { 836,229,53,442, }  },
	{ "6802_08924000", 1599, { 847,267,1048, }  },
	{ "6802_10841F00", 592, { 833,584, }  },
	{ "6100_00464800", 325, { 835,230,331, }  },
	{ "6100_40666700", 412, { 835,230, }  },
	{ "6200_00543100", 2367, { 849,2362, }  },
	{ "6200_00464A00", 329, { 835,230,331, }  },
	{ "6100_00543E00", 2454, { 849,2362, }  },
	{ "6800_10A23000", 3399, { 831,299, }  },
	{ "6802_0892E600", 2501, { 847,267,2500,1079, }  },
	{ "6800_10821E00", 284, { 831,299, }  },
	{ "6300_40451F00", 441, { 834,67, }  },
	{ "6802_00925F00", 1988, { 847,267,1052,1053, }  },
	{ "6310_40651F00", 441, { 834,67, }  },
	{ "6200_40666700", 412, { 835,230, }  },
	{ "6802_00879A00", 1336, { 836,229,53,28,1332, }  },
	{ "6802_08927F00", 1263, { 847,267,1230, }  },
	{ "6802_08837300", 63, { 832,309, }  },
	{ "6802_00B12A00", 880, { 846,299,901, }  },
	{ "6800_108A5500", 715, { 839,3313, }  },
	{ "6200_0046C200", 2369, { 835,2360, }  },
	{ "6802_00873800", 236, { 836,229,53, }  },
	{ "6802_00912200", 253, { 846,299,900, }  },
	{ "6802_0892E300", 1974, { 847,267,1602,2139, }  },
	{ "6802_40866000", 1226, { 835,267,1080, }  },
	{ "6802_08871F00", 54, { 836,229, }  },
	{ "6800_08838B00", 730, { 832,703, }  },
	{ "6200_00464B00", 1562, { 835,230,331, }  },
	{ "6100_00664F00", 406, { 835,230, }  },
	{ "680E_08821F00", 286, { 831,299, }  },
	{ "6802_0092E100", 1607, { 847,267,1602,2139, }  },
	{ "6322_00854D00", 447, { 834,62,57, }  },
	{ "6802_00924300", 1050, { 847,267,1048,1049, }  },
	{ "6100_40665F00", 413, { 835,230, }  },
	{ "6300_40452100", 38, { 834,67, }  },
	{ "6200_00465000", 325, { 835,230,20, }  },
	{ "6802_08922100", 1066, { 847,267, }  },
	{ "6800_10A22F00", 1717, { 831,299, }  },
	{ "6400_00462500", 417, { 835,290,230, }  },
	{ "6800_10AA6200", 717, { 839,1708, }  },
	{ "6802_08927200", 1046, { 847,267,1024,1043, }  },
	{ "6802_00872A00", 273, { 836,229,53, }  },
	{ "6800_088A4C00", 733, { 839,3313, }  },
	{ "6800_00822500", 1761, { 831,299, }  },
	{ "6100_40463600", 1408, { 835,290,230, }  },
	{ "6102_40254E00", 217, { 834,268, }  },
	{ "6180_08413200", 50, { 830,309,1087, }  },
	{ "6802_00927000", 1225, { 847,267,1024,1043, }  },
	{ "6800_10AA6100", 716, { 839,1708, }  },
	{ "6802_40925500", 1054, { 847,267,1052,3543, }  },
	{ "6802_4092A400", 2281, { 847,267,1602,2280, }  },
	{ "6802_0092E800", 2513, { 847,267,2500,1079, }  },
	{ "6A02_0892B100", 2017, { 847,267, }  },
	{ "6100_40465300", 325, { 835,230,20, }  },
	{ "6802_00865F00", 1205, { 835,267,1080, }  },
	{ "6100_0046C200", 2369, { 835,2360, }  },
	{ "6802_00925C00", 1056, { 847,267,1052,1053, }  },
	{ "6802_00878300", 243, { 836,229,53,222, }  },
	{ "6802_00922200", 1029, { 847,267, }  },
	{ "6802_00927300", 1989, { 847,267,1024,1043, }  },
	{ "6800_08838D00", 732, { 832,703, }  },
	{ "6800_088B7000", 733, { 840,1628,1629, }  },
	{ "6802_00921E00", 1055, { 847,267, }  },
	{ "6802_00912A00", 253, { 846,299,901, }  },
	{ "6802_0892EF00", 2505, { 847,267,2500,1075, }  },
	{ "6200_40465300", 325, { 835,230,20, }  },
	{ "6100_004AB600", 3314, { 839,3313, }  },
	{ "6100_00665900", 1401, { 835,230, }  },
	{ "6400_00618C00", 105, { 830,309, }  },
	{ "6400_00543D00", 2454, { 849,2361, }  },
	{ "6802_00878600", 278, { 836,229,53,222, }  },
	{ "6802_40926000", 2008, { 847,267,1052,1053, }  },
	{ "6802_0092ED00", 2502, { 847,267,2500,1075, }  },
	{ "6802_00926600", 1222, { 847,267,1024,1025, }  },
	{ "6200_40463700", 890, { 835,290,230, }  },
	{ "6802_0892EC00", 2501, { 847,267,2500,1075, }  },
	{ "6180_084B1E00", 50, { 840,1716, }  },
	{ "6802_00839F00", 393, { 832,2497,3328, }  },
	{ "6A12_00923500", 1039, { 847,267,1024,1194, }  },
	{ "6100_00464C00", 2097, { 835,230,331, }  },
	{ "6180_104A9C00", 1714, { 839,1708, }  },
	{ "6800_08831E00", 310, { 832,309, }  },
	{ "6802_0892E900", 2501, { 847,267,2500,1072, }  },
	{ "6380_40452100", 38, { 834,67, }  },
	{ "6400_00543A00", 2367, { 849,2361, }  },
	{ "6802_4092A200", 2281, { 847,267,1602,2279, }  },
	{ "6200_00543E00", 2454, { 849,2362, }  },
	{ "6802_00927C00", 1217, { 847,267,1230, }  },
	{ "6800_088A8A00", 1718, { 839,1708, }  },
	{ "6800_08822100", 573, { 831,299, }  },
	{ "6180_104AB700", 1712, { 839,3313, }  },
	{ "6802_0092EA00", 2502, { 847,267,2500,1072, }  },
	{ "6800_008B7300", 330, { 840,1628,1630, }  },
	{ "6A02_40924A00", 1050, { 847,267,1048,1193, }  },
	{ "6802_00878900", 282, { 836,229,53,222, }  },
	{ "6802_0892CC00", 2274, { 847,267,1602,2273, }  },
	{ "6802_0092DF00", 1605, { 847,267,1602,2139, }  },
	{ "6802_00A62A00", 408, { 835,290, }  },
	{ "6100_40666200", 329, { 835,230,437, }  },
	{ "6A02_0892B000", 1690, { 847,267, }  },
	{ "6800_10AA6400", 714, { 839,1708, }  },
	{ "6380_40451F00", 441, { 834,67, }  },
	{ "6802_4092A100", 2282, { 847,267,1602,2278, }  },
	{ "6802_0092DE00", 1604, { 847,267,1602,2139, }  },
	{ "6802_00878200", 242, { 836,229,53,222, }  },
	{ "6800_10838F00", 1626, { 832,703, }  },
	{ "6802_00879200", 2013, { 836,229,53,222, }  },
	{ "6802_00839C00", 393, { 832,2497,2498, }  },
	{ "6180_00522900", 2511, { 847,309,2508, }  },
	{ "6200_40666000", 325, { 835,230,437, }  },
	{ "6802_08926800", 1328, { 847,267,1024,1025, }  },
	{ "6800_00922500", 315, { 847,2113,3360, }  },
	{ "6802_00B12900", 372, { 846,299,901, }  },
	{ "6100_00543400", 2372, { 849,2362, }  },
	{ "6180_00522700", 2509, { 847,309,2508, }  },
	{ "6800_088A4D00", 3318, { 839,3313, }  },
	{ "6800_40A63A00", 2474, { 835,290,230, }  },
	{ "6100_00418000", 153, { 830,309,888, }  },
	{ "6802_00875500", 279, { 836,229,53,442, }  },
	{ "6100_40464100", 327, { 835,230,448, }  },
	{ "6802_00B12100", 372, { 846,299,900, }  },
	{ "6802_00878C00", 1099, { 836,229,53,222, }  },
	{ "6802_00912900", 393, { 846,299,901, }  },
	{ 0 }
};
typedef struct _sma_object sma_object_t;

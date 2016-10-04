unsigned char CTPM_FW[] = {
	#ifdef CONFIG_TOUCHSCREEN_ILITEK_2120_X50L
	#include "ILI2120_V132_20160530_X50L.ili"
	#else
	#include "ILI2120_V132_20160608_X55.ili"
	#endif
};
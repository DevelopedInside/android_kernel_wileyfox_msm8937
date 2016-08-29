#include "ilitek_ts.h"
#include <linux/firmware.h>

#ifdef ILI_UPDATE_FW
bool driver_upgrade_flag = false;
#if !IC2120
//static int chip_id_H = 0, chip_id_L = 0, ice_flag = 0, ice_checksum = 0;
static int iceRomSize;
static int iceLockPage;
static int icestartsector;
static int iceendsector;
static int ifbiceRomStart;
static int ifbiceRomEnd;
static int ifbicestartsector;
static int ifbiceendsector;
static unsigned char chip_id_H = 0x0, chip_id_L = 0x0;
static int ic_model;

#endif
extern struct i2c_data i2c;
#include "ilitek_fw.h"
#include <asm/delay.h>
static int inwrite(unsigned int address)
{
	uint8_t outbuff[64];
	int data, ret;
	outbuff[0] = 0x25;
	outbuff[1] = (char)((address & 0x000000FF) >> 0);
	outbuff[2] = (char)((address & 0x0000FF00) >> 8);
	outbuff[3] = (char)((address & 0x00FF0000) >> 16);
	ret = ilitek_i2c_write(i2c.client, outbuff, 4);
	//udelay(10);
	mdelay(1);
	ret = ilitek_i2c_read(i2c.client, outbuff, 4);
	data = (outbuff[0] + outbuff[1] * 256 + outbuff[2] * 256 * 256 + outbuff[3] * 256 * 256 * 256);
	//printk("%s, data=0x%x, outbuff[0]=%x, outbuff[1]=%x, outbuff[2]=%x, outbuff[3]=%x\n", __func__, data, outbuff[0], outbuff[1], outbuff[2], outbuff[3]);
	return data;
}

static int outwrite(unsigned int address, unsigned int data, int size)
{
	int ret, i;
	char outbuff[64];
	outbuff[0] = 0x25;
	outbuff[1] = (char)((address & 0x000000FF) >> 0);
	outbuff[2] = (char)((address & 0x0000FF00) >> 8);
	outbuff[3] = (char)((address & 0x00FF0000) >> 16);
	for(i = 0; i < size; i++)
	{
		outbuff[i + 4] = (char)(data >> (8 * i));
	}
	ret = ilitek_i2c_write(i2c.client, outbuff, size + 4);
	return ret;
}


#if !IC2120



#if 1
static void set_program_key(void) {
	int ret;
	ret = outwrite(0x41014, 0x7EAE, 2);
	printk("%s, ilitek_write, ret = %d\n", __func__, ret);
}

static void clear_program_key(void) {
	int ret;
	ret = outwrite(0x41014, 0x0, 2);
	printk("%s, ilitek_write, ret = %d\n", __func__, ret);
}

static void set_standby_key(unsigned int chip_id_h, unsigned int chip_id_l) {
	int ret;
	printk("%s, chip id: 0x%x%x\n", __func__, chip_id_h, chip_id_l);
	ret = outwrite(0x40010, (chip_id_h << 8) + chip_id_l, 2);
	printk("%s, ilitek_write, ret = %d\n", __func__, ret);
}

static void mtp_control_reg_2210new(int delay, unsigned int address, int retry) {
	char buf[64];
	int i, temp = 0;
	buf[0] = 0x0;
	for(i = 0; i < retry ; i++) {
		temp = inwrite(address);
		printk("%s, temp = %d, i=%d, 0x%X\n", __func__, temp, i, temp);
		if((temp & 0x00000001) == 0x00000001) {
			break;
		}
		mdelay(delay);
	}
	if(i != 0) {
		printk("%s, delay time = %dus\n", __func__, i * delay);
	}
	if(i == retry) {
		printk("%s, MTP is busy\n", __func__);
	}
}

static void set_standby_2210new(unsigned int address, int retry) {
	int ret;
	ret = outwrite(0x40024, 0x01, 1);
	printk("%s, ilitek_write, ret = %d\n", __func__, ret);
	mtp_control_reg_2210new(100, address, retry);
	ret = outwrite(0x41030, 00, 4);
}

static int set_sector_erase(unsigned int usSector, unsigned int chip_id_h, unsigned int chip_id_l) {
	int ret;
	char buf[64];
	printk("%s, chip id: 0x%x%x\n", __func__, chip_id_h, chip_id_l);

	//a.Setting sector erase key (0x41012)
	ret = outwrite(0x41012, 0xA512, 2);
	printk("%s, a.Setting sector erase key (0x41012), ilitek_write, ret = %d\n", __func__, ret);

	//b.Setting standby key (0x40010)
	set_standby_key(chip_id_h, chip_id_l);
	printk("%s, b.Setting standby key (0x40010)\n", __func__);

	//c.Setting sector number (0x41018)
	buf[0] = 0X25;
	buf[1] = 0X18;
	buf[2] = 0X10;
	buf[3] = 0X04;
	buf[4] = (char)((usSector & 0x00FF) >> 0);
	buf[5] = (char)((usSector & 0xFF00) >> 8);
	buf[6] = 0x00;
	buf[7] = 0x00;
	ret = ilitek_i2c_write(i2c.client, buf, 8);
	printk("%s, c.Setting sector number (0x41018), ilitek_write, ret = %d\n", __func__, ret);

	//d.Enable sector erase (0x41031)
	ret = outwrite(0x41031, 0xA5, 1);
	printk("%s, d.Enable sector erase (0x41031), ilitek_write, ret = %d\n", __func__, ret);

    //e.Enable standby (0x40024)
	//f.Wait chip erase (94ms) or check MTP_busy (0x4102c)
	//set_standby();
	set_standby_2210new(0x041031, 20);
	printk("%s, e.Enable standby\n", __func__);
	return 0;
}
	
static int set_chip_erase(unsigned short usLCKN, unsigned int chip_id_h, unsigned int chip_id_l) {
	int ret;
	unsigned char ucCount;
	printk("%s, chip id: 0x%x%x\n", __func__, chip_id_h, chip_id_l);

	//a.Setting lock page (0x41024)
	for(ucCount = 0; ucCount < 3; ucCount++) {
		ret = outwrite(0x41024, usLCKN, 2);
		printk("%s, a.Setting lock page (0x41024), ilitek_write, ret = %d ucCount = %d\n", __func__, ret, ucCount);

		if((inwrite(0x41024) | 0x00FF) == usLCKN) {
			break;
		}
	}
	if((inwrite(0x41024) | 0x00FF) != usLCKN) {
		return -1;
	}

	//b.Setting chip erase key (0x41010)
	ret = outwrite(0x41010, 0xCEAE, 4);
	printk("%s, b.Setting chip erase key (0x41010), ilitek_write, ret = %d\n", __func__, ret);

	//c.Setting standby key (0x40010)
	set_standby_key(chip_id_h, chip_id_l);
	printk("%s, c.Setting standby key (0x40010)\n", __func__);

	//d.Enable chip erase (0x41030)
	ret = outwrite(0x41030, 0x7E, 1);
	printk("%s, d.Enable chip erase (0x41030), ilitek_write, ret = %d\n", __func__, ret);

	//e.Enable standby (0x40024)
	//f.Wait chip erase (94ms) or check MTP_busy (0x4102c)
	//set_standby();
	set_standby_2210new(0x041030, 3);
	printk("%s, e.Enable standby\n", __func__);

	//g.Clear lock page (0x41024)
	ret = outwrite(0x41024, 0xFFFF, 2);
	printk("%s, g.Clear lock page (0x41024), ilitek_write, ret = %d\n", __func__, ret);

	return 0;
}
	
static void set_preprogram(unsigned int usStart, unsigned int usEnd, unsigned int chip_id_h, unsigned int chip_id_l) {

	int ret;
	unsigned int usSize;
	printk("%s, chip id: 0x%x%x\n", __func__, chip_id_h, chip_id_l);
	//a.Setting program key
	set_program_key();
	printk("%s, a.Setting program key\n", __func__);

	//b.Setting standby key
	set_standby_key(chip_id_h, chip_id_l);
	printk("%s, b.Setting standby key (0x40010)\n", __func__);
	
	//c.Setting program start address and size (0x41018)
	usStart = usStart / 16;
	usEnd = usEnd / 16;
	usSize = usEnd - usStart;
	ret = outwrite(0x41018, usStart + ((usSize - 1) << 16), 4);
	printk("%s, c.Setting program start address and size (0x41018), ilitek_write, ret = %d\n", __func__, ret);

	//d.Setting pre-program data (0x4101c)
	ret = outwrite(0x4101C, 0xFFFFFFFF, 4);
	printk("%s, d.Setting pre-program data (0x4101c), ilitek_write, ret = %d\n", __func__, ret);

	//e.Enable pre-program (0x41032)
	ret = outwrite(0x41032, 0xB6, 1);
	printk("%s, e.Enable pre-program (0x41032), ilitek_write, ret = %d\n", __func__, ret);

	//f.Enable standby (0x40024)
	//g.Wait program time (260us*size) or check MTP_busy (0x4102c)
	//set_standby();
	set_standby_2210new(0x041032, 20);
	printk("%s, f.Enable standby\n", __func__);

	//h.Clear program key (0x41014)
	clear_program_key();
	printk("%s, h.Clear program key (0x41014)\n", __func__);
}

static void clear_standby_key(void) {
	int ret;
	ret = outwrite(0x40010, 0x0000, 2);
	printk("%s, ilitek_write, ret = %d\n", __func__, ret);
}

static int Set2ndHighVoltage_2210(unsigned int chip_id_h, unsigned int chip_id_l) {
	outwrite(0x04100E, 0x00000000, 1);
	//outwrite(0x040010, ic_model, 2);
	outwrite(0x040010, (chip_id_h << 8) + chip_id_l, 2);
	outwrite(0x040024, 0x00000001, 1);
	mdelay(10);
	return 0;
}

static int SetDefaultVoltage_2210(unsigned int chip_id_h, unsigned int chip_id_l) {
	outwrite(0x04100E, 0x00000002, 1);
	//outwrite(0x040010, ic_model, 2);
	outwrite(0x040010, (chip_id_h << 8) + chip_id_l, 2);
	outwrite(0x040024, 0x00000001, 1);
	mdelay(10);
	return 0;
}

static void set_48MHz(unsigned int chip_id_h, unsigned int chip_id_l) {

	int ret;
	char buf[64];
	unsigned int uiData;
	printk("%s, chip id: 0x%x%x\n", __func__, chip_id_h, chip_id_l);
	//a.Setting OSC key
	ret = outwrite(0x42000, 0x27,1);
	printk("%s, a.Setting OSC key, ilitek_write, ret = %d\n", __func__, ret);

	//b.Setting standby key 
	set_standby_key(chip_id_h, chip_id_l);
	printk("%s, b.Setting standby key (0x40010)\n", __func__);

	//c.Disable OSC48DIV2
	if(chip_id_h == 0x21 && (chip_id_l == 0x15 ||chip_id_l == 0x16)){
		uiData = inwrite(0x42008);
		buf[1] = 0x08;
		printk("uiData = 0x%x\n", uiData);
		buf[0] = 0x25;
		buf[2] = 0x20;
		buf[3] = 0x04;
		buf[4] = ((unsigned char)((uiData & 0x000000FF) >> 0));
		buf[5] = ((unsigned char)((uiData & 0x0000FF00) >> 8));
		buf[6] = ((unsigned char)((uiData & 0x00FF0000) >> 16));
		buf[7] = ((unsigned char)((uiData & 0xFF000000) >> 24) | 4);
		ret = ilitek_i2c_write(i2c.client, buf, 8);
	}
	if(chip_id_h == 0x22 && chip_id_l == 0x10){
		uiData = inwrite(0x4200b);
		buf[1] = 0x0b;
		printk("uiData = 0x%x\n", uiData);
		buf[0] = 0x25;
		buf[2] = 0x20;
		buf[3] = 0x04;
		buf[4] = ((unsigned char)((uiData & 0x000000FF) >> 0) | 4);
		buf[5] = ((unsigned char)((uiData & 0x0000FF00) >> 8));
		buf[6] = ((unsigned char)((uiData & 0x00FF0000) >> 16));
		buf[7] = ((unsigned char)((uiData & 0xFF000000) >> 24));
		ret = ilitek_i2c_write(i2c.client, buf, 8);
	}
	printk("%s, c.Disable OSC48DIV2, ilitek_write, ret = %d\n", __func__, ret);

	//d.Enable standby
	ret = outwrite(0x40024, 0x01, 1);
	printk("%s, d.Enable standby, ilitek_write, ret = %d\n", __func__, ret);

	//e.Clear OSC key
	ret = outwrite(0x42000, 0x0, 1);
	printk("%s, e.Clear OSC key, ilitek_write, ret = %d\n", __func__, ret);

	//f.Clear standby key
	clear_standby_key();
	printk("%s, f.Clear standby key\n", __func__);
}

static void reset_register(int data) {
	int i;

	//RESET CPU
	//1.Software Reset
	//1-1 Clear Reset Done Flag
	outwrite(0x40048, 0x00000000,4);
	//1-2 Set CDC/Core Reset
	outwrite(0x40040, data,4);
	for(i = 0; i < 5 ;i++) {
		//printk("333 udelay(1000) %d \n", i);
		mdelay(1);
		if((inwrite(0x040048)&0x00010000) == 0x00010000)
			break;
		//break;
	}
	mdelay(100);
	outwrite(0x40040, 0x00000000,4);
}

static void exit_ice_mode(void) {
	unsigned char buf[16];
	int ret;

	reset_register(0x033E00AE);
	//Exit ICE
	buf[0] = 0x1B;
	buf[1] = 0x62;
	buf[2] = 0x10;
	buf[3] = 0x18;
	ret = ilitek_i2c_write(i2c.client, buf, 4);
	printk("%s, ilitek_write, ret = %d\n", __func__, ret);
	mdelay(1000);
}

static int getchecksum(int start_addr, int end_addr, int check) {
	int ret = 0, ice_checksum = 0;
	printk("%s, after write, chip id: 0x%x%x\n", __func__, chip_id_H, chip_id_L);
	set_standby_key(chip_id_H, chip_id_L);
	
	ret = outwrite(0x41020, start_addr + (end_addr << 16), 4);
	printk("%s, ilitek_write, ret = %d", __func__, ret);
	
	ret = outwrite(0x41038, 0x01, 1);
	printk("%s, ilitek_write, ret = %d", __func__, ret);
	
	ret = outwrite(0x41033, 0xCD, 1);
	printk("%s, ilitek_write, ret = %d", __func__, ret);
	
	//set_standby();
	set_standby_2210new(0x041033, 20);
	
	ret = outwrite(0x41030, 0x0, 4);
#if 1
	if (check) {
		ice_checksum = inwrite(0x41028);
		if(check != ice_checksum) {
			printk("checksum error, hex checksum = 0x%6x, ic checksum = 0x%6x\n", (int)check, (int)ice_checksum);
			ret = -1;
		} else {
			printk("checksum equal, hex checksum = 0x%6x, ic checksum = 0x%6x\n", (int)check, (int)ice_checksum);
			ret = 0;
		}
	}
	else {
		ret = 0;
	}
	return ret;
#endif
}

static int erase_data(int df_ap) {
	int i = 0;
	printk("erase_data df_ap = %d\n", df_ap);
	if (df_ap == 0) {
		if ((ic_model & 0xFF00) != 0x2100) {
			Set2ndHighVoltage_2210(chip_id_H, chip_id_L);
		}
		printk("%s, set_preprogram, chip id: 0x%x%x\n", __func__, chip_id_H, chip_id_L);
		set_preprogram(0x0000, iceRomSize, chip_id_H, chip_id_L);
		if ((ic_model & 0xFF00) != 0x2100) {
			SetDefaultVoltage_2210(chip_id_H, chip_id_L);
		}
		printk("%s, set_chip_erase, chip id: 0x%x%x\n", __func__, chip_id_H, chip_id_L);
		set_chip_erase(iceLockPage, chip_id_H, chip_id_L);
		
		printk("%s, set_sector_erase, chip id: 0x%x%x\n", __func__, chip_id_H, chip_id_L);					
		for(i = icestartsector; i <= iceendsector; i++) {
			set_sector_erase(i, chip_id_H, chip_id_L);
		}
	}
	else {
		if ((ic_model & 0xFF00) != 0x2100) {
			Set2ndHighVoltage_2210(chip_id_H, chip_id_L);
		}
		
		printk("%s, set_preprogram, chip id: 0x%x%x\n", __func__, chip_id_H, chip_id_L);
		set_preprogram(ifbiceRomStart, ifbiceRomEnd, chip_id_H, chip_id_L);
		if ((ic_model & 0xFF00) != 0x2100) {
			SetDefaultVoltage_2210(chip_id_H, chip_id_L);
		}
#if 0
		printk("%s, set_chip_erase, chip id: 0x%x%x\n", __func__, chip_id_H, chip_id_L);
		set_chip_erase(jni.fm.iceLockPage, chip_id_H, chip_id_L);
		if(jni.fm.upgrade_status < 0) {
			printk("%s, after set_chip_erase, chip id: 0x%x%x\n", __func__, chip_id_H, chip_id_L);
			return jni.fm.upgrade_status;
		}
#endif
		printk("%s, set_sector_erase, chip id: 0x%x%x\n", __func__, chip_id_H, chip_id_L);					
		for(i = ifbicestartsector; i <= ifbiceendsector; i++) {
			set_sector_erase(i, chip_id_H, chip_id_L);
		}
	}
	return 0;
}
#endif

#endif

/*
   description
   upgrade firmware
   prarmeters

   return
   status
*/
int ilitek_upgrade_firmware(void)
{
	int ret = 0, upgrade_status = 0, i = 0, j = 0, k = 0, ap_len = 0, df_len = 0;
	unsigned char buf[64] = {0};
	//unsigned char flash_buf[64] = {0};
	unsigned long ap_startaddr = 0, df_startaddr = 0, ap_endaddr = 0, df_endaddr = 0, ap_checksum = 0, df_checksum = 0, temp = 0;
	unsigned char firmware_ver[8];
	//const struct firmware *fw;
	#if IC2120
	int retry = 0;
	#endif
	#if !IC2120
	int ice_flag = 0, ice_checksum = 0;
	//unsigned char chip_id_H = 0, chip_id_L = 0;
	int bl_ver0 = 0, bl_ver1 = 0, page_number = 8;
	//int bl_ver = 0, flow_flag = 0;
	int fast_update_flag = 0;
	int tmp_ap_end_addr = 0;
	bool has_df = false;
	bool handle_df = false;
	#endif
	ap_startaddr = ( CTPM_FW[0] << 16 ) + ( CTPM_FW[1] << 8 ) + CTPM_FW[2];
	ap_endaddr = ( CTPM_FW[3] << 16 ) + ( CTPM_FW[4] << 8 ) + CTPM_FW[5];
	ap_checksum = ( CTPM_FW[6] << 16 ) + ( CTPM_FW[7] << 8 ) + CTPM_FW[8];
	df_startaddr = ( CTPM_FW[9] << 16 ) + ( CTPM_FW[10] << 8 ) + CTPM_FW[11];
	df_endaddr = ( CTPM_FW[12] << 16 ) + ( CTPM_FW[13] << 8 ) + CTPM_FW[14];
	df_checksum = ( CTPM_FW[15] << 16 ) + ( CTPM_FW[16] << 8 ) + CTPM_FW[17];
	firmware_ver[0] = CTPM_FW[18];
	firmware_ver[1] = CTPM_FW[19];
	firmware_ver[2] = CTPM_FW[20];
	firmware_ver[3] = CTPM_FW[21];
	firmware_ver[4] = CTPM_FW[22];
	firmware_ver[5] = CTPM_FW[23];
	firmware_ver[6] = CTPM_FW[24];
	firmware_ver[7] = CTPM_FW[25];
	df_len = ( CTPM_FW[26] << 16 ) + ( CTPM_FW[27] << 8 ) + CTPM_FW[28];
	ap_len = ( CTPM_FW[29] << 16 ) + ( CTPM_FW[30] << 8 ) + CTPM_FW[31];
	printk("ilitek ap_startaddr=0x%lX, ap_endaddr=0x%lX, ap_checksum=0x%lX, ap_len = 0x%d\n", ap_startaddr, ap_endaddr, ap_checksum, ap_len);
	printk("ilitek df_startaddr=0x%lX, df_endaddr=0x%lX, df_checksum=0x%lX, df_len = 0x%d\n", df_startaddr, df_endaddr, df_checksum, df_len);

#if 1
	if (!driver_upgrade_flag) {
		for(i = 0; i < 4; i++)
		{
			printk("i2c.firmware_ver[%d] = %d, firmware_ver[%d] = %d\n", i, i2c.firmware_ver[i], i, CTPM_FW[i + 18]);
			if((i2c.firmware_ver[i] > CTPM_FW[i + 18]) || ((i == 3) && (i2c.firmware_ver[3] == CTPM_FW[3 + 18])))
			{
				ret = 1;
				printk("ilitek_upgrade_firmware Do not need update\n"); 
				return 1;
				//break;
			}
			else if(i2c.firmware_ver[i] < CTPM_FW[i + 18])
			{
				break;
			}
		}
	}
#endif
	#if IC2120
	Retry:
	ret = outwrite(0x181062, 0x0, 0);
	//printk("%s, release Power Down Release mode\n", __func__);
	msleep(1);
	ret = outwrite(0x5200c, 0x0, 2);
	msleep(1);
	ret = outwrite(0x52020, 0x1, 1);
	msleep(1);
	ret = outwrite(0x52020, 0x0, 1);
	msleep(1);
	ret = outwrite(0x42000, 0x0f154900, 4);
	msleep(1);
	ret = outwrite(0x42014, 0x2, 1);
	msleep(1);
	ret = outwrite(0x42000, 0x00000000, 4);
	msleep(1);
	ret = outwrite(0x041000, 0xab, 1);
	msleep(1);
	ret = outwrite(0x041004, 0x66aa5500, 4);
	msleep(1);
	ret = outwrite(0x04100d, 0x00, 1);
	msleep(5);
	for (i = 0x0; i <= 0xD000;i += 0x1000) {
		//printk("%s, i = %X\n", __func__, i);
		ret = outwrite(0x041000, 0x06, 1);
		msleep(1);
		ret = outwrite(0x041004, 0x66aa5500, 4);
		msleep(1);
		temp = (i << 8) + 0x20;
		ret = outwrite(0x041000, temp, 4);
		msleep(1);
		ret = outwrite(0x041004, 0x66aa5500, 4);
		msleep(15);
		for (j = 0; j < 50; j++) {
			ret = outwrite(0x041000, 0x05, 1);
			ret = outwrite(0x041004, 0x66aa5500, 4);
			msleep(1);
			buf[0] = inwrite(0x041013);
			//printk("%s, buf[0] = %X\n", __func__, buf[0]);
			if (buf[0] == 0) {
				break;
			}
			else {

				msleep(2);
			};
		}
	}
	msleep(100);
	for(i = ap_startaddr; i < ap_endaddr; i += 0x20) {
		//printk("%s, i = %X\n", __func__, i);
		ret = outwrite(0x041000, 0x06, 1);
		//msleep(1);
		ret = outwrite(0x041004, 0x66aa5500, 4);
		//msleep(1);
		temp = (i << 8) + 0x02;
		ret = outwrite(0x041000, temp, 4);
		//msleep(1);
		ret = outwrite(0x041004, 0x66aa551f, 4);
		//msleep(1);
		buf[0] = 0x25;
		buf[3] = (char)((0x041020  & 0x00FF0000) >> 16);
		buf[2] = (char)((0x041020  & 0x0000FF00) >> 8);
		buf[1] = (char)((0x041020  & 0x000000FF));
		for(k = 0; k < 32; k++)
		{
			buf[4 + k] = CTPM_FW[i + 32 + k];
		}


		if(ilitek_i2c_write(i2c.client, buf, 36) < 0) {
			printk("%s, write data error, address = 0x%X, start_addr = 0x%X, end_addr = 0x%X\n", __func__, (int)i, (int)ap_startaddr, (int)ap_endaddr);
			return 3;
		}

		upgrade_status = (i * 100) / ap_len;
		//printk("%cILITEK: Firmware Upgrade(Data flash), %02d%c.",0x0D,upgrade_status,'%');
		//printk("ILITEK: Firmware Upgrade(Data flash), %02d%c. ", upgrade_status, '%');
		mdelay(3);
	}
	printk("\n");
	buf[0] = 0x1b;
	buf[1] = 0x62;
	buf[2] = 0x10;
	buf[3] = 0x18;
	ilitek_i2c_write(i2c.client, buf, 4);
	ilitek_reset(i2c.reset_gpio);
	gpio_direction_output(i2c.reset_gpio,1);
	//msleep(50);
	mdelay(10);
	gpio_direction_output(i2c.reset_gpio,0);
	//msleep(50);
	mdelay(10);
	gpio_direction_output(i2c.reset_gpio,1);
	//msleep(100);
	//mdelay(100);

	mdelay(10);
	for (i =0; i < 30; i++ ) {
		ret = ilitek_poll_int();
		printk("ilitek int status = %d\n", ret);
		if (ret == 0) {
			break;
		}
		else {
			mdelay(5);
		}
	}
	if (i >= 30) {
		printk("ilitek reset but int not pull low upgrade retry\n");
		if (retry < 2) {
			retry++;
			goto Retry;
		}
	}
	else {
		printk("upgrade ILITEK_IOCTL_I2C_RESET end int pull low\n");
		buf[0] = 0x10;
		ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 3);
		printk("%s, buf = %X, %X, %X\n", __func__, buf[0], buf[1], buf[2]);
		if (buf[1] >= 0x80) {
			printk("upgrade ok ok \n");
		}else {
			printk("ilitek reset int pull low 0x10 cmd read error  upgrade retry\n");
			if (retry < 2) {
				retry++;
				goto Retry;
			}
		}
	}
	#else
#if 1
	ilitek_reset(i2c.reset_gpio);
	//set into test mode
	buf[0] = 0xF2;
	buf[1] = 0x01;
	//msgs[0].len = 2;
	//ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	ret = ilitek_i2c_write(i2c.client, buf, 2);
	msleep(100);
	
	//check ic type
	buf[0] = ILITEK_TP_CMD_GET_KERNEL_VERSION;
	ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 5);
	//ilitek_i2c_write_and_read(i2c.client, 0x61, buf, 5);
	printk("buf[0] = 0x%X\n", buf[0]);
	if(buf[0] == 0x07 && buf[1] == 0x00)
	{
		ice_flag = 1;
		chip_id_H = 0x21;
		chip_id_L = 0x15;
		ic_model = (chip_id_H << 8) + chip_id_L;
		printk("IC is 0x%4X\n", ic_model);
	}
	if(buf[0] == 0x08 && buf[1] == 0x00)
	{
		ice_flag = 1;
		chip_id_H = 0x21;
		chip_id_L = 0x16;
		ic_model = (chip_id_H << 8) + chip_id_L;
		printk("IC is 0x%4X\n", ic_model);
	}
	if(buf[0] == 0x0A && buf[1] == 0x00) {
		ice_flag = 1;
		chip_id_H = 0x21;
		chip_id_L = 0x17;
		ic_model = (chip_id_H << 8) + chip_id_L;
		printk("%s, ic is 0x%4X\n", __func__, ic_model);
	}
	//printk("%s, 2211 buf[0] == 0x10 && buf[1] == 0x22 \n", __func__);
	if(buf[0] == 0x0B && buf[1] == 0x00) {
		ice_flag = 1;
		chip_id_H = 0x22;
		chip_id_L = 0x10;
		ic_model = (chip_id_H << 8) + chip_id_L;
		printk("%s, ic is 0x%4X\n", __func__, ic_model);
	}
	if(buf[0] == 0x10 && buf[1] == 0x22) {
		ice_flag = 1;
		chip_id_H = 0x22;
		chip_id_L = 0x10;
		ic_model = (chip_id_H << 8) + chip_id_L;
		printk("%s, ic is 0x%4X\n", __func__, ic_model);
	}
	if(buf[0] == 0x11 && buf[1] == 0x22) {
		ice_flag = 1;
		chip_id_H = 0x22;
		chip_id_L = 0x11;
		ic_model = (chip_id_H << 8) + chip_id_L;
		printk("%s, ic is 0x%4X\n", __func__, ic_model);
	}
	if(buf[0] == 0x17 && buf[1] == 0x21) {
		ice_flag = 1;
		chip_id_H = 0x21;
		chip_id_L = 0x17;
		ic_model = (chip_id_H << 8) + chip_id_L;
		printk("%s, ic is 0x%4X\n", __func__, ic_model);
	}
			
	if((buf[0] == 0xFF && buf[1] == 0xFF) || (buf[0] == 0x00 && buf[1] == 0x00))
	{
		ice_flag = 1;
		printk("ilitek IC is NULL\n");
	}
	
	if ((buf[0] == 0x03 || buf[0] == 0x09 || buf[1] == 0x23)) {
		handle_df = true;
		printk("ilitek handle_df = %d\n", handle_df);
	}
	if (df_endaddr > df_startaddr) {
		has_df = true;
		printk("%s, has_df = %d\n", __func__, has_df);
	}
	if(ice_flag)
	{
		ic_model = 0;
		printk("%s, entry ice mode start\n", __func__);
		ret = outwrite(0x181062, 0x0, 0);
		printk("%s, outwrite, ret = %d\n", __func__, ret);
		ic_model = inwrite(0x4009c) >> 16;
		if (ic_model == 0x2210) {
			chip_id_H = ic_model >> 8;
			chip_id_L = ic_model;
			iceRomSize = 0xBF00;
			iceLockPage = 0x07FF;
			icestartsector = 0xB0;	
			iceendsector = 0xBC;
			outwrite(0x41039, 0x0, 1);
			
			temp = inwrite(0x00BF30) >> 16 & 0xFF;
			if (temp != 0xCA && temp != 0xCB) {
				printk("2210 but not CA and CB return\n");
				return ret;
			}
			outwrite(0x041004, 0x0000095F, 2);
			outwrite(0x041006, 0x0057E3FF, 3);
		}
		else {
			//Read PID
			buf[0] = inwrite(0x0004009b);
			printk("%s, chip id:%x\n", __func__, buf[0]);
	#if 1	
			outwrite(0x41039, 0x0, 1);
	#endif
			//check 2115 or 2116 again
			if(buf[0] == 0x01 || buf[0] == 0x02 || buf[0] == 0x03 || buf[0] == 0x04 || buf[0] == 0x11 || buf[0] == 0x12 || buf[0] == 0x13 || buf[0] == 0x14) {
				printk("%s, ic is 2115\n", __func__);
				chip_id_H = 0x21;
				chip_id_L = 0x15;
				ic_model = (chip_id_H << 8) + chip_id_L;
				printk("IC Type: 2115----------------------\n");
				iceRomSize = 0x7F00;
				iceLockPage = 0x7FFF;
				icestartsector = 0x78;	
				iceendsector = 0x7C;
			}
			if(buf[0] == 0x81 || buf[0] == 0x82 || buf[0] == 0x83 || buf[0] == 0x84 || buf[0] == 0x91 || buf[0] == 0x92 || buf[0] == 0x93 || buf[0] == 0x94) {
				printk("%s, ic is 2116\n", __func__);
				chip_id_H = 0x21;
				chip_id_L = 0x16;
				ic_model = (chip_id_H << 8) + chip_id_L;
				printk("IC Type: 2116----------------------\n");
				iceRomSize = 0x7F00;
				iceLockPage = 0x7FFF;
				icestartsector = 0x78;	
				iceendsector = 0x7C;
				printk("%s, ic is 0x%x\n", __func__,ic_model);
			}
			outwrite(0x041004, 0x9F000960, 4);
			outwrite(0x041008, 0x24, 1);
		}
		#if 0
		//Device clock ON
		ret = outwrite(0x40020, 0x00000000, 4);
		printk("%s, Device clock ON, outwrite, ret = %d\n", __func__, ret);
		#endif
		
		printk("ic = 0x%04x\n", ic_model);
		
		printk("%s, set_48MHz\n", __func__);
		set_48MHz(chip_id_H, chip_id_L);
		
		erase_data(0);

		//write data
		if(((ap_endaddr + 1) % 0x10) != 0)
		{
			ap_endaddr += 0x10;
		}
		
		//Setting program key
		set_program_key();
		printk("%s, Setting program key\n", __func__);
		
		if ((ic_model & 0xFF00) != 0x2100) {
			Set2ndHighVoltage_2210(chip_id_H, chip_id_L);
		}
		
		j = 0;
		for(i = ap_startaddr; i < ap_endaddr; i += 16)
		{
			buf[0] = 0x25;
			buf[3] = (char)((i  & 0x00FF0000) >> 16);
			buf[2] = (char)((i  & 0x0000FF00) >> 8);
			buf[1] = (char)((i  & 0x000000FF));
			for(k = 0; k < 16; k++)
			{
				buf[4 + k] = CTPM_FW[i + 32 + k];
			}
			//msgs[0].len = 20;
			//ret = ilitek_i2c_transfer(i2c.client, msgs, 1); 
			ret = ilitek_i2c_write(i2c.client, buf, 20);
			upgrade_status = ((i * 100)) / ap_endaddr;
			if(upgrade_status > j)
			{
				printk("%c ilitek ILITEK: Firmware Upgrade(AP), %02d%c. \n", 0x0D, upgrade_status, '%');
				j = j + 10;
			}
			//msleep(1);
			mdelay(1);
			//mtp_control_reg(100);
		}
		printk("%s, upgrade end(program)\n", __func__);

		ice_checksum = getchecksum(ap_startaddr, ap_endaddr, ap_checksum);

		if ((ic_model & 0xFF00) != 0x2100) {
			SetDefaultVoltage_2210(chip_id_H, chip_id_L);
		}
#if 1
		// for data flash
		if(ic_model == 0x2115 || ic_model == 0x2116) {
			printk("data flash IC Type: 2115--or --2116------------------\n");
			ifbiceRomStart = 0x7D00;
			ifbiceRomEnd = 0x7E00;
			ifbicestartsector = 0x7D;  
			ifbiceendsector = 0x7E;
		}
		else {
			printk("%s, =====data flash ===========ic is 2210 serial\n", __func__);
			ifbiceRomStart = 0xBD00;
			ifbiceRomEnd = 0xBE00;
			ifbicestartsector = 0xBD;
			ifbiceendsector = 0xBE;
		}
		erase_data(1);
		//Setting program key
		set_program_key();
		printk("%s, Setting program key\n", __func__);
		
		if ((ic_model & 0xFF00) != 0x2100) {
			Set2ndHighVoltage_2210(chip_id_H, chip_id_L);
		}
		
		for(i = df_startaddr; i < df_endaddr; i += 16) {
			buf[0] = 0x25;
			buf[3] = (char)((i	& 0x00FF0000) >> 16);
			buf[2] = (char)((i	& 0x0000FF00) >> 8);
			buf[1] = (char)((i	& 0x000000FF));
			for(k = 0; k < 16; k++) {
				buf[4 + k] = CTPM_FW[i + 32 + k];
			}
			ret = ilitek_i2c_write(i2c.client, buf, 20);
			//mtp_control_reg(100);
			//msleep(1);
			mdelay(1);
			upgrade_status = (((i - df_startaddr) * 100)) / (df_endaddr - df_startaddr);
			//printk("%s, jni.fm.upgrade_status = %d\n", __func__, jni.fm.upgrade_status);
		}
		printk("%s, upgrade end(program)\n", __func__);
		//if (df_start_addr > df_end_addr) {
			ice_checksum = getchecksum(df_startaddr, df_endaddr, df_checksum);
		//}
		if ((ic_model & 0xFF00) != 0x2100) {
			SetDefaultVoltage_2210(chip_id_H, chip_id_L);
		}
#endif
		printk("%s, Clear program key (0x41014)\n", __func__);
		clear_program_key();

		exit_ice_mode();
		
		if(!ice_checksum)
		{
			//check system busy
			for(i = 0; i < 50 ; i++)
			{
				buf[0] = 0x80;
				ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 1);
				//ilitek_i2c_read(i2c.client, 0x80, buf, 1);
				if(buf[0] == 0x50)
				{
					printk("%s, system is not busy, i = %d\n", __func__, i);
					break;
				}
			}
		}
		if(i == 50 && buf[0] != 0x50)
		{
			printk("%s, system is busy, i = %d\n", __func__, i);
		}
	}
	else
	{	
		#if 1
		for (i = 0; i < 5; i++) {
			//buf[0] = 0xc0;
			//msgs[0].len = 1;
			buf[0] = 0xc0;
			ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 2);
			//ret = ilitek_i2c_read(i2c.client, 0xc0, buf, 1);
			if(ret < 0)
			{
				return 3;
			}
			msleep(30);
			printk("ilitek ic. mode = %d\n", buf[0]);
				
			if(buf[0] != 0x55)
			{	
				buf[0] = 0xc4;
				buf[1] = 0x5A;
				buf[2] = 0xA5;
				//msgs[0].len = 3;
				//ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
				ret = ilitek_i2c_write(i2c.client, buf, 3);
				if(ret < 0)
				{
					return 3;
				}
				msleep(30);
				buf[0] = 0xc2;
				//ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
				ret = ilitek_i2c_write(i2c.client, buf, 1);
				if(ret < 0)
				{
					return 3;
				}		
				msleep(100);
			}
			else {
				break;
			}
		}
		if(buf[0] != OP_MODE_BOOTLOADER) {
			printk("%s, current op mode is application mode, already retry change to bootloader mode four count, will stop update firmware, 0x%X\n", __func__, buf[0]);
		}
		else {
			printk("ilitek change to bl mode ok\n");
		}

		//check support fast mode update firmware
		if(buf[1] == 0x80) {
			printk("%s, this bl protocol support fast update firmware\n", __func__);
			fast_update_flag = 1;
		}
		#if 0
		buf[0] = ILITEK_TP_CMD_GET_FIRMWARE_VERSION;
		ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 4);
		if(ret < 0)
		{
			return 3;
		}
		
		printk(ILITEK_DEBUG_LEVEL "%s, bl version %d.%d.%d.%d\n", __func__, buf[0], buf[1], buf[2], buf[3]);
		msleep(100);
		bl_ver = buf[3] + (buf[2] << 8) + (buf[1] << 16) + (buf[0] << 24);
		if(bl_ver)
		{
			if(bl_ver < 0x01000100)
			{
				flow_flag = 1;
			}
			else
			{
				flow_flag = 2;
			}
		}
		else
		{
			return	3;
		}
		#endif
		#if 1
		buf[0] = ILITEK_TP_CMD_GET_PROTOCOL_VERSION;
		ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 4);
		if(ret < 0)
		{
			return 3;
		}
		bl_ver0 = buf[0];
		bl_ver1 = buf[1];
		if (bl_ver0 == 1 && bl_ver1 >= 4) {
			buf[0] = ILITEK_TP_CMD_GET_KERNEL_VERSION;
			ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 6);
			df_startaddr = buf[2] * 256 * 256 + buf[3] * 256 + buf[4];
			printk("%s, df_start_addr = %x", __func__, (int)df_startaddr);
			if(buf[0] != 0x05) {
				page_number = 16;
				printk("ilitek page_number = 16, page is 512 bytes\n");
			} else {
				printk("ilitek page_number = 8, page is 256 bytes\n");
			}
#if 1
			buf[0] = 0xc7;
			ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 1);
			printk("%s, 0xc7 read= %x", __func__, buf[0]);
#endif
		}
		#endif
		//buf[0] = 0xc0;
		//msgs[0].len = 1;
		//ret = ilitek_i2c_read(i2c.client, 0xc0, buf, 1);
		buf[0] = 0xc0;
		ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 1);
		if(ret < 0)
		{
			return 3;
		}
	
		msleep(30);
		printk("ILITEK:%s, upgrade firmware...\n", __func__);
		if (bl_ver0 == 1 && bl_ver1 >= 4) {
			
			#if 1
			buf[0] = (unsigned char)ILITEK_TP_CMD_WRITE_ENABLE;//0xc4
			buf[1] = 0x5A;
			buf[2] = 0xA5;
			buf[3] = 0x81;
			
			#if 1
			if (handle_df && !has_df) { 
				printk("ilitek no df data\n");
				df_endaddr = 0x1ffff;
				df_checksum = 0x1000 * 0xff;
				buf[4] = df_endaddr >> 16;
				buf[5] = (df_endaddr >> 8) & 0xFF;
				buf[6] = (df_endaddr) & 0xFF;
				buf[7] = df_checksum >> 16;
				buf[8] = (df_checksum >> 8) & 0xFF;
				buf[9] = df_checksum & 0xFF;
			}
			else {
				buf[4] = df_endaddr >> 16;
				buf[5] = (df_endaddr >> 8) & 0xFF;
				buf[6] = (df_endaddr) & 0xFF;
				buf[7] = df_checksum >> 16;
				buf[8] = (df_checksum >> 8) & 0xFF;
				buf[9] = df_checksum & 0xFF;
			}
			#endif
			ilitek_i2c_write(i2c.client, buf, 10);
			msleep(20);
			#endif
			printk("ilitek df_startaddr=0x%lX, df_endaddr=0x%lX, df_checksum=0x%lX, df_len = 0x%d\n", df_startaddr, df_endaddr, df_checksum, df_len);
			j = 0;
			for(i = df_startaddr; i < df_endaddr; i += 32)
			{
				buf[0] = ILITEK_TP_CMD_WRITE_DATA;
				for(k = 0; k < 32; k++)
				{
					if (has_df) { 
						if ((i + k) >= df_endaddr) {
							buf[1 + k] = 0x00;
						}
						else {
							buf[1 + k] = CTPM_FW[i + 32 + k];
						}
					}
					else {
						buf[1 + k] = 0xff;
					}
				
					//buf[1 + k] = CTPM_FW[i + 32 + k];
				}
		
				//msgs[0].len = 33;
				//ret = ilitek_i2c_transfer(i2c.client, msgs, 1); 
				ret = ilitek_i2c_write(i2c.client, buf, 33);
				if(ret < 0)
				{
					return 3;
				}
				j += 1;
				if((j % (page_number)) == 0) {
					mdelay(20);
				}
				upgrade_status = ((i - df_startaddr) * 100) / (df_endaddr - df_startaddr);
				mdelay(10);
				printk("%c ilitek ILITEK: Firmware Upgrade(Data flash), %02d%c. \n",0x0D,upgrade_status,'%');
			}
			#if 1
			buf[0] = (unsigned char)ILITEK_TP_CMD_WRITE_ENABLE;//0xc4
			buf[1] = 0x5A;
			buf[2] = 0xA5;
			buf[3] = 0x80;
			#if 1
			if ((ap_endaddr % 32) != 0) {
				printk("32 no 0 ap_endaddr 32 = 0x%X\n", (int)(ap_endaddr + 32 + 32 - 1 - (ap_endaddr % 32)));
				printk("ap_endaddr = 0x%X\n", (int)(ap_endaddr + 32 + 32 - 1 - (ap_endaddr % 32)));
				buf[4] = (ap_endaddr + 32 + 32 - 1 - (ap_endaddr % 32)) >> 16;
				buf[5] = ((ap_endaddr + 32 + 32 - 1 - (ap_endaddr % 32)) >> 8) & 0xFF;
				buf[6] = ((ap_endaddr + 32 + 32 - 1 - (ap_endaddr % 32))) & 0xFF;
				printk("ap_checksum = 0x%X\n", (int)(ap_checksum + (32 + 32 - 1 - (ap_endaddr % 32)) * 0xff));
				buf[7] = (ap_checksum + (32 + 32 - 1 -(ap_endaddr % 32)) * 0xff) >> 16;
				buf[8] = ((ap_checksum + (32 + 32 - 1 -(ap_endaddr % 32)) * 0xff) >> 8) & 0xFF;
				buf[9] = (ap_checksum + (32 + 32 - 1 - (ap_endaddr % 32)) * 0xff) & 0xFF;
			}
			else {
				printk("32 0 ap_endaddr  32 = 0x%X\n", (int)(ap_endaddr + 32));
				printk("ap_endaddr = 0x%X\n", (int)(ap_endaddr + 32));
				buf[4] = (ap_endaddr + 32) >> 16;
				buf[5] = ((ap_endaddr + 32) >> 8) & 0xFF;
				buf[6] = ((ap_endaddr + 32)) & 0xFF;
				printk("ap_checksum = 0x%X\n", (int)(ap_checksum + (ap_endaddr + 32) * 0xff));
				buf[7] = (ap_checksum + (ap_endaddr + 32) * 0xff) >> 16;
				buf[8] = ((ap_checksum + (ap_endaddr + 32) * 0xff) >> 8) & 0xFF;
				buf[9] = (ap_checksum + (ap_endaddr + 32) * 0xff) & 0xFF;
			}
			#endif
			ret = ilitek_i2c_write(i2c.client, buf, 10);
			msleep(20);
			#endif
			#if 1
			tmp_ap_end_addr = ap_endaddr;
			//if(((ap_end_addr + 1) % 32) != 0) {
				ap_endaddr += 32;
			//}
			//for(i = ap_startaddr; i < ap_endaddr; i += 32)
			#endif
			#if 0
			tmp_ap_end_addr = ap_len;
			//if(((ap_end_addr + 1) % 32) != 0) {
				ap_len += 32;
			//}
			#endif
			j = 0;
			for(i = ap_startaddr; i < ap_endaddr; i += 32)
			{
				buf[0] = ILITEK_TP_CMD_WRITE_DATA;
				for(k = 0; k < 32; k++)
				{
					if((i + k) > tmp_ap_end_addr) {
						buf[1 + k] = 0xff;
					}
					else {
						buf[1 + k] = CTPM_FW[i + k + 32];
					}
					//buf[1 + k] = CTPM_FW[i + df_len + 32 + k];
				}
		
				buf[0] = 0xc3;
				//msgs[0].len = 33;
				//ret = ilitek_i2c_transfer(i2c.client, msgs, 1); 
				ret = ilitek_i2c_write(i2c.client, buf, 33);
				if(ret < 0)
				{
					return 3;
				}
				j += 1;
				if((j % (page_number)) == 0) {
					//msleep(30);
					mdelay(20);
				}
				upgrade_status = (i * 100) / ap_endaddr;
				//upgrade_status = (i * 100) / ap_len;
				//msleep(6);
				mdelay(10);
				printk("%c ilitek ILITEK: Firmware Upgrade(AP), %02d%c. \n", 0x0D, upgrade_status, '%');
			}
		
		}
		else {
				printk("-------ilitek--------BL protocol < 1.4---------------\n");
				
			#if 1
				buf[0] = ILITEK_TP_CMD_WRITE_ENABLE;//0xC4
				buf[1] = 0x5A;
				buf[2] = 0xA5;
				ret = ilitek_i2c_write(i2c.client, buf, 3);
				if(ret < 0)
				{
					return 3;
				}
				if (df_endaddr > df_startaddr) {
					ap_endaddr = df_endaddr;
				}
				tmp_ap_end_addr = ap_endaddr;
				if((ap_endaddr % 32) != 0) {
					ap_endaddr += 32;
				}
				j = 0;
				if (handle_df && !has_df) {
					printk("ilitek no df data set end_addr = 0x1ffff\n");
					ap_endaddr = 0x1ffff;
				}
				for(i = ap_startaddr; i < ap_endaddr; i += 32) {
					//we should do delay when the size is equal to 512 bytes
					j += 1;
					if((j % 16) == 1) {
						mdelay(30);
					}
					buf[0] = (unsigned char)ILITEK_TP_CMD_WRITE_DATA;
					for(k = 0; k < 32; k++) {
						if((i + k) > tmp_ap_end_addr) {
							buf[1 + k] = 0xff;
						}
						else {
							buf[1 + k] = CTPM_FW[i + k + 32];
						}
					}
					if(ilitek_i2c_write(i2c.client, buf, 33) < 0) {
						return 3;
					}
					upgrade_status = (i * 100) / ap_endaddr;
					mdelay(10);
					printk("%c ilitek ILITEK: Firmware Upgrade(AP), %02d%c. \n", 0x0D, upgrade_status, '%');
				}
			#endif
			}
		printk("ILITEK:%s, upgrade firmware completed  reset\n", __func__);
		ilitek_reset(i2c.reset_gpio);
		msleep(50);
		#if 0
		buf[0] = 0x60;
		msleep(50);
		ret = ilitek_i2c_write(i2c.client, buf, 1);
		#endif
		buf[0] = 0xc0;
		ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 2);
		if(ret < 0){
			return 3;
		}
		msleep(30);
		printk("ilitek ic. mode =%d , it's	%s \n",buf[0],((buf[0] == 0x5A)?"AP MODE":"BL MODE"));
		if (buf[0] == 0x5A) {
			printk("ilitek change to AP mode ok\n");
		}
		else {
			for (i = 0; i < 5; i++) {
				buf[0] = ILITEK_TP_CMD_WRITE_ENABLE;
				buf[1] = 0x5A;
				buf[2] = 0xA5;
				ret = ilitek_i2c_write(i2c.client, buf, 3);
				if(ret < 0){
					return 3;
				}
				msleep(50);
				buf[0] = 0xc1;
				ret = ilitek_i2c_write(i2c.client, buf, 1);
				if(ret < 0){
					return 3;
				}
				msleep(50);
				buf[0] = 0xc0;
				ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 2);
				if(ret < 0){
					return 3;
				}
				msleep(30);
				printk("ilitek ic. mode =%d , it's  %s \n",buf[0],((buf[0] == 0x5A)?"AP MODE":"BL MODE"));
				if (buf[0] == 0x5A) {
					printk("ilitek change to AP mode ok\n");
					break;
				}
			}
		}
		#endif
	}
#endif
	#endif
	msleep(100);
	return 2;
}


#endif

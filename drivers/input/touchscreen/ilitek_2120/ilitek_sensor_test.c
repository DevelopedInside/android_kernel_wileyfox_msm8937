#include "ilitek_ts.h"
#ifdef SENSOR_TEST
#define SYS_ATTR_FILE
#ifdef SYS_ATTR_FILE
	 void ilitek_sensor_test_deinit(void);
	 void ilitek_sensor_test_init(void);
	extern struct i2c_data i2c;

#endif
#endif

#ifdef SENSOR_TEST
static struct kobject *ilitek_android_touch_kobj = NULL;

static int i2c_connect = 1;
static int microshorttestwindow = 7, short_test_result = 1;
static int opentestthrehold = 3200, open_test_result = 1;
static int allnodetestymax = 2500, allnodetestymin = 1500, allnode_test_result = 1;
static int allnodetestymax_self = 2500, allnodetestymin_self = 1500, allnode_self_test_result = 1;

static int ilitek_ready_into_test(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	uint8_t buf[32] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf,}
	};

	cmd[0] = 0x10;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		i2c_connect = 0;
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	msgs_ret[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
		return ret;
	}
	if (buf[1] < 0x80) {
		printk("ilitek_ready_into_test IC is not ready\n");
	}
	cmd[0] = 0xF6;
	cmd[1] = 0x13;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		return ret;
	}

	cmd[0] = 0x13;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	msgs_ret[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
	

}
static int ilitek_into_testmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	cmd[0] = 0xF0;
	cmd[1] = 0x01;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	mdelay(10);
	return 0;
}
static int ilitek_into_normalmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	cmd[0] = 0xF0;
	cmd[1] = 0x00;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
#ifdef SYS_ATTR_FILE

static ssize_t ilitek_open_test(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0, len = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc((sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32), GFP_KERNEL);
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};
	int test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if (!buf_recv) {
		//printk("ilitek_all_test buf_recv kzalloc error\n");
	}
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	open_test_result = 1;
	ilitek_i2c_irq_disable();
	ret = ilitek_ready_into_test();
	ret = ilitek_into_testmode();
	cmd[0] = 0xF1;
	cmd[1] = 0x06;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		//return ret;
	}
	mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		//return ret;
	}
	printk("ilitek_open_test test_32 = %d\n", test_32);
	for(i = 0; i < test_32; i++){
		if ((i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		printk("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	for(i = 0; i < test_32; i++) {
		if (j == i2c.y_ch * i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < opentestthrehold) {
				open_test_result = 0;
			}
			sprintf(buf + len, "%4d, ", (buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			j++;
			len += 6;
			if(j % i2c.x_ch == 0) {
				sprintf(buf + len, "\n");
				len += 1;
			}
			if (j == i2c.y_ch * i2c.x_ch) {
				break;
			}
		}
	}
	sprintf(buf + len, "\n");
	len += 1;
	printk("ilitek_open_test len = %d\n", len);
	ret = ilitek_into_normalmode();
	ilitek_i2c_irq_enable();
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	return len;
}
static ssize_t ilitek_open_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	int ret = 0;
	ret = kstrtoint(buf, 10, &opentestthrehold);
	printk("ilitek_open_test_store opentestthrehold = %d\n", opentestthrehold);
	if (ret) {
		printk("ilitek_open_test_store kstrtoint error\n");
		return ret;
	}
	return size;
}
static DEVICE_ATTR(ilitek_open_test, S_IRWXUGO, ilitek_open_test, ilitek_open_store);

static ssize_t ilitek_short_test(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0, len = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.x_ch * 2 + 32, GFP_KERNEL);
	int * x1_data = kzalloc(sizeof(int) * i2c.x_ch + 32, GFP_KERNEL);
	int * x2_data = kzalloc(sizeof(int) * i2c.x_ch + 32, GFP_KERNEL);
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};
	int test_32 = (i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	short_test_result = 1;
	ilitek_i2c_irq_disable();
	ret = ilitek_ready_into_test();
	ret = ilitek_into_testmode();
	#if 1
	cmd[0] = 0xF1;
	cmd[1] = 0x04;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	for(i = 0; i < test_32; i++){
		if ((i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		printk("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	j = 0;
	len = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.x_ch * 2) {
			break;
		}
		for(index = 2; index < newMaxSize; index++) {
			if (j < i2c.x_ch) {
				x1_data[j] = buf_recv[i * newMaxSize + index];
			}
			else {
				x2_data[j - i2c.x_ch] = buf_recv[i * newMaxSize + index];
			}
			sprintf(buf + len, "%4d, ", buf_recv[i * newMaxSize + index]);
			j++;
			len += 6;
			if(j % i2c.x_ch == 0) {
				sprintf(buf + len, "\n");
				len += 1;
			}
			if (j == i2c.x_ch * 2) {
				break;
			}
		}
	}
	for (i = 0; i < i2c.x_ch; i++) {
		if (abs(x1_data[i] - x2_data[i]) > microshorttestwindow) {
			short_test_result = 0;
		}
	}
	sprintf(buf + len, "\n");
	len += 1;
	#endif
	printk("ilitek_short_test len = %d\n", len);
	ret = ilitek_into_normalmode();
	ilitek_i2c_irq_enable();
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	if (x1_data) {
		kfree(x1_data);
		x1_data = NULL;
	}
	if (x2_data) {
		kfree(x2_data);
		x2_data = NULL;
	}
	//return ret;
	return len;
}
static ssize_t ilitek_short_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	int ret = 0;
	ret = kstrtoint(buf, 10, &microshorttestwindow);
	printk("ilitek_open_test_store microshorttestwindow = %d\n", microshorttestwindow);
	if (ret) {
		printk("ilitek_open_test_store kstrtoint error\n");
		return ret;
	}
	return size;
}

static DEVICE_ATTR(ilitek_short_test, S_IRWXUGO, ilitek_short_test, ilitek_short_store);
static ssize_t ilitek_allnode_test(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0, newMaxSize = 32, i = 0, j = 0, index = 0, len = 0, frame_count = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32, GFP_KERNEL);
	unsigned int * allnode_data = kzalloc(sizeof(int) * i2c.y_ch * i2c.x_ch + 32, GFP_KERNEL);
	unsigned int * allnode_data_min = kzalloc(sizeof(int) * i2c.y_ch * i2c.x_ch + 32, GFP_KERNEL);
	unsigned int * allnode_data_max = kzalloc(sizeof(int) * i2c.y_ch * i2c.x_ch + 32, GFP_KERNEL);
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};
	int test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	if (!buf_recv) {
		//printk("ilitek_all_test buf_recv kzalloc error\n");
	}
	memset(allnode_data_min, 0xFF, sizeof(int) * i2c.y_ch * i2c.x_ch + 32);
	memset(allnode_data_max, 0, sizeof(int) * i2c.y_ch * i2c.x_ch + 32);
	printk("allnode_data_min[100] = 0x%x, allnode_data_max[100] = 0x%x\n", allnode_data_min[100], allnode_data_max[100]);
	allnode_test_result = 1;
	allnode_self_test_result = 1;
	ilitek_i2c_irq_disable();
	gpio_direction_output(i2c.reset_gpio,1);
	//msleep(50);
	mdelay(50);
	gpio_direction_output(i2c.reset_gpio,0);
	//msleep(50);
	mdelay(50);
	gpio_direction_output(i2c.reset_gpio,1);
	//msleep(100);
	//mdelay(100);

	mdelay(10);
	for (i =0; i < 300; i++ ) {
		ret = ilitek_poll_int();
		printk("ilitek int status = %d\n", ret);
		if (ret == 0) {
			break;
		}
		else {
			mdelay(5);
		}
	}
	ret = ilitek_ready_into_test();
	ret = ilitek_into_testmode();

	
	cmd[0] = 0x0A;
	cmd[1] = 0x00;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	mdelay(10);
	cmd[0] = 0x01;
	cmd[1] = 0x01;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	mdelay(10);
	for (frame_count = 0; frame_count < 10; frame_count++) {
		len = 0;
		j = 0;
		cmd[0] = 0xF1;
		cmd[1] = 0x03;
		cmd[2] = 0x00;
		msgs_cmd[0].len = 3;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
			//return ret;
		}
		//msleep(3);
		for (i =0; i < 300; i++ ) {
			ret = ilitek_poll_int();
			printk("ilitek int status = %d\n", ret);
			if (ret == 1) {
				break;
			}
			else {
				mdelay(5);
			}
		}
		//mdelay(350);
		cmd[0] = 0xF6;
		cmd[1] = 0xF2;
		msgs_cmd[0].len = 2;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
			//return ret;
		}
		cmd[0] = 0xF2;
		msgs_cmd[0].len = 1;
		ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
		if(ret < 0){
			printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
			//return ret;
		}
		for(i = 0; i < test_32; i++){
			if ((i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
				msgs_ret[0].len = (i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) + 2;
				msgs_ret[0].buf = buf_recv + newMaxSize*i;
				ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
				if(ret < 0){
					printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
					//return ret;
				}
			}
			else {
				msgs_ret[0].len = newMaxSize;
				msgs_ret[0].buf = buf_recv + newMaxSize*i;
				ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
				if(ret < 0){
					printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
					//return ret;
				}
			}
			printk("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
		}
		for(i = 0; i < test_32; i++) {
			if (j == i2c.y_ch * i2c.x_ch) {
				break;
			}
			for(index = 2; index < newMaxSize; index += 2) {
				if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < allnodetestymin || 
					((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) > allnodetestymax) {
					allnode_test_result = 0;
				}
				if (allnode_data_max[j] < ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index])) {
					allnode_data_max[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
				}
				if (allnode_data_min[j] > ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index])) {
					allnode_data_min[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
				}
				if (frame_count == 0) {
					allnode_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
				}
				sprintf(buf + len, "%4d, ", (buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
				printk("%d,", (buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
				j++;
				len += 6;
				if(j % i2c.x_ch == 0) {
					sprintf(buf + len, "\n");
					printk("\n");
					len += 1;
				}
				if (j == i2c.y_ch * i2c.x_ch) {
					break;
				}
			}
		}
	}
	sprintf(buf + len, "\n");
	printk("ilitek allnode data\n");
	for (i = 0; i < i2c.y_ch * i2c.x_ch; i++) {
		if (i % i2c.x_ch == 0) {
			printk("\n");
		}
		printk("%d, ", allnode_data[i]);
	}
	printk("\n");
	printk("ilitek allnode Tx delta\n");
	for (i = 0; i < i2c.y_ch - 1; i++) {
		for (j = 0; j < i2c.x_ch; j++) {
			printk("%d, ", (int)abs(allnode_data[i * i2c.x_ch + j] - allnode_data[(i + 1) * i2c.x_ch + j]));
		}
		printk("\n");
	}
	printk("\n");
	printk("ilitek allnode data\n");
	for (i = 0; i < i2c.y_ch * i2c.x_ch; i++) {
		if (i % i2c.x_ch == 0) {
			printk("\n");
		}
		printk("%d, ", allnode_data[i]);
	}
	printk("\n");
	printk("ilitek allnode Rx delta\n");
	for (i = 0; i < i2c.y_ch; i++) {
		for (j = 0; j < i2c.x_ch - 1; j++) {
			printk("%d, ", (int)abs(allnode_data[i * i2c.x_ch + j + 1] - allnode_data[i * i2c.x_ch + j]));
		}
		printk("\n");
	}
	printk("\n");
	printk("ilitek allnode max\n");
	for (i = 0; i < i2c.y_ch * i2c.x_ch; i++) {
		if (i % i2c.x_ch == 0) {
			printk("\n");
		}
		printk("%d, ", allnode_data_max[i]);
	}
	printk("\n");
	printk("ilitek allnode min\n");
	for (i = 0; i < i2c.y_ch * i2c.x_ch; i++) {
		if (i % i2c.x_ch == 0) {
			printk("\n");
		}
		printk("%d, ", allnode_data_min[i]);
	}
	printk("\n");
	printk("ilitek allnode max - min\n");
	for (i = 0; i < i2c.y_ch * i2c.x_ch; i++) {
		if (i % i2c.x_ch == 0) {
			printk("\n");
		}
		printk("%d, ", allnode_data_max[i] - allnode_data_min[i]);
	}
	printk("\n");
	len += 1;
	test_32 = (i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	cmd[0] = 0xF1;
	cmd[1] = 0x0B;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	//msleep(3);
	for (i =0; i < 300; i++ ) {
		ret = ilitek_poll_int();
		printk("ilitek int status = %d\n", ret);
		if (ret == 1) {
			break;
		}
		else {
			mdelay(5);
		}
	}
	//mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	for(i = 0; i < test_32; i++){
		if ((i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		printk("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < allnodetestymin_self|| 
				((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) > allnodetestymax_self) {
				allnode_self_test_result = 0;
			}
			sprintf(buf + len, "%4d, ", (buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			j++;
			len += 6;
			if(j % i2c.x_ch == 0) {
				sprintf(buf + len, "\n");
				len += 1;
			}
			if (j == i2c.x_ch) {
				break;
			}
		}
	}
	sprintf(buf + len, "\n");
	len += 1;
	printk("ilitek_allnode_test len = %d\n", len);
	ret = ilitek_into_normalmode();
	ilitek_reset(i2c.reset_gpio);
	ilitek_i2c_irq_enable();
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	if (allnode_data) {
		kfree(allnode_data);
		allnode_data = NULL;
	}
	if (allnode_data_max) {
		kfree(allnode_data_max);
		allnode_data_max = NULL;
	}
	if (allnode_data_min) {
		kfree(allnode_data_min);
		allnode_data_min = NULL;
	}
	return len;
}
static ssize_t ilitek_allnode_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{	
	if (sscanf(buf, "%02d:%02d:%02d:%02d\n", &allnodetestymax, &allnodetestymax_self, &allnodetestymin, &allnodetestymin_self) != 4){
		printk("ilitek cmd format error\n");
		//return -EINVAL;
	}
	printk("ilitek allnodetestymax = %d, allnodetestymax_self = %d, allnodetestymin = %d, allnodetestymin_self = %d\n", allnodetestymax, allnodetestymax_self, allnodetestymin, allnodetestymin_self);
	return size;
}

static DEVICE_ATTR(ilitek_allnode_test, S_IRWXUGO, ilitek_allnode_test, ilitek_allnode_store);


static ssize_t ilitek_all_test(struct device *dev,
		struct device_attribute *attr, char *buf)
{	int len = 0, i = 0;
	#if IC2120
	int ret = 0, newMaxSize = 32, j = 0, index = 0;
	uint8_t cmd[4] = {0};
	uint8_t * buf_recv = kzalloc(sizeof(uint8_t) * i2c.y_ch * i2c.x_ch * 2 + 32, GFP_KERNEL);
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	struct i2c_msg msgs_ret[] = {
		{.addr = i2c.client->addr, .flags = I2C_M_RD, .len = 3, .buf = buf_recv,}
	};
	int * x1_data = kzalloc(sizeof(int) * i2c.x_ch + 32, GFP_KERNEL);
	int * x2_data = kzalloc(sizeof(int) * i2c.x_ch + 32, GFP_KERNEL);
	int * open_data = kzalloc(sizeof(int) * i2c.y_ch * i2c.x_ch + 32, GFP_KERNEL);
	int * allnode_data = kzalloc(sizeof(int) * i2c.y_ch * i2c.x_ch + 32, GFP_KERNEL);
	int * allnode_self_data = kzalloc(sizeof(int)  * i2c.x_ch + 32, GFP_KERNEL);
	int test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	#if 0
	buf = kzalloc(sizeof(char)  * 4320, GFP_KERNEL);
	printk("ilitek_all_test kzalloc4320  buf\n");
	#endif
	short_test_result = 1;
	open_test_result = 1;
	allnode_test_result = 1;
	allnode_self_test_result = 1;
	ilitek_i2c_irq_disable();
	ret = ilitek_ready_into_test();
	ret = ilitek_into_testmode();
#if 1
	cmd[0] = 0xF1;
	cmd[1] = 0x06;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		//return ret;
	}
	msleep(3);
	for (i =0; i < 300; i++ ) {
		ret = ilitek_poll_int();
		printk("ilitek int status = %d\n", ret);
		if (ret == 1) {
			break;
		}
		else {
			msleep(5);
		}
	}
	//mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		//return ret;
	}
	printk("ilitek_open_test test_32 = %d\n", test_32);
	for(i = 0; i < test_32; i++){
		if ((i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		//printk("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.y_ch * i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			open_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < opentestthrehold) {
				open_test_result = 0;
			}
			//printk("%4d, ", (buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			j++;
			if(j % i2c.x_ch == 0) {
				//printk("\n");
			}
			if (j == i2c.y_ch * i2c.x_ch) {
				break;
			}
		}
	}
	//printk("\n");
#endif

#if 1
	test_32 = (i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	cmd[0] = 0xF1;
	cmd[1] = 0x04;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	msleep(3);
	for (i =0; i < 300; i++ ) {
		ret = ilitek_poll_int();
		printk("ilitek int status = %d\n", ret);
		if (ret == 1) {
			break;
		}
		else {
			msleep(5);
		}
	}
	//mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	for(i = 0; i < test_32; i++){
		if ((i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		//printk("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.x_ch * 2) {
			break;
		}
		for(index = 2; index < newMaxSize; index++) {
			if (j < i2c.x_ch) {
				x1_data[j] = buf_recv[i * newMaxSize + index];
			}
			else {
				x2_data[j - i2c.x_ch] = buf_recv[i * newMaxSize + index];
			}
			//printk("%4d, ", buf_recv[i * newMaxSize + index]);
			j++;
			if(j % i2c.x_ch == 0) {
				//printk("\n");
			}
			if (j == i2c.x_ch * 2) {
				break;
			}
		}
	}
	for (i = 0; i < i2c.x_ch; i++) {
		if (abs(x1_data[i] - x2_data[i]) > microshorttestwindow) {
			short_test_result = 0;
		}
	}
	//printk("\n");
#endif
	test_32 = (i2c.y_ch * i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.y_ch * i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	cmd[0] = 0xF1;
	cmd[1] = 0x08;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	msleep(3);
	for (i =0; i < 300; i++ ) {
		ret = ilitek_poll_int();
		printk("ilitek int status = %d\n", ret);
		if (ret == 1) {
			break;
		}
		else {
			msleep(5);
		}
	}
	//mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	for(i = 0; i < test_32; i++){
		if ((i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.y_ch * i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		//printk("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.y_ch * i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			allnode_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < allnodetestymin || 
				((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) > allnodetestymax) {
				allnode_test_result = 0;
			}
			//printk("0x%x, ", (buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			j++;
			if(j % i2c.x_ch == 0) {
				//printk("\n");
			}
			if (j == i2c.y_ch * i2c.x_ch) {
				break;
			}
		}
	}
	printk("\n");
	test_32 = (i2c.x_ch * 2) / (newMaxSize - 2);
	if ((i2c.x_ch * 2) % (newMaxSize - 2) != 0) {
		test_32 += 1;
	}
	cmd[0] = 0xF1;
	cmd[1] = 0x0B;
	cmd[2] = 0x00;
	msgs_cmd[0].len = 3;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	msleep(3);
	for (i =0; i < 300; i++ ) {
		ret = ilitek_poll_int();
		printk("ilitek int status = %d\n", ret);
		if (ret == 1) {
			break;
		}
		else {
			msleep(5);
		}
	}
	//mdelay(350);
	cmd[0] = 0xF6;
	cmd[1] = 0xF2;
	msgs_cmd[0].len = 2;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	cmd[0] = 0xF2;
	msgs_cmd[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		//return ret;
	}
	for(i = 0; i < test_32; i++){
		if ((i2c.x_ch * 2)%(newMaxSize - 2) != 0 && i == test_32 - 1) {
			msgs_ret[0].len = (i2c.x_ch * 2)%(newMaxSize - 2) + 2;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		else {
			msgs_ret[0].len = newMaxSize;
			msgs_ret[0].buf = buf_recv + newMaxSize*i;
			ret = ilitek_i2c_transfer(i2c.client, msgs_ret, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
				//return ret;
			}
		}
		//printk("buf_recv 0x%x, 0x%x\n", buf_recv[newMaxSize*i], buf_recv[newMaxSize*i + 1]);
	}
	j = 0;
	for(i = 0; i < test_32; i++) {
		if (j == i2c.x_ch) {
			break;
		}
		for(index = 2; index < newMaxSize; index += 2) {
			allnode_self_data[j] = ((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			if (((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) < allnodetestymin_self|| 
				((buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]) > allnodetestymax_self) {
				allnode_self_test_result = 0;
			}
			//printk("0x%x, ", (buf_recv[i * newMaxSize + index + 1] << 8) + buf_recv[i * newMaxSize + index]);
			j++;
			if(j % i2c.x_ch == 0) {
				//printk("\n");
			}
			if (j == i2c.x_ch) {
				break;
			}
		}
	}
	//printk("\n");
	ret = ilitek_into_normalmode();
	ilitek_i2c_irq_enable();
	#if 1 //printf data
	len = 0;
	sprintf(buf + len, "*****i2c_connect***open***short***allnode*****\n");
	len += 47;
	if (i2c_connect) {
		sprintf(buf + len, "1P-");
	}
	else {
		sprintf(buf + len, "1F-");
	}
	len += 3;
	if (open_test_result) {
		sprintf(buf + len, "2P-");
	}
	else {
		sprintf(buf + len, "2F-");
	}
	len += 3;
	if (short_test_result) {
		sprintf(buf + len, "3P-");
	}
	else {
		sprintf(buf + len, "3F-");
	}
	len += 3;
	if (allnode_test_result) {
		sprintf(buf + len, "4P-");
	}
	else {
		sprintf(buf + len, "4F-");
	}
	len += 3;
	if (allnode_self_test_result) {
		sprintf(buf + len, "5P-");
	}
	else {
		sprintf(buf + len, "5F-");
	}
	len += 3;
	sprintf(buf + len, "\n");
	len += 1;
	j = 0;
	i = 0;
	#if 0
	for (j = 0; j < i2c.y_ch * i2c.x_ch; j++) {
		printk("%4d, ", open_data[j]);
		if ((j + 1) % i2c.x_ch == 0) {
			printk("\n");
		}
	}
	printk("\n");
	i = 0;
	for (j = 0; j < i2c.y_ch * i2c.x_ch; j++) {
		sprintf(buf + len, "%4d,", open_data[j]);
		len += 5;
		i++;
		if((i) % i2c.x_ch == 0) {
			sprintf(buf + len, "\n");
			len += 1;
		}
	}
	sprintf(buf + len, "\n");
	len += 1;
	printk("ilitek_open_test len = %d\n", len);
	#endif
	#if 0
	for (j = 0; j < i2c.x_ch; j++) {
		printk("%4d, ", x1_data[j]);
	}
	printk("\n");
	for (j = 0; j < i2c.x_ch; j++) {
		printk("%4d, ", x2_data[j]);
	}
	printk("\n");
	#endif
	sprintf(buf + len, "***open***\n");
	len += 11;
	for (j = 0; j < i2c.x_ch; j++) {
		sprintf(buf + len, "%3d,", x1_data[j]);
		len += 4;
	}
	sprintf(buf + len, "\n");
	len += 1;
	for (j = 0; j < i2c.x_ch; j++) {
		sprintf(buf + len, "%3d,", x2_data[j]);
		len += 4;
	}
	sprintf(buf + len, "\n");
	len += 1;
	sprintf(buf + len, "\n");
	len += 1;
	i = 0;
	printk("ilitek_short_test len = %d\n", len);
	#if 0
	for (j = 0; j < i2c.y_ch * i2c.x_ch; j++) {
		printk("%4d, ", allnode_data[j]);
		i++;
		if((i) % i2c.x_ch == 0) {
			printk("\n");
		}
	}
	#endif
	#if 1
	i = 0;
	sprintf(buf + len, "***allnode***\n");
	len += 14;
	for (j = 0; j < i2c.y_ch * i2c.x_ch; j++) {
		sprintf(buf + len, "%4d,", allnode_data[j]);
		len += 5;
		i++;
		if((i) % i2c.x_ch == 0) {
			sprintf(buf + len, "\n");
			len += 1;
		}
	}
	sprintf(buf + len, "\n");
	len += 1;
	printk("ilitek_allnode_test len = %d\n", len);
	#if 0
	for (j = 0; j < i2c.x_ch; j++) {
		printk("%4d, ", allnode_self_data[j]);
	}
	printk("\n");
	#endif
	sprintf(buf + len, "***allnode_self***\n");
	len += 19;
	for (j = 0; j < i2c.x_ch; j++) {
		sprintf(buf + len, "%4d,", allnode_self_data[j]);
		len += 5;
	}
	sprintf(buf + len, "\n");
	len += 1;
	printk("ilitek_allnode_self_test len = %d\n", len);
	#endif

	
	#endif
	if (buf_recv) {
		kfree(buf_recv);
		buf_recv = NULL;
	}
	if (x1_data) {
		kfree(x1_data);
		x1_data = NULL;
	}
	if (x2_data) {
		kfree(x2_data);
		x2_data = NULL;
	}
	if (open_data) {
		kfree(open_data);
		open_data = NULL;
	}
	if (allnode_data) {
		kfree(allnode_data);
		allnode_data = NULL;
	}
	if (allnode_self_data) {
		kfree(allnode_self_data);
		allnode_self_data = NULL;
	}
	#endif
	#if 1
	while(1) {
		sprintf(buf + len, "%4d,", i++);
		len += 5;
		if (i % 15 == 0) {
			sprintf(buf + len, "\n");
			len += 1;
		}
		if (len > 7291) {
			break;
		}
	}
	sprintf(buf + len, "\n");
	len += 1;
	#endif
	printk("ilitek 5000 len = %d\n", len);
	return len;
}

static DEVICE_ATTR(ilitek_mmi_test, S_IRUGO, ilitek_all_test, NULL);

static ssize_t ilitek_open_test_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	ret = kstrtoint(buf, 10, &opentestthrehold);
	printk("ilitek_open_test_store opentestthrehold = %d\n", opentestthrehold);
	if (ret) {
		printk("ilitek_open_test_store kstrtoint error\n");
		return ret;
	}
	return size;
}
#if 1
static ssize_t set_4cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "ilitek sent 4 bytes data to ic\n");
}
static ssize_t set_4cmd_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{	
	int ret = 0;
	uint8_t cmd[4] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 4, .buf = cmd,},
	};
	//printk("ilitek set_4cmd_store len = %d\n", size);
	#if 0
	for (ret = 0; ret < (size + 1) / 3 - 1) {
		sscanf(buf, "%02x:", (int *)&cmd[ret]);
		printk("ilitek 0x%x ", cmd[ret]);
	}
	sscanf(buf, "%02x\n", (int *)&cmd[ret]);
	printk("ilitek 0x%x \n", cmd[ret]);
	#endif
	if (sscanf(buf, "%02X:%02X:%02X:%02X\n", (unsigned int *)&cmd[0], (unsigned int *)&cmd[1], (unsigned int *)&cmd[2], (unsigned int *)&cmd[3]) != 4){
		printk("ilitek cmd format error\n");
		return -EINVAL;
	}
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return size;
}
static DEVICE_ATTR(ilitek_set_4cmd, S_IRWXUGO, set_4cmd_show, set_4cmd_store);
static ssize_t set_3cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "ilitek sent 3 bytes data to ic\n");
}

static ssize_t set_3cmd_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{	
	int ret = 0;
	uint8_t cmd[4] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 3, .buf = cmd,},
	};
	if (sscanf(buf, "%02x:%02x:%02x\n", (unsigned int *)&cmd[0], (unsigned int *)&cmd[1], (unsigned int *)&cmd[2]) != 3){
		printk("ilitek cmd format error\n");
		//return -EINVAL;
	}
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return size;
}
static DEVICE_ATTR(ilitek_set_3cmd, S_IRWXUGO, set_3cmd_show, set_3cmd_store);
static ssize_t set_2cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "ilitek sent 2 bytes data to ic\n");
}

static ssize_t set_2cmd_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{	
	int ret = 0;
	uint8_t cmd[4] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	if (sscanf(buf, "%02x:%02x\n", (unsigned int *)&cmd[0], (unsigned int *)&cmd[1]) != 2){
		printk("ilitek cmd format error\n");
		return -EINVAL;
	}
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return size;
}
static DEVICE_ATTR(ilitek_set_2cmd, S_IRWXUGO, set_2cmd_show, set_2cmd_store);
#endif
static struct attribute *ilitek_sysfs_attrs_ctrl[] = {
	&dev_attr_ilitek_open_test.attr,
	//&dev_attr_open_test_set.attr,
	&dev_attr_ilitek_short_test.attr,
	//&dev_attr_short_test_set.attr,
	&dev_attr_ilitek_allnode_test.attr,
	//&dev_attr_allnode_test_set.attr,
	&dev_attr_ilitek_mmi_test.attr,
	#if 1
	&dev_attr_ilitek_set_4cmd.attr,
	&dev_attr_ilitek_set_3cmd.attr,
	&dev_attr_ilitek_set_2cmd.attr,
	#endif
	NULL
};
static struct attribute_group ilitek_attribute_group[] = {
	{.attrs = ilitek_sysfs_attrs_ctrl },
};

static struct kobj_attribute ilitek_open_test_set = {
	.attr = {.name = "ilitek_open_test_set", .mode = 0664},
	.show = NULL,
	.store = ilitek_open_test_store,
};
#if 0
static ssize_t ilitek_short_test_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	ret = kstrtoint(buf, 10, &microshorttestwindow);
	printk("ilitek_short_test_store opentestthrehold = %d\n", microshorttestwindow);
	if (ret) {
		printk("ilitek_short_test_store kstrtoint error\n");
		return ret;
	}
	return count;
}
static DEVICE_ATTR(short_test_set, S_IWUGO, NULL, ilitek_short_test_store);
#endif

void ilitek_sensor_test_init(void)
{
	int ret ;
	if (!ilitek_android_touch_kobj) {
		ilitek_android_touch_kobj = kobject_create_and_add("touchscreen", NULL) ;
		if (ilitek_android_touch_kobj == NULL) {
			printk(KERN_ERR "[ilitek]%s: kobject_create_and_add failed\n", __func__);
			return;
		}
	}
	ret = sysfs_create_group(ilitek_android_touch_kobj, ilitek_attribute_group);
	if (ret < 0) {
		printk(KERN_ERR "[ilitek]%s: sysfs_create_group failed\n", __func__);
	}
	printk("ilitek_open_test_set\n");
	ret = sysfs_create_file(ilitek_android_touch_kobj, &ilitek_open_test_set.attr);
	if (ret < 0) {
		printk(KERN_ERR "[ilitek]%s: sysfs_create_group failed\n", __func__);
	}
	return;
}

void ilitek_sensor_test_deinit(void)
{
	if (ilitek_android_touch_kobj) {
		sysfs_remove_group(ilitek_android_touch_kobj, ilitek_attribute_group);
		kobject_del(ilitek_android_touch_kobj);
	}
}

#endif
#endif

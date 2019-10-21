#include "ilitek_ts.h"

#if defined HALL_CHECK
extern struct i2c_data i2c;

int ilitek_into_hall_halfmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	cmd[0] = 0x0C;
	cmd[1] = 0x00;
	printk("ilitek_into_hall_halfmode\n");
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
int ilitek_into_hall_normalmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	cmd[0] = 0x0C;
	cmd[1] = 0x01;
	printk("ilitek_into_hall_normalmode\n");
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
#if HALL_CHECK == HALL_CHECK_OTHER
	//define it for 300ms timer
#define HALL_TIMROUT_PERIOD 300 

	//define global variables
	 struct timer_list ilitek_hall_check_timer;
	 int curr_hall_state= 1;//0
	 int prev_hall_state= 1;//0
	 void ilitek_hall_check_init(void);
	 int ilitek_hall_check_work_func(void);

static int hall_gpio = 132;
//declare all the hall check function here
void ilitek_hall_check_init(void);
void ilitek_hall_check_callback(unsigned long data);
int ilitek_hall_check_work_func(void);

//when the hall check timer is timeout, system will call this function. 
void ilitek_hall_check_callback(unsigned long data)
{
    printk("[ilitek]:%s check mode enter\n", __func__);
    
    curr_hall_state = gpio_get_value(hall_gpio);
    if(curr_hall_state != prev_hall_state)
    {
    	#if 0
        //active workqueue here
        tpd_flag = 1;
        wake_up_interruptible(&waiter);
		#endif
		ilitek_hall_check_work_func();
    }
    
    ilitek_hall_check_timer.expires = jiffies + msecs_to_jiffies(HALL_TIMROUT_PERIOD);
    add_timer(&ilitek_hall_check_timer);
}

//init the hall check timer 
void ilitek_hall_check_init(void)
{
	//return ;
	#if 1
        init_timer(&ilitek_hall_check_timer);
        ilitek_hall_check_timer.function = ilitek_hall_check_callback;
        ilitek_hall_check_timer.expires = jiffies + msecs_to_jiffies(HALL_TIMROUT_PERIOD);
        add_timer(&ilitek_hall_check_timer);
		#endif
}

//when the hall state is changed ,call this function to send the cmd
int ilitek_hall_check_work_func(void)
{
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s,	err, ret %d\n", __func__, ret);
		return ret;
	}
    printk("[ilitek]:%s change mode enter!\n", __func__);
    prev_hall_state = curr_hall_state;
    if(curr_hall_state)
    {
        //normal mode
		ilitek_into_hall_normalmode();
    }
    else
    {
        //half mode 
		ilitek_into_hall_halfmode();
    }
	return 0;
}
#elif HALL_CHECK == HALL_CHECK_HW

int tpd_sensitivity_status = 0;
static struct kobject *ilitek_android_touch_kobj = NULL;

static ssize_t ilitek_tpd_sensitivity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "tpd_sensitivity_status = %d\n", tpd_sensitivity_status);
}
static ssize_t ilitek_tpd_sensitivity_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	int ret = 0;
	ret = kstrtoint(buf, 10, &tpd_sensitivity_status);
	printk("ilitek_tpd_sensitivity_store tpd_sensitivity_status = %d\n", tpd_sensitivity_status);
	if (ret) {
		printk("ilitek_tpd_sensitivity_store kstrtoint error\n");
		return ret;
	}
    if(tpd_sensitivity_status)
    {
        //normal mode
		ilitek_into_hall_normalmode();
    }
    else
    {
        //half mode 
		ilitek_into_hall_halfmode();
    }
	return size;
}

static DEVICE_ATTR(ilitek_tpd_sensitivity, S_IRWXUGO, ilitek_tpd_sensitivity_show, ilitek_tpd_sensitivity_store);

static struct attribute *ilitek_sysfs_attrs_ctrl[] = {
	&dev_attr_ilitek_tpd_sensitivity.attr,
	NULL
};
static struct attribute_group ilitek_attribute_group[] = {
	{.attrs = ilitek_sysfs_attrs_ctrl },
};

void ilitek_hall_check_hw_init(void)
{
	int ret = 0;
	if (!ilitek_android_touch_kobj) {
		ilitek_android_touch_kobj = kobject_create_and_add("tpd_sensitivity", NULL) ;
		if (ilitek_android_touch_kobj == NULL) {
			printk(KERN_ERR "[ilitek]%s: kobject_create_and_add failed\n", __func__);
			//return;
		}
	}
	ret = sysfs_create_group(ilitek_android_touch_kobj, ilitek_attribute_group);
	if (ret < 0) {
		printk(KERN_ERR "[ilitek]%s: sysfs_create_group failed\n", __func__);
	}
	return;
}

void ilitek_hall_check_hw_deinit(void)
{
	if (ilitek_android_touch_kobj) {
		sysfs_remove_group(ilitek_android_touch_kobj, ilitek_attribute_group);
		kobject_del(ilitek_android_touch_kobj);
	}
}


#endif

#endif



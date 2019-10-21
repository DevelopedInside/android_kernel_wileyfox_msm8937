#include "ilitek_ts.h"
#include <linux/firmware.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_HY_DRV_ASSIST
#include <linux/hy-assist.h>
#endif

#if !IC2120
static bool serial23 = false;
#endif
#ifdef GESTURE
int ilitek_system_resume = 1;
#include "ilitek_gesture.h"
int gesture_count,getstatus;	
#endif

#ifdef CLOCK_INTERRUPT
#ifdef REPORT_THREAD
	static DECLARE_WAIT_QUEUE_HEAD(waiter);
	static int tpd_flag = 0;
#endif
#endif

#ifdef DEBUG_NETLINK
bool debug_flag = false;
struct sock *netlink_sock;
static kuid_t	uid;
static int pid = 100, seq = 23/*, sid*/;
#endif

#ifdef ILI_UPDATE_FW
	#ifdef UPDATE_THREADE
	static int update_wait_flag = 0;
	#endif
	extern unsigned char CTPM_FW[];
	extern bool driver_upgrade_flag;
	extern int ilitek_upgrade_firmware(void);
#endif


#ifdef TOOL
	// device data
	struct dev_data {
		// device number
		dev_t devno;
		// character device
		struct cdev cdev;
		// class device
		struct class *class;
	};

	extern struct dev_data dev_ilitek;
	extern struct proc_dir_entry * proc_ilitek;
	extern int create_tool_node(void);
	extern int remove_tool_node(void);
#endif

#ifdef SENSOR_TEST
#define SYS_ATTR_FILE
#ifdef SYS_ATTR_FILE
	//extern struct kobject *android_touch_kobj;
	extern void ilitek_sensor_test_deinit(void);
	extern void ilitek_sensor_test_init(void);
#endif
#endif
#if defined HALL_CHECK
extern int ilitek_into_hall_halfmode(void);
#if HALL_CHECK == HALL_CHECK_OTHER
	//define it for 300ms timer
#define HALL_TIMROUT_PERIOD 300 

	//define global variables
	extern struct timer_list ilitek_hall_check_timer;
	extern int curr_hall_state;//0
	extern int prev_hall_state;//0
	extern void ilitek_hall_check_init(void);
	extern int ilitek_hall_check_work_func(void);
#elif HALL_CHECK == HALL_CHECK_HW
extern int tpd_sensitivity_status;
extern void ilitek_hall_check_hw_init(void);
extern void ilitek_hall_check_hw_deinit(void);

#endif

#endif

#ifdef ILITEK_ESD_CHECK
	 static struct workqueue_struct *esd_wq = NULL;
	 static struct delayed_work esd_work;
	 static atomic_t ilitek_cmd_response = ATOMIC_INIT(0);
	 static unsigned long delay = 2*HZ;
	 static int ilitek_i2c_esd_check(struct i2c_client *client, uint8_t *data, int length);
#endif
int out_use = 0;

int touch_key_hold_press = 0;
int touch_key_code[] = {KEY_MENU,KEY_HOMEPAGE,KEY_BACK,KEY_VOLUMEDOWN,KEY_VOLUMEUP};
int touch_key_press[] = {0, 0, 0, 0, 0};
unsigned long touch_time=0;
int driver_information[] = {DERVER_VERSION_MAJOR,DERVER_VERSION_MINOR,CUSTOMER_ID,MODULE_ID,PLATFORM_ID,PLATFORM_MODULE,ENGINEER_ID};
char Report_Flag;

volatile char int_Flag;
volatile char update_Flag;
int update_timeout;


char EXCHANG_XY = 0;
char REVERT_X = 0;
char REVERT_Y = 0;
char DBG_FLAG = 0,DBG_COR;
//#define ROTATE_FLAG

struct i2c_data i2c;
static void ilitek_set_input_param(struct input_dev*, int max_tp, int max_x, int max_y);
static int ilitek_i2c_process_and_report(void);
static int ilitek_i2c_probe(struct i2c_client*, const struct i2c_device_id*);
static int ilitek_i2c_remove(struct i2c_client*);
static int ilitek_i2c_polling_thread(void*);
static irqreturn_t ilitek_i2c_isr(int, void*);
static void ilitek_i2c_irq_work_queue_func(struct work_struct*);
static int Request_IRQ(void);
static int ilitek_set_irq(void);
static int ilitek_config_irq(void);
static int ilitek_should_load_driver(void);
static int ilitek_register_prepare(void);
static int ilitek_request_init_reset(void);
static int ilitek_init(void);
static void ilitek_exit(void);

// i2c id table
static const struct i2c_device_id ilitek_i2c_id[] ={
	{ILITEK_I2C_DRIVER_NAME, 0}, {}
};
MODULE_DEVICE_TABLE(i2c, ilitek_i2c_id);

// declare i2c function table
static struct of_device_id ilitek_match_table[] = {
	{ .compatible = "tchip,ilitek",},
	{ .compatible = "ilitek,2120",},
	{ .compatible = "ilitek,2139",},
	{ .compatible = "ilitek,2839",},
	{ .compatible = "ilitek,2113",},
	{ .compatible = "ilitek,2115",},
	{ .compatible = "ilitek,2116",},
	{ .compatible = "ilitek,2656",},
	{ .compatible = "ilitek,2645",},
	{ .compatible = "ilitek,2302",},
	{ .compatible = "ilitek,2303",},
	{ .compatible = "ilitek,2102",},
	{ .compatible = "ilitek,2111",},
};

static struct i2c_driver ilitek_ts_driver = {
	.probe = ilitek_i2c_probe,
	.remove = ilitek_i2c_remove,
	.driver = {
		.name = "ilitek_i2c",
		.owner = THIS_MODULE,
		.of_match_table = ilitek_match_table,
	},
	.id_table = ilitek_i2c_id,
};

#ifdef CONFIG_HY_DRV_ASSIST
static ssize_t ilitek_ic_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n","Ilitek.ili2120");
}
static ssize_t ilitek_fw_ver_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d.%d.%d.%d\n", i2c.firmware_ver[0],i2c.firmware_ver[1],i2c.firmware_ver[2],i2c.firmware_ver[3]);
}
static ssize_t ilitek_tp_vendor_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d.%d.%d.%d\n", i2c.firmware_ver[0],i2c.firmware_ver[1],i2c.firmware_ver[2],i2c.firmware_ver[3]);
}
#endif


#ifdef DEBUG_NETLINK
//bool debug_flag = false;
//static struct sock *netlink_sock;

static void udp_reply(int pid,int seq,void *payload,int size)
{
	struct sk_buff	*skb;
	struct nlmsghdr	*nlh;
	int		len = NLMSG_SPACE(size);
	void		*data;
	int ret;
	skb = alloc_skb(len, GFP_ATOMIC);
	if (!skb)
		return;
	printk("ilitek udp_reply\n");
	nlh= nlmsg_put(skb, pid, seq, 0, size, 0);
	nlh->nlmsg_flags = 0;
	data=NLMSG_DATA(nlh);
	memcpy(data, payload, size);
	NETLINK_CB(skb).portid = 0;         /* from kernel */
	NETLINK_CB(skb).dst_group = 0;  /* unicast */
	ret=netlink_unicast(netlink_sock, skb, pid, MSG_DONTWAIT);
	if (ret <0)
	{
		printk("ilitek send failed\n");
		return;
	}
	return;

}

/* Receive messages from netlink socket. */

static void udp_receive(struct sk_buff  *skb)
{
	void			*data;
	uint8_t buf[64] = {0};
	struct nlmsghdr *nlh;

	nlh = (struct nlmsghdr *)skb->data;
	pid  = 100;//NETLINK_CREDS(skb)->pid;
	uid  = NETLINK_CREDS(skb)->uid;
	//sid  = NETLINK_CB(skb).sid;
	seq  = 23;//nlh->nlmsg_seq;
	data = NLMSG_DATA(nlh);
	//printk("recv skb from user space uid:%d pid:%d seq:%d,sid:%d\n",uid,pid,seq,sid);
	if(!strcmp(data,"Hello World!"))
	{
		printk("recv skb from user space pid:%d seq:%d\n",pid,seq);
		printk("data is :%s\n",(char *)data);
		udp_reply(pid,seq,data,sizeof("Hello World!"));
		//udp_reply(pid,seq,data);
	}
	else
	{
		memcpy(buf,data,64);
	}
	//kfree(data);
	return ;
}
#endif

void ilitek_reset(int reset_gpio)
{
	printk("Enter 10 ilitek_reset\n");

	gpio_direction_output(reset_gpio,1);
	//msleep(50);
	mdelay(10);
	gpio_direction_output(reset_gpio,0);
	//msleep(50);
	mdelay(10);
	gpio_direction_output(reset_gpio,1);
	//msleep(100);
	mdelay(10);
	return;
}

/*
   for compatible purpose
   judge if driver should loading and do some previous work,
   if do not loading driver,return value should < 0
 */
static int ilitek_should_load_driver(void)
{
	//add judge here
	return 0;
}

/*
   do some previous work depends on platform before add i2c driver,
   if return value  < 0,driver will not register,
   if return value  >= 0,conrinue register work
 */
static int ilitek_register_prepare(void)
{
	//if necessary,add work here
	return 0;
}

/*
   request reset gpio and reset tp,
   return value < 0 means fail
 */

static int ilitek_request_init_reset(void)
{
	s32 ret=0;


	ilitek_reset(i2c.reset_gpio);
	return ret;
}

static int Request_IRQ(void)
{
	int ret = 0;

	ret = request_irq(i2c.client->irq, ilitek_i2c_isr, IRQF_TRIGGER_LOW | IRQF_ONESHOT, "ilitek_i2c_irq", &i2c);
	return ret;
}

/*
   set the value of i2c.client->irq,could add some other work about irq here
   return value < 0 means fail

 */
static int ilitek_set_irq(void)
{
	return 0;
}

/*
   irq here
   return value < 0 means fail

 */
static int ilitek_config_irq(void)
{
	return 0;
}

void ilitek_set_finish_init_flag(void)
{
	return;
}

/*
   description
   set input device's parameter
   prarmeters
   input
   input device data
   max_tp
   single touch or multi touch
   max_x
   maximum	x value
   max_y
   maximum y value
   return
   nothing
 */
static void ilitek_set_input_param(struct input_dev *input,	int max_tp, int max_x, int max_y)
{
#if 0
	int key;
#endif

	__set_bit(INPUT_PROP_DIRECT, input->propbit);
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	#ifndef ROTATE_FLAG
		input_set_abs_params(input, ABS_MT_POSITION_X, 0, max_x, 0, 0);
		input_set_abs_params(input, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	#else
		input_set_abs_params(input, ABS_MT_POSITION_X, 0, max_y, 0, 0);
		input_set_abs_params(input, ABS_MT_POSITION_Y, 0, max_x, 0, 0);
	#endif
		input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
		input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);

#ifdef TOUCH_PROTOCOL_B
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
		input_mt_init_slots(input, max_tp, INPUT_MT_DIRECT);
	#else
		input_mt_init_slots(input, max_tp);
	#endif
#else
		input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, max_tp, 0, 0);
#endif
#ifdef REPORT_PRESSURE
	input_set_abs_params(input, ABS_MT_PRESSURE, 0, 255, 0, 0);
#endif
#if 0
	for(key=0; key<sizeof(touch_key_code) / sizeof(touch_key_code[0]); key++){
			if(touch_key_code[key] <= 0){
					continue;
		}
			set_bit(touch_key_code[key] & KEY_MAX, input->keybit);
	}
#endif
	input->name = ILITEK_I2C_DRIVER_NAME;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &(i2c.client)->dev;
}


int ilitek_poll_int(void) 
{
	return gpio_get_value(i2c.irq_gpio);
}
/*
   description
   send message to i2c adaptor
   parameter
   client
   i2c client
   msgs
   i2c message
   cnt
   i2c message count
   return
   >= 0 if success
   others if error
 */
int ilitek_i2c_transfer(struct i2c_client *client, struct i2c_msg *msgs, int cnt)
{
	int ret, count=ILITEK_I2C_RETRY_COUNT;
	#if 0
	for (ret = 0; ret < cnt; ret++) {
		msgs[ret].scl_rate = 400000;
	}
	#endif
	ret = 0;
	while(count >= 0){
		count-= 1;
		ret = down_interruptible(&i2c.wr_sem);
		ret = i2c_transfer(client->adapter, msgs, cnt);
		up(&i2c.wr_sem);
		if(ret < 0){
			msleep(500);
			continue;
		}
		break;
	}
	return ret;
}

int ilitek_i2c_write(struct i2c_client *client, uint8_t * cmd, int length)
{
	int ret;
	struct i2c_msg msgs[] = {
		{.addr = client->addr, .flags = 0, .len = length, .buf = cmd,}
	};

	ret = ilitek_i2c_transfer(client, msgs, 1);
	if(ret < 0)
	{
		printk(ILITEK_ERROR_LEVEL "%s, i2c write error, ret %d\n", __func__, ret);
	}
	return ret;
}

int ilitek_i2c_read(struct i2c_client *client, uint8_t *data, int length)
{
	int ret;

	struct i2c_msg msgs_ret[] = {
		{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = data,}
	};


	ret = ilitek_i2c_transfer(client, msgs_ret, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d\n", __func__, ret);
	}

	return ret;
}

int ilitek_i2c_write_and_read(struct i2c_client *client, uint8_t *cmd,
		int write_len, int delay, uint8_t *data, int read_len)
{
	int ret = 0;
	struct i2c_msg msgs_send[] = {
		{.addr = client->addr, .flags = 0, .len = write_len, .buf = cmd,},
		{.addr = client->addr, .flags = I2C_M_RD, .len = read_len, .buf = data,}
	};
	struct i2c_msg msgs_receive[] = {
		{.addr = client->addr, .flags = I2C_M_RD, .len = read_len, .buf = data,}
	};
	#if 1
	if (read_len == 0) {
		if (write_len > 0) {
			ret = ilitek_i2c_transfer(client, msgs_send, 1);
			if(ret < 0)
			{
				printk(ILITEK_ERROR_LEVEL "%s, i2c write error, ret = %d\n", __func__, ret);
			}
		}
		if(delay > 1)
			msleep(delay);
	}
	else if (write_len == 0) {
		if(read_len > 0){
			ret = ilitek_i2c_transfer(client, msgs_receive, 1);
			if(ret < 0)
			{
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret = %d\n", __func__, ret);
			}
		}
	}
	else if (delay > 0) {
		if (write_len > 0) {
			ret = ilitek_i2c_transfer(client, msgs_send, 1);
			if(ret < 0)
			{
				printk(ILITEK_ERROR_LEVEL "%s, i2c write error, ret = %d\n", __func__, ret);
			}
		}
		if(delay > 1)
			msleep(delay);
		if(read_len > 0){
			ret = ilitek_i2c_transfer(client, msgs_receive, 1);
			if(ret < 0)
			{
				printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret = %d\n", __func__, ret);
			}
		}
	}
	else {
		//printk("ilitek restart restart\n");
		ret = ilitek_i2c_transfer(client, msgs_send, 2);
		if(ret < 0)
		{
			printk(ILITEK_ERROR_LEVEL "%s, i2c write error, ret = %d\n", __func__, ret);
		}
	}
	#else
	if (write_len > 0) {
		ret = ilitek_i2c_transfer(client, msgs_send, 1);
		if(ret < 0)
		{
			printk(ILITEK_ERROR_LEVEL "%s, i2c write error, ret = %d\n", __func__, ret);
		}
	}
	if(delay > 1)
		msleep(delay);
	if(read_len > 0){
		ret = ilitek_i2c_transfer(client, msgs_receive, 1);
		if(ret < 0)
		{
			printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret = %d\n", __func__, ret);
		}
	}
	#endif
	return ret;
}

#ifdef ILITEK_ESD_CHECK
/*
description
	 read data from i2c device with delay between cmd & return data
parameter
	 client
		 i2c client data
	 addr
		 i2c address
	 data
		 data for transmission
	 length
		 data length
return
	 status
*/
static int 
ilitek_i2c_esd_check(struct i2c_client *client, uint8_t *data, int length)
{
	int ret;
	char buf[4] = {0};
	printk("iic address =%x \n",client->addr);
	if (out_use == 0) {
		//printk(" %s out_use = %d\n", __func__, out_use);
		buf[0] = ILITEK_TP_CMD_ESD_CHECK;
		//buf[0] = 0x42;
		ret = ilitek_i2c_write(i2c.client, buf, 1);
		if(ret < 0){
			//printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %dn", __func__, ret);
			printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d,addr %x \n", __func__, ret,client->addr);
			return ret;
		}
	}
	else {
		data[0] = 0x02;
		printk("%s APK USE SO not check out_use = %d\n", __func__, out_use);
		return 0;
	}
	msleep(10);
	if (out_use == 0) {
		//printk(" %s out_use = %d\n", __func__, out_use);
		ret = ilitek_i2c_read(i2c.client, data, length);
		if(ret < 0){
			//printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %dn", __func__, ret);
			printk(ILITEK_ERROR_LEVEL "%s, i2c read error, ret %d,addr %x \n", __func__, ret,client->addr);
			return ret;
		}
	}
	else {
		data[0] = 0x02;
		printk("%s APK USE SO not check out_use = %d\n", __func__, out_use);
		return 0;
	}
	 return ret;
}

static void ilitek_touch_esd_func(struct work_struct *work)
{	
	int i = 0;	
	unsigned char buf[4]={0};

	printk("esd %s: enter.......\n", __func__);
	
	if(out_use == 1){
		printk("ilitek esd %s APK USE SO not check\n", __func__);
		goto ilitek_esd_check_out;
	}
	
	if(atomic_read(&ilitek_cmd_response) == 0){
		for (i = 0; i < 3; i++) {
			if(ilitek_i2c_esd_check(i2c.client, buf, 2) < 0){
				printk("ilitek esd %s: ilitek_i2c_read_info i2c communication error \n", __func__);
				if ( i == 2) {
					printk("esd %s: ilitek_i2c_read_info , i2c communication failed three times reset now\n", __func__);
					break;
				}
			}
			else {
				if (buf[0] == 0x02) {
					printk("esd %s: ilitek_ts_send_cmd successful, response ok\n", __func__);
						goto ilitek_esd_check_out;
				}
				else {
					if ( i == 2) {
						printk("esd %s: ilitek_ts_send_cmd successful, response failed three times reset now\n", __func__);
						break;
					}
					printk("esd %s: ilitek_ts_send_cmd successful, response failed\n", __func__);
				}
			}
		}
	}
	else{
		printk("esd %s: have interrupt so not check!!!\n", __func__);
		goto ilitek_esd_check_out;
	}
	
	ilitek_reset(i2c.reset_gpio);
ilitek_esd_check_out:	
	
	atomic_set(&ilitek_cmd_response, 0);
	queue_delayed_work(esd_wq, &esd_work, delay);
	printk("ilitek esd %s: out....... i = %d\n", __func__, i);	
	return;
}

#endif

static int ilitek_touch_down(int id, int x, int y, int pressure) {
#ifdef TOUCH_PROTOCOL_B
	input_mt_slot(i2c.input_dev, id);
	input_mt_report_slot_state(i2c.input_dev, MT_TOOL_FINGER, true);
	//i2c.touchinfo[id].flag = 1;
#endif
	input_event(i2c.input_dev, EV_ABS, ABS_MT_POSITION_X, x);
	input_event(i2c.input_dev, EV_ABS, ABS_MT_POSITION_Y, y);
	input_event(i2c.input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
#ifdef REPORT_PRESSURE
	//printk("ilitek_touch_down pressure = %d\n", pressure);
	input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, pressure);
#endif
#ifndef TOUCH_PROTOCOL_B
	input_event(i2c.input_dev, EV_ABS, ABS_MT_TRACKING_ID, id);
	input_report_key(i2c.input_dev, BTN_TOUCH, 1);
	input_mt_sync(i2c.input_dev);
	i2c.touch_flag = 1;
#endif
	i2c.release_flag[id] = 1;
	return 0;
}

static int ilitek_touch_release(int id) {
#ifdef TOUCH_PROTOCOL_B
	if(i2c.release_flag[id] == 1)
	{
		DBG("release point okokok id = %d\n", id);
	#ifdef REPORT_PRESSURE
		input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
	#endif
		input_mt_slot(i2c.input_dev, id);
		input_mt_report_slot_state(i2c.input_dev, MT_TOOL_FINGER, false);
		i2c.release_flag[id] = 0;
	}
#else
#ifdef REPORT_PRESSURE
	input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
#endif
	input_report_key(i2c.input_dev, BTN_TOUCH, 0);
	input_mt_sync(i2c.input_dev);
	i2c.touch_flag = 0;
#endif
	return 0;
}

static int ilitek_touch_release_all_point(int sync) {
	int i = 0;
#ifdef TOUCH_PROTOCOL_B
	for(i = 0; i < i2c.max_tp; i++)
	{
		ilitek_touch_release(i);
	}
#else
	if(i2c.touch_flag == 1)
	{	
		i2c.touch_flag == 0;
		for(i = 0; i < i2c.max_tp; i++)
		{
			i2c.release_flag[i] = 0;
		}
		ilitek_touch_release(i);
	}
#endif
	if (sync) {
		#ifdef TOUCH_PROTOCOL_B
		input_mt_report_pointer_emulation(i2c.input_dev, true);
		#endif
		input_sync(i2c.input_dev);
	}
	return 0;
}
		
#ifdef GESTURE
#if GESTURE == GESTURE_DRIVER
static int ilitek_gesture_check(void) {
	struct input_dev *input = i2c.input_dev;
	int gesture_mode=0;
	int keycode=0;
	int gesture_flag =0;
#ifdef GESTURE
#if GESTURE == GESTURE_DRIVER
	//		release_counter++;					
	if(ilitek_system_resume == 0)
	{
		gesture_count = 0;
		gesture_flag =0;
		getstatus = 0;
		
#if GESTURE_FUN == GESTURE_FUN_1
		gesture_mode = ilitek_GestureMatchProcess(0,gesture_count,0,0);
		getstatus = ilitek_GetGesture();
#endif
		switch(getstatus)
		{
		  case 'l':
					gesture_flag =1;
					keycode = KEY_LEFT;
					DBG("ilitek Input Gesture 'left'\n");

													break;
		  case 'r':
					gesture_flag = 1;
					keycode = KEY_RIGHT;
					DBG("ilitek Input Gesture 'right'\n");
													break;
		  case 'u':
					gesture_flag = 1;
					keycode = KEY_UP;
					DBG("ilitek Input Gesture 'up'\n");
													break;
		  case 'd':
					gesture_flag = 1;
					keycode = KEY_DOWN;
					DBG("ilitek Input Gesture 'down'\n");
								 break;
		  case 'o':
					gesture_flag = 1;
					keycode = KEY_O;
					DBG("ilitek Input Gesture 'o'\n");

													break;
		  case 'w':
					gesture_flag = 1;
					keycode = KEY_W;
					DBG("ilitek Input Gesture 'w'\n");

													break;
		  case 'm':
					gesture_flag = 1;
					keycode = KEY_M;
					DBG("ilitek Input Gesture 'M'\n");

													break;
													
		  case 'e':
					gesture_flag = 1;
					keycode = KEY_E;
					DBG("ilitek Input Gesture 'E'\n");

													break;
		  case 'c':
					gesture_flag = 1;
					keycode = KEY_C;
					DBG("ilitek Input Gesture 'c'\n");

													break;	
		  case 'v':
					gesture_flag = 1;
					keycode = KEY_V;
					DBG("ilitek Input Gesture 'v'\n");

													break;	
		  case 's':
					gesture_flag = 1;
					keycode = KEY_S;
					DBG("ilitek Input Gesture 's'\n");

													break;	
		  case 'z':
					gesture_flag = 1;
					keycode = KEY_Z;
					DBG("ilitek Input Gesture 'z'\n");

													break;																						
		  default:	   break;
			
		   }
		if (gesture_flag == 1)
		{
			gesture_flag = 0;
			ilitek_system_resume = 1;
			input_report_key(input, keycode, 1);

			input_sync(input);
			msleep(10);
			input_report_key(input, keycode, 0);
			input_sync(input);
#if 0
			printk("ilitek gesture wakeup\n");
			input_report_key(i2c.input_dev, KEY_POWER, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_POWER, 0);
			input_sync(i2c.input_dev);
#endif
		}

	   }	
#endif						
#endif						
	return 0;
}
#endif
#endif

#if IC2120
#ifndef NEW_PROTOCOL_2120
static int ilitek_report_data_2120_old(void) {
	unsigned char buf[64]={0};
	int ret = 0, len = 0, i = 0, x = 0, y = 0, tmp = 0, mult_tp_id = 0;
#ifdef REPORT_PRESSURE
	int pressure = 0;
#endif

#ifndef TOUCH_PROTOCOL_B
	int release_counter = 0;
#endif
	//read i2c data from device
	buf[0] = ILITEK_TP_CMD_READ_DATA;
	ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 0, buf, 3);
	if(ret < 0)
	{
		return ret;
	}
	len = buf[0];
	ret = 1;
	if(len>20)
	{
		return ret;
	}
	//read touch point
	for(i = 0; i < len; i++)
	{
		//parse point
		buf[0] = ILITEK_TP_CMD_READ_SUB_DATA;
		if(ilitek_i2c_write_and_read(i2c.client, buf, 1, 0, buf, 7))
		{
			x = (((int)buf[2]) << 8) + buf[1];
			y = (((int)buf[4]) << 8) + buf[3];
			if (EXCHANG_XY) {
				tmp = x;
				x = y;
				y = tmp;
			}
			if (REVERT_X) {
				x = i2c.max_x - x;
			}
				
			if (REVERT_Y) {
				y = i2c.max_y - y;
			}
		#ifdef REPORT_PRESSURE
				pressure = buf[5];
		#endif
			mult_tp_id = buf[0];
			switch ((mult_tp_id & 0xC0))
			{
				case TOUCH_POINT:
					{
						DBG("TOUCH_POINT  i = %d  (buf[0] & 0x3F) - 1 = %d\n", i, (buf[0] & 0x3F) - 1);
						//report to android system
						DBG("Point, ID=%02X, X=%04d, Y=%04d,touch_key_hold_press=%d\n",buf[0]  & 0x3F, x,y,touch_key_hold_press);
					#ifdef REPORT_PRESSURE
						ilitek_touch_down((buf[0] & 0x3F) - 1, x, y, pressure);
					#else
						ilitek_touch_down((buf[0] & 0x3F) - 1, x, y, 10);
					#endif
						ret = 0;
					}
					break;
				case RELEASE_POINT:
					DBG("RELEASE_POINT	i = %d	(buf[0] & 0x3F) - 1 = %d\n", i, (buf[0] & 0x3F) - 1);
					// release point
				#ifdef TOUCH_PROTOCOL_B
					ilitek_touch_release((buf[0] & 0x3F) - 1);
				#else
					release_counter++;
					if(release_counter == len)
					{
						for(i = 0; i < i2c.max_tp; i++)
						{
							i2c.release_flag[i] = 0;
						}
						ilitek_touch_release((buf[0] & 0x3F) - 1);
					}
				#endif
					break;
				default:
					break;
			}
		}
	}
	if(len == 0)
	{
	#ifdef TOUCH_PROTOCOL_B
		for(i = 0; i < i2c.max_tp; i++)
		{
			ilitek_touch_release(i);
		}
	#else
		if(i2c.touch_flag == 1)
		{	
			
			for(i = 0; i < i2c.max_tp; i++)
			{
				i2c.release_flag[i] = 0;
			}
			ilitek_touch_release(i);
		}
	#endif
	}
#ifdef TOUCH_PROTOCOL_B
	input_mt_report_pointer_emulation(i2c.input_dev, true);
#endif
	input_sync(i2c.input_dev);
	return 0;
}
#else
static int ilitek_report_data_2120_new(void) {
	unsigned char buf[64]={0};
	int ret = 0, len = 0, i = 0, x = 0, y = 0, tmp = 0, tp_status = 0;
#ifdef REPORT_PRESSURE
		int pressure = 0;
#endif
	
#ifndef TOUCH_PROTOCOL_B
		int release_counter = 0;
#endif
	buf[0] = ILITEK_TP_CMD_READ_DATA;
	ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 0, buf, 53);
#ifdef DEBUG_NETLINK
	if (debug_flag) {
		udp_reply(pid,seq,buf,sizeof(buf));
	}
	if (buf[1] == 0x5F) {
		return 0;
	}
#endif
	//printk("ilitek start\n");
	len = buf[0];
	
	if (ret < 0) {
		printk("ilitek ILITEK_TP_CMD_READ_DATA error return & release\n");
		ilitek_touch_release_all_point(1);
		return ret;
	}
	else {
		ret = 0;
	}
	len = buf[0];
	DBG("ilitek len = 0x%x buf[0] = 0x%x, buf[1] = 0x%x, buf[2] = 0x%x\n", len, buf[0], buf[1], buf[2]);
	if (len > 20) {
		printk("ilitek len > 20  return & release\n");
				ilitek_touch_release_all_point(1);
		return ret;
	}
#ifdef GESTURE
#if GESTURE == GESTURE_2120
	if (ilitek_system_resume == 0) {
		printk("ilitek gesture wake up 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2]);
		if (buf[2] == 0x60) {
			DBG("ilitek gesture wake up this is c\n");
			input_report_key(i2c.input_dev, KEY_C, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_C, 0);
			input_sync(i2c.input_dev);
		}
		if (buf[2] == 0x62) {
			DBG("ilitekgesture wake up this is e\n");
			input_report_key(i2c.input_dev, KEY_E, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_E, 0);
			input_sync(i2c.input_dev);
		}
		if (buf[2] == 0x64) {
			DBG("gesture wake up this is m\n");
			input_report_key(i2c.input_dev, KEY_M, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_M, 0);
			input_sync(i2c.input_dev);
		}
		if (buf[2] == 0x66) {
			DBG("ilitek gesture wake up this is w\n");
			input_report_key(i2c.input_dev, KEY_W, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_W, 0);
			input_sync(i2c.input_dev);
		}
		if (buf[2] == 0x68) {
			DBG("ilitek gesture wake up this is o\n");
			input_report_key(i2c.input_dev, KEY_O, 1);
			input_sync(i2c.input_dev);
			input_report_key(i2c.input_dev, KEY_O, 0);
			input_sync(i2c.input_dev);
		}
		input_report_key(i2c.input_dev, KEY_POWER, 1);
		input_sync(i2c.input_dev);
		input_report_key(i2c.input_dev, KEY_POWER, 0);
		input_sync(i2c.input_dev);
		return 0;
	}
#endif
#endif
	for(i = 0; i < i2c.max_tp; i++){
		tp_status = buf[i*5+3] >> 7;	
		x = (((int)(buf[i*5+3] & 0x3F) << 8) + buf[i*5+4]);
		y = (((int)(buf[i*5+5] & 0x3F) << 8) + buf[i*5+6]);
		if (EXCHANG_XY) {
			tmp = x;
			x = y;
			y = tmp;
		}
		if (REVERT_X) {
			x = i2c.max_x - x;
		}
			
		if (REVERT_Y) {
			y = i2c.max_y - y;
		}
	#ifdef REPORT_PRESSURE
		pressure = buf[i * 5 + 7];
	#endif
		if (tp_status) {
			DBG("ilitek TOUCH_POINT  i = %d x= %d  y=%d  \n", i,x,y);
			//report to android system
			//DBG("Point, ID=%02X, X=%04d, Y=%04d,touch_key_hold_press=%d\n",buf[0]  & 0x3F, x,y,touch_key_hold_press);
			DBG("i2c.keycount:%d,i2c.keyflag:%d\n",i2c.keycount,i2c.keyflag);
			#ifdef VIRTUAL_KEY_PAD
			if(i2c.keyflag == 0){
				DBG("key_x:%d,key_y:%d\n",x,y);
				if(x ==2250 && y ==2250)
				{
					input_report_key(i2c.input_dev, BTN_TOUCH, 1);
					input_report_key(i2c.input_dev,  KEY_MENU, 1);
					input_sync(i2c.input_dev);
					touch_key_hold_press = 1;
					i2c.keyflag =1;
					printk("key1\n");
				}
				else if(x ==3250 && y ==3250)
				{
					input_report_key(i2c.input_dev, BTN_TOUCH, 1);
					input_report_key(i2c.input_dev,  KEY_HOMEPAGE, 1);
					input_sync(i2c.input_dev);
					touch_key_hold_press = 1;
					i2c.keyflag =1;
				}
				else if(x ==4250 && y ==4250)
				{
					input_report_key(i2c.input_dev, BTN_TOUCH, 1);
					input_report_key(i2c.input_dev,  KEY_BACK, 1);
					input_sync(i2c.input_dev);
					touch_key_hold_press = 1;
					i2c.keyflag =1;
				}
				else
				{
					i2c.keyflag =0;
				}
			}
			#endif
			if(touch_key_hold_press == 0)
			{
			#ifdef REPORT_PRESSURE
				ilitek_touch_down(i, x, y, 10);
			#else
				ilitek_touch_down(i, x, y, 10);
			#endif
			}
			ret = 0;
		} else {
			DBG("ilitek RELEASE_POINT  i = %d  \n", i);
			// release point
		#ifdef TOUCH_PROTOCOL_B
			ilitek_touch_release(i);
		#else
			release_counter++;
		#ifdef REPORT_PRESSURE
			input_event(i2c.input_dev, EV_ABS, ABS_MT_PRESSURE, 0);
		#endif
			input_mt_sync(i2c.input_dev);
		#endif
		}
	}
	if(len == 0)
	{
		ilitek_touch_release_all_point(0);
		touch_key_hold_press = 0;
		i2c.keyflag =0;
		input_report_key(i2c.input_dev,  KEY_HOMEPAGE, 0);
		input_report_key(i2c.input_dev,  KEY_BACK, 0);
		input_report_key(i2c.input_dev,  KEY_MENU, 0);
	}
#ifdef TOUCH_PROTOCOL_B
	input_mt_report_pointer_emulation(i2c.input_dev, true);
#endif
	input_sync(i2c.input_dev);
	//printk("ilitek end\n");
	return 0;
}
#endif
#else
static int ilitek_report_data_2XX(void) {
	struct input_dev *input = i2c.input_dev;
	unsigned char buf[64]={0};
	int ret = 0, len = 0, i = 0, x = 0, y = 0, tmp = 0, mult_tp_id = 0, key = 0, key_id = 0;
#ifdef REPORT_PRESSURE
	int pressure = 0;
#endif
			
#ifndef TOUCH_PROTOCOL_B
	int release_counter = 0;
#endif
#ifdef GESTURE
#if GESTURE == GESTURE_DRIVER
	int gesture_mode=0;
#endif
#endif
	//ilitek_system_resume = 0;
	// read i2c data from device
	buf[0] = ILITEK_TP_CMD_READ_DATA;
	ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 0, buf, 1);
	if(ret < 0){
		return ret;
	}
	len = buf[0];
	ret = 1;
	DBG("ilitek len = %d\n", len);
	if(len>20)
		return ret;
	// read touch point
	for(i=0; i<len; i++) {
		// parse point
		buf[0] = ILITEK_TP_CMD_READ_SUB_DATA;
		if(ilitek_i2c_write_and_read(i2c.client, buf, 1, 0, buf, 5)){
			x = (((int)buf[1]) << 8) + buf[2];
			y = (((int)buf[3]) << 8) + buf[4];
			if (EXCHANG_XY) {
				tmp = x;
				x = y;
				y = tmp;
			}
			if (REVERT_X) {
				x = i2c.max_x - x;
			}
				
			if (REVERT_Y) {
				y = i2c.max_y - y;
			}
			
			#ifdef REPORT_PRESSURE
				pressure = 10;
			#endif
			DBG("buf[0] = 0x%X,buf[1] = 0x%X,buf[2] = 0x%X,buf[3] = 0x%X,buf[4] = 0x%X\n",buf[0],buf[1],buf[2],buf[3],buf[4]);
			DBG("buf[0] = %d,x = %d,y = %d\n",buf[0],x,y);

			mult_tp_id = buf[0];
			switch ((mult_tp_id & 0xC0)){
				#ifdef VIRTUAL_KEY_PAD	
				case RELEASE_KEY:
					//release key
					DBG("Key: Release\n");
					for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
						if(touch_key_press[key]){
							input_report_key(input, touch_key_code[key], 0);
							touch_key_press[key] = 0;
							DBG("Key:%d ID:%d release\n", touch_key_code[key], key);
							DBG(ILITEK_DEBUG_LEVEL "%s key release, %X, %d, %d\n", __func__, buf[0], x, y);
						}
						touch_key_hold_press=0;
						i2c.keyflag = 0;
						//ret = 1;// stop timer interrupt	
					}		

					break;
				
				case TOUCH_KEY:
					DBG("TOUCH_KEY: \n");
					#ifdef GESTURE
					#if GESTURE == GESTURE_DRIVER
					if (ilitek_system_resume == 0) {
						return 0;
					}
					#endif
					#endif
					//touch key
				#if VIRTUAL_FUN==VIRTUAL_FUN_1
					key_id = buf[1] - 1;
					i2c.keyflag = 1;
				#endif	
				#if VIRTUAL_FUN==VIRTUAL_FUN_2
					if (abs(jiffies-touch_time) < msecs_to_jiffies(BTN_DELAY_TIME))
						break;
					//DBG("Key: Enter\n");
					x = (((int)buf[4]) << 8) + buf[3];
					
					//printk("%s,x=%d\n",__func__,x);
					if (x > KEYPAD01_X1 && x<KEYPAD01_X2)		// btn 1
					{
						key_id = 0;
						i2c.keyflag = 1;
					}
					else if (x > KEYPAD02_X1 && x<KEYPAD02_X2)	// btn 2
					{
						key_id = 1;
						i2c.keyflag = 1;
					}
					else if (x > KEYPAD03_X1 && x<KEYPAD03_X2)	// btn 3
					{
						key_id = 2;
						i2c.keyflag = 1;
					}
					else if (x > KEYPAD04_X1 && x<KEYPAD04_X2)	// btn 4
					{
						key_id = 3;
						i2c.keyflag = 1;
					}
					else 
					{
						i2c.keyflag = 0;
					}
				#endif
				DBG("TOUCH_KEY: key_id = %d\n", key_id);
					if((touch_key_press[key_id] == 0) && (touch_key_hold_press == 0 && i2c.keyflag)){
						input_report_key(input, touch_key_code[key_id], 1);
						touch_key_press[key_id] = 1;
						touch_key_hold_press = 1;
						DBG("Key:%d ID:%d press x=%d,touch_key_hold_press=%d,key_flag=%d\n", touch_key_code[key_id], key_id,x,touch_key_hold_press,i2c.keyflag);
					}
					break;					
				#endif	
				case TOUCH_POINT:
					DBG("TOUCH_POINT: \n");
					#ifdef VIRTUAL_KEY_PAD		
					#if VIRTUAL_FUN==VIRTUAL_FUN_3
					if((buf[0] & 0x80) != 0 && ( y > KEYPAD_Y) && i==0){
						DBG("%s,touch key\n",__func__);
						if((x > KEYPAD01_X1) && (x < KEYPAD01_X2)){
							input_report_key(input,  touch_key_code[0], 1);
							touch_key_press[0] = 1;
							touch_key_hold_press = 1;
							DBG("%s,touch key=0 ,touch_key_hold_press=%d\n",__func__,touch_key_hold_press);
						}
						else if((x > KEYPAD02_X1) && (x < KEYPAD02_X2)){
							input_report_key(input, touch_key_code[1], 1);
							touch_key_press[1] = 1;
							touch_key_hold_press = 1;
							DBG("%s,touch key=1 ,touch_key_hold_press=%d\n",__func__,touch_key_hold_press);
						}
						else if((x > KEYPAD03_X1) && (x < KEYPAD03_X2)){
							input_report_key(input, touch_key_code[2], 1);
							touch_key_press[2] = 1;
							touch_key_hold_press = 1;
							DBG("%s,touch key=2 ,touch_key_hold_press=%d\n",__func__,touch_key_hold_press);
						}
						else {
							input_report_key(input, touch_key_code[3], 1);
							touch_key_press[3] = 1;
							touch_key_hold_press = 1;
							DBG("%s,touch key=3 ,touch_key_hold_press=%d\n",__func__,touch_key_hold_press);
						}
						
					}
					if((buf[0] & 0x80) != 0 && y <= KEYPAD_Y) {
						touch_key_hold_press=0;
					}
					if((buf[0] & 0x80) != 0 && y <= KEYPAD_Y)
					#endif
					#endif
					#ifdef GESTURE
					#if GESTURE == GESTURE_DRIVER
					DBG("ilitek_system_resume=%d,len=%d\n", ilitek_system_resume,len);
					if(ilitek_system_resume == 0)
					{
					  if(len==1)
					   {
						#if GESTURE_FUN == GESTURE_FUN_1	
						gesture_mode = ilitek_GestureMatchProcess(1,gesture_count,x,y);
						gesture_count++;
						#endif
						if(gesture_count == 0)
						 gesture_count++;
						}
					}
				    else
					#endif	
					#endif	
					{				
						// report to android system
						DBG("Point, ID=%02X, X=%04d, Y=%04d,touch_key_hold_press=%d\n",buf[0]  & 0x3F, x,y,touch_key_hold_press);	
						ilitek_touch_down((buf[0] & 0x3F)-1, x, y, 10);
						ret=0;
					}
					break;
					
				case RELEASE_POINT:
					DBG("RELEASE_POINT: \n");
					if (touch_key_hold_press !=0 && i==0){
						for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
							if(touch_key_press[key]){
								input_report_key(input, touch_key_code[key], 0);
								touch_key_press[key] = 0;
								DBG("Key:%d ID:%d release\n", touch_key_code[key], key);
								DBG(ILITEK_DEBUG_LEVEL "%s key release, %X, %d, %d,touch_key_hold_press=%d\n", __func__, buf[0], x, y,touch_key_hold_press);
							}
							touch_key_hold_press=0;
							//ret = 1;// stop timer interrupt	
						}		
					}
					// release point
				#ifdef CLOCK_INTERRUPT
					#ifdef TOUCH_PROTOCOL_B
					ilitek_touch_release((buf[0] & 0x3F) - 1);
					#else
					release_counter++;
					if(release_counter == len)
					{
						i2c.keyflag = 0;
						for(i = 0; i < i2c.max_tp; i++)
						{
							i2c.release_flag[i] = 0;
						}
						ilitek_touch_release(0);
					}
					#endif
				#endif			
					ret=0;	
					break;
					
				default:
					break;
			}
		}
	}
	// release point
	if(len == 0){
		#ifdef GESTURE
		#if GESTURE == GESTURE_DRIVER
			ilitek_gesture_check();
		#endif						
		#endif						
		
		i2c.keyflag = 0;
		DBG("Release3, ID=%02X, X=%04d, Y=%04d\n",buf[0]  & 0x3F, x,y);
		ret = 1;
		if (touch_key_hold_press !=0){
			for(key=0; key<sizeof(touch_key_code)/sizeof(touch_key_code[0]); key++){
				if(touch_key_press[key]){
					input_report_key(input, touch_key_code[key], 0);
					touch_key_press[key] = 0;
					DBG("Key:%d ID:%d release\n", touch_key_code[key], key);
					DBG(ILITEK_DEBUG_LEVEL "%s key release, %X, %d, %d\n", __func__, buf[0], x, y);
				}
				touch_key_hold_press=0;
				//ret = 1;// stop timer interrupt	
			}		
		}
		ilitek_touch_release_all_point(0);
	}
	
#ifdef TOUCH_PROTOCOL_B
	input_mt_report_pointer_emulation(i2c.input_dev, true);
#endif
	input_sync(i2c.input_dev);
	return 0;
}

static int ilitek_report_data_3XX(void) {
	struct input_dev *input = i2c.input_dev;
	unsigned char buf[64]={0};
	int ret = 0, i = 0, j = 0, x = 0, y = 0, tmp = 0, tp_status = 0, packet = 0, max_point = 6;
#ifdef REPORT_PRESSURE
	int pressure = 0;
#endif
		
	int release_counter = 0;
#ifdef GESTURE
#if GESTURE == GESTURE_DRIVER
	int gesture_mode=0;
#endif
#endif
	// read i2c data from device
	buf[0] = ILITEK_TP_CMD_READ_DATA;
	ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 0, buf, 31);
	if(ret < 0){
		return ret;
	}
	packet = buf[0];
	DBG("ilitek packet = %d\n", packet);
	ret = 1;
	if (packet == 2){
		ret = ilitek_i2c_read(i2c.client, buf+31, 20);
		if(ret < 0){
			return ret;
		}
		max_point = 10;
	}
	// read touch point
	for(i = 0; i < max_point; i++){
		tp_status = buf[i*5+1] >> 7;	
		x = (((buf[i*5+1] & 0x3F) << 8) + buf[i*5+2]);
		y = (buf[i*5+3] << 8) + buf[i*5+4];
		if (EXCHANG_XY) {
			tmp = x;
			x = y;
			y = tmp;
		}
		if (REVERT_X) {
			x = i2c.max_x - x;
		}
			
		if (REVERT_Y) {
			y = i2c.max_y - y;
		}
		#ifdef REPORT_PRESSURE
			pressure = 10;
		#endif
		if(tp_status){
			#ifdef GESTURE
			#if GESTURE == GESTURE_DRIVER
			i2c.release_flag[i] = 1;
			DBG("ilitek_system_resume=%d\n", ilitek_system_resume);
			if(ilitek_system_resume == 0)
			{
			  //if(len==1)
			   {
				#if GESTURE_FUN == GESTURE_FUN_1	
				gesture_mode = ilitek_GestureMatchProcess(1,gesture_count,x,y);
				gesture_count++;
				#endif
				if(gesture_count == 0)
				 gesture_count++;
				}
			}
			else
			#endif	
			#endif	
			{
			#ifdef VIRTUAL_KEY_PAD
			if(i2c.keyflag == 0){
				for(j = 0; j < i2c.keycount; j++){
					if((x >= i2c.keyinfo[j].x && x <= i2c.keyinfo[j].x + i2c.key_xlen) && (y >= i2c.keyinfo[j].y && y <= i2c.keyinfo[j].y + i2c.key_ylen)){
						input_report_key(input,  i2c.keyinfo[j].id, 1);
						i2c.keyinfo[j].status = 1;
						touch_key_hold_press = 1;
						i2c.release_flag[0] = 1;
						DBG("Key, Keydown ID=%d, X=%d, Y=%d, key_status=%d,keyflag=%d\n", i2c.keyinfo[j].id ,x ,y , i2c.keyinfo[j].status,i2c.keyflag);
						break;
					}
				}
			}
			#endif	
			if(touch_key_hold_press == 0){
				i2c.keyflag = 1;
				ilitek_touch_down(i, x, y, 10);
				DBG("Point, ID=%02X, X=%04d, Y=%04d,release_flag[%d]=%d,tp_status=%d,keyflag=%d\n",i, x,y,i,i2c.release_flag[i],tp_status,i2c.keyflag); 
			}
			#ifdef VIRTUAL_KEY_PAD	
			if(touch_key_hold_press == 1){
				for(j = 0; j <= i2c.keycount; j++){
					if((i2c.keyinfo[j].status == 1) && (x < i2c.keyinfo[j].x || x > i2c.keyinfo[j].x + i2c.key_xlen || y < i2c.keyinfo[j].y || y > i2c.keyinfo[j].y + i2c.key_ylen)){
						input_report_key(input,  i2c.keyinfo[j].id, 0);
						i2c.keyinfo[j].status = 0;
						touch_key_hold_press = 0;
						DBG("Key, Keyout ID=%d, X=%d, Y=%d, key_status=%d\n", i2c.keyinfo[j].id ,x ,y , i2c.keyinfo[j].status);
						break;
					}
				}
			}
			#endif		
			}
			ret = 0;
		}
		else{
			#ifdef TOUCH_PROTOCOL_B
			ilitek_touch_release(i);
			#else
			//release_flag[i] = 0;
			i2c.release_flag[i] = 0;
			DBG("Point, ID=%02X, X=%04d, Y=%04d,release_flag[%d]=%d,tp_status=%d\n",i, x,y,i,i2c.release_flag[i],tp_status);	
			input_mt_sync(i2c.input_dev);
			#endif
		} 
			
	}
	if(packet == 0 ){
		DBG("ilitek packet = 0\n");
		#ifdef GESTURE
		#if GESTURE == GESTURE_DRIVER
			ilitek_gesture_check();
		#endif						
		#endif						
		i2c.keyflag = 0;
		ilitek_touch_release_all_point(0);
	}
	else{
		for(i = 0; i < max_point; i++){
			if(i2c.release_flag[i] == 0)
				release_counter++;
		}
		if(release_counter == max_point ){
			DBG("ilitek release_counter == max_point\n");
			#ifdef GESTURE
			#if GESTURE == GESTURE_DRIVER
				ilitek_gesture_check();
			#endif						
			#endif						
			i2c.keyflag = 0;
			ilitek_touch_release_all_point(0);
			#ifdef VIRTUAL_KEY_PAD	
			i2c.keyflag = 0;
			if (touch_key_hold_press == 1){
				for(i = 0; i < i2c.keycount; i++){
					if(i2c.keyinfo[i].status){
						input_report_key(input, i2c.keyinfo[i].id, 0);
						i2c.keyinfo[i].status = 0;
						touch_key_hold_press = 0;
						DBG("Key, Keyup ID=%d, X=%d, Y=%d, key_status=%d, touch_key_hold_press=%d\n", i2c.keyinfo[i].id ,x ,y , i2c.keyinfo[i].status, touch_key_hold_press);
					}
				}
			}
			#endif	
		}
		DBG("release_counter=%d,packet=%d\n",release_counter,packet);
	}
#ifdef TOUCH_PROTOCOL_B
	input_mt_report_pointer_emulation(i2c.input_dev, true);
#endif
	input_sync(i2c.input_dev);
	return 0;
}
#endif
//shawn
/*
   description
   process i2c data and then report to kernel
   parameters
   none
   return
   status
 */
static int ilitek_i2c_process_and_report(void)
{
	if(i2c.report_status == 0){
		return 1;
	}
	#if IC2120
		#ifndef NEW_PROTOCOL_2120
		ilitek_report_data_2120_old();
		#else
		ilitek_report_data_2120_new();
	#endif
	#else
	#if 1
		//mutli-touch for protocol 3.0
		if((i2c.protocol_ver & 0xFF00) == 0x300){
			ilitek_report_data_3XX();
		}
		// multipoint process
		else if((i2c.protocol_ver & 0xFF00) == 0x200){
			ilitek_report_data_2XX();
		}
		
	#endif
	#endif
	return 0;
}

#ifndef CLOCK_INTERRUPT
static void ilitek_i2c_timer(unsigned long handle)
{
	struct i2c_data *priv = (void *)handle;
	DBG("Enter\n");

	schedule_work(&priv->irq_work);
}
#endif
/*
   description
   work queue function for irq use
   parameter
   work
   work queue
   return
   nothing
 */
static void ilitek_i2c_irq_work_queue_func(struct work_struct *work)
{
	int ret;
#ifndef CLOCK_INTERRUPT
	struct i2c_data *priv = container_of(work, struct i2c_data, irq_work);
#endif
	DBG("Enter\n");

	ret = ilitek_i2c_process_and_report();
#ifdef CLOCK_INTERRUPT
	ilitek_i2c_irq_enable();
#else
	if (ret == 0){
		if (!i2c.stop_polling)
			mod_timer(&priv->timer, jiffies + msecs_to_jiffies(0));
	}
	else if (ret == 1){
		if (!i2c.stop_polling){
			ilitek_i2c_irq_enable();
		}
		DBG("stop_polling\n");
	}
	else if(ret < 0){
		msleep(100);
		DBG(ILITEK_ERROR_LEVEL "%s, process error\n", __func__);
		ilitek_i2c_irq_enable();
	}
#endif
}

/*
   description
   i2c interrupt service routine
   parameters
   irq
   interrupt number
   dev_id
   device parameter
   return
   return status
 */
static irqreturn_t ilitek_i2c_isr(int irq, void *dev_id)
{
	DBG("11111 Enter\n");
	#ifdef ILITEK_ESD_CHECK
		atomic_set(&ilitek_cmd_response, 1);
	#endif
	#ifdef ILI_UPDATE_FW
	#ifdef UPDATE_THREADE
	if (update_wait_flag == 1) {
		DBG(ILITEK_ERROR_LEVEL "%s 11111 update_wait_flag =1 return\n",__func__);
		return IRQ_HANDLED;
	}
	#endif
	#endif
	if(i2c.irq_status ==1){
		disable_irq_nosync(i2c.client->irq);
		DBG("ilitek disable nosync\n");
		i2c.irq_status = 0;
	}
	if(update_Flag == 1){
		DBG(ILITEK_ERROR_LEVEL "%s 11111 update_Flag =1\n",__func__);
		int_Flag = 1;
	}
	else{
		#ifndef REPORT_THREAD
		queue_work(i2c.irq_work_queue, &i2c.irq_work);
		#else
		tpd_flag = 1;
		wake_up_interruptible(&waiter);
		#endif
	}

	return IRQ_HANDLED;
}
#ifdef ILI_UPDATE_FW
#ifdef UPDATE_THREADE
/*
   description
   ilitek_i2c_touchevent_thread
   parameters
   arg
   arguments
   return
   return status
 */
static int ilitek_i2c_update_thread(void *arg)
{

	int ret=0;
	printk(ILITEK_DEBUG_LEVEL "%s, enter\n", __func__);

	if(kthread_should_stop()){
		printk(ILITEK_DEBUG_LEVEL "%s, stop\n", __func__);
		return -1;
	}
	
	disable_irq(i2c.client->irq);
	printk("ilitek disable irq\n");
	#if 1
	update_wait_flag = 1;
	ret = ilitek_upgrade_firmware();
	if(ret==2) {
		printk("ilitek update end\n");
	}
	else if(ret==3) {
		printk("ilitek i2c communication error\n");
	}
	if(i2c.reset_request_success){
		ilitek_reset(i2c.reset_gpio);
	}
	#endif
	// read touch parameter
	ret=ilitek_i2c_read_tp_info();
	if(ret < 0)
	{
		return ret;
	}
	
	enable_irq(i2c.client->irq);
	printk("ilitek enable irq\n");
	update_wait_flag = 0;
	// register input device
	ilitek_set_input_param(i2c.input_dev, i2c.max_tp, i2c.max_x, i2c.max_y);
	ret = input_register_device(i2c.input_dev);
	if(ret){
		printk(ILITEK_ERROR_LEVEL "%s, register input device, error\n", __func__);
		return ret;
	}
	printk(ILITEK_ERROR_LEVEL "%s, register input device, success\n", __func__);
	

	printk(ILITEK_DEBUG_LEVEL "%s, exit\n", __func__);
	return ret;
}
#endif
#endif
#ifdef CLOCK_INTERRUPT
#ifdef REPORT_THREAD

/*
   description
   ilitek_i2c_touchevent_thread
   parameters
   arg
   arguments
   return
   return status
 */
static int ilitek_i2c_touchevent_thread(void *arg)
{

	int ret=0;
	struct sched_param param = { .sched_priority = 4};
	sched_setscheduler(current, SCHED_RR, &param);	
	DBG("Enter\n");
	// check input parameter
	printk(ILITEK_DEBUG_LEVEL "%s, enter\n", __func__);
	//write_update_fw();

	// mainloop
	while(1){
		// check whether we should exit or not
		DBG("ilitek enter tpd_flag = %d\n", tpd_flag);
		if(kthread_should_stop()){
			printk(ILITEK_DEBUG_LEVEL "%s, stop\n", __func__);
			break;
		}
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, tpd_flag != 0);
		tpd_flag = 0;
		set_current_state(TASK_RUNNING);
		// read i2c data
		if(ilitek_i2c_process_and_report() < 0){
			msleep(3000);
			printk(ILITEK_ERROR_LEVEL "%s, process error\n", __func__);
		}
		#ifdef CLOCK_INTERRUPT
		ilitek_i2c_irq_enable();
		#endif
	}

	printk(ILITEK_DEBUG_LEVEL "%s, exit\n", __func__);
	return ret;
}
#endif
#endif
/*
   description
   i2c polling thread
   parameters
   arg
   arguments
   return
   return status
 */
static int ilitek_i2c_polling_thread(void *arg)
{

	int ret=0;
	DBG("Enter\n");
	// check input parameter
	printk(ILITEK_DEBUG_LEVEL "%s, enter\n", __func__);

	// mainloop
	while(1){
		// check whether we should exit or not
		if(kthread_should_stop()){
			printk(ILITEK_DEBUG_LEVEL "%s, stop\n", __func__);
			break;
		}

		// this delay will influence the CPU usage and response latency
		msleep(10);

		// when i2c is in suspend or shutdown mode, we do nothing
		if(i2c.stop_polling){
			msleep(1000);
			continue;
		}

		// read i2c data
		if(ilitek_i2c_process_and_report() < 0){
			msleep(3000);
			printk(ILITEK_ERROR_LEVEL "%s, process error\n", __func__);
		}
	}

	printk(ILITEK_DEBUG_LEVEL "%s, exit\n", __func__);
	return ret;
}

#if defined(CONFIG_FB)
/*******************************************************
Function:
Early suspend function.
Input:
h: early_suspend struct.
Output:
None.
 *******************************************************/
static void ilitek_ts_suspend(struct i2c_data *ts)
{
	ilitek_i2c_suspend(i2c.client, PMSG_SUSPEND);
	printk(ILITEK_ERROR_LEVEL "%s\n", __func__);
}

/*******************************************************
Function:
Late resume function.
Input:
h: early_suspend struct.
Output:
None.
 *******************************************************/
static void ilitek_ts_resume(struct i2c_data *ts)
{
	ilitek_i2c_resume(i2c.client);
	//printk(ILITEK_ERROR_LEVEL "%s\n", __func__);

}

static int fb_notifier_callback(struct notifier_block *self,
		unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct i2c_data *ts = container_of(self, struct i2c_data, fb_notif);

	if (evdata && evdata->data && event == FB_EVENT_BLANK &&
			ts && ts->client) {
		blank = evdata->data;
		if (*blank == FB_BLANK_UNBLANK) {
			printk("fb_notifier_callback ilitek_ts_resume\n");
			ilitek_ts_resume(ts);
		}
		else if (*blank == FB_BLANK_POWERDOWN) {
			printk("fb_notifier_callback ilitek_ts_suspend\n");
			ilitek_ts_suspend(ts);
		}
	}

	return 0;
}

#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void ilitek_i2c_early_suspend(struct early_suspend *h)
{
	ilitek_i2c_suspend(i2c.client, PMSG_SUSPEND);
	printk(ILITEK_ERROR_LEVEL "%s\n", __func__);
}

/*
   description
   i2c later resume function
   parameters
   h
   early suspend pointer
   return
   nothing
 */
static void ilitek_i2c_late_resume(struct early_suspend *h)
{
	ilitek_i2c_resume(i2c.client);
	printk("%s\n", __func__);
}
#endif

/*
   description
   i2c irq enable function
 */
void ilitek_i2c_irq_enable(void)
{
	if (i2c.irq_status == 0){
		i2c.irq_status = 1;
		enable_irq(i2c.client->irq);
		DBG("enable\n");
		//printk("ilitek_i2c_irq_enable ok.\n");
	}
	else
		DBG("no enable\n");
}
/*
   description
   i2c irq disable function
 */
void ilitek_i2c_irq_disable(void)
{
	if (i2c.irq_status == 1){
		i2c.irq_status = 0;
		disable_irq(i2c.client->irq);
		DBG("ilitek_i2c_irq_disable ok.\n");
	}
	else
		DBG("no disable\n");
}
static int ilitek_handle_irqorpolling(void) {
	int ret = 0;
#ifndef POLLING_MODE
		
	ret = ilitek_set_irq();
	printk("%s, IRQ: 0x%X\n", __func__, (i2c.client->irq));
	if(ret < 0){
		printk("ilitek set irq fail\n");
	}

	if(i2c.client->irq != 0 ){ // == => polling mode, != => interrup mode
		i2c.irq_work_queue = create_singlethread_workqueue("ilitek_i2c_irq_queue");
		if(i2c.irq_work_queue){
			INIT_WORK(&i2c.irq_work, ilitek_i2c_irq_work_queue_func);
#ifdef CLOCK_INTERRUPT
			if(Request_IRQ()){
				printk(ILITEK_ERROR_LEVEL "%s, request irq, error\n", __func__);
			}
			else{
				ret = ilitek_config_irq();
				if(ret < 0){
					printk("ilitek config irq fail\n");
				}
				i2c.valid_irq_request = 1;
				i2c.irq_status = 1;
				printk(ILITEK_ERROR_LEVEL "%s, request irq, success\n", __func__);
			}
#else
			init_timer(&i2c.timer);
			i2c.timer.data = (unsigned long)&i2c;
			i2c.timer.function = ilitek_i2c_timer;
			if(Request_IRQ()){
				printk(ILITEK_ERROR_LEVEL "%s, request irq, error\n", __func__);
			}
			else{
				ret = ilitek_config_irq();
				if(ret < 0){
					printk("ilitek config irq fail\n");
				}
				i2c.valid_irq_request = 1;
				i2c.irq_status = 1;
				printk(ILITEK_ERROR_LEVEL "%s, request irq, success\n", __func__);
			}
#endif
		}
	}
	else
#endif
	{
#ifdef POLLING_MODE
seting_polling_mode:
#endif
		printk( "%s,i2c.client->irq = %d , setting polling mode \n", __func__,(i2c.client->irq));
		i2c.stop_polling = 0;
		i2c.thread = kthread_create(ilitek_i2c_polling_thread, NULL, "ilitek_i2c_thread");
		if(i2c.thread == (struct task_struct*)ERR_PTR){
			i2c.thread = NULL;
			printk(ILITEK_ERROR_LEVEL "%s, kthread create, error\n", __func__);
		}
		else{
			i2c.set_polling_mode = 1;
			wake_up_process(i2c.thread);
		}
	}
#ifdef POLLING_MODE
	if(i2c.set_polling_mode != 1){
		goto seting_polling_mode;
	}
#endif
	return 0;
}


/*
   description
   read touch information
   parameters
   none
   return
   status
 */
int ilitek_i2c_read_tp_info( void)
{
	unsigned char buf[64] = {0};
	int ret=0;
	int i = 0;
	#if !IC2120
	int res_len = 0;
	#endif
	#if IC2120
		for (i = 0; i < 20; i++) {
			buf[0] = 0x10;
			ret = ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 3);
			if(ret<0)
			{
			return -1;
			}
			printk("ilitek %s, write 0x10 read buf = %X, %X, %X\n", __func__, buf[0], buf[1], buf[2]);
			if (buf[1] >= 0x80) {
				printk("FW is ready  ok ok \n");
				break;
			}else {
				mdelay(5);
			}
		}
		if (i >= 20) {
#ifdef ILI_UPDATE_FW
			printk("ilitek wirte 0x10 read data error (< 0x80) so driver_upgrade_flag = true\n");
			driver_upgrade_flag = true;
#endif
		}
		else {
			//read firmware version
			buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
			buf[1] = ILITEK_TP_CMD_GET_FIRMWARE_VERSION;
			if(ilitek_i2c_write_and_read(i2c.client, buf, 2, 10, buf, 0) < 0)
			{
				//tpd_load_status = -1;
				return -1;
			}
			buf[0] = ILITEK_TP_CMD_GET_FIRMWARE_VERSION;
			if(ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 3) < 0)
			{
				return -1;
			}
			printk(ILITEK_DEBUG_LEVEL "%s, firmware version:%d.%d.%d\n", __func__, buf[0], buf[1], buf[2]);
			if ((buf[0] == 0 &&  buf[1] == 0 &&  buf[2] == 0) || (buf[0] == 0xFF ||  buf[1] == 0xFF ||  buf[2] == 0xFF)) {
				#ifdef ILI_UPDATE_FW
				printk("ilitek firmware version Exception so driver_upgrade_flag = true\n");
				driver_upgrade_flag = true;
				#endif
			}
			i2c.firmware_ver[0] = 0;
			for(i = 1; i < 4; i++)
			{
				i2c.firmware_ver[i] = buf[i - 1];
			}
			//read protocol version
			buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
			buf[1] = ILITEK_TP_CMD_GET_PROTOCOL_VERSION;
			if(ilitek_i2c_write_and_read(i2c.client, buf, 2, 10, buf, 0) < 0)
			{
				return -1;
			}

			buf[0] = ILITEK_TP_CMD_GET_PROTOCOL_VERSION;
			if(ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 2) < 0)
			{
				return -1;
			}
			i2c.protocol_ver = (((int)buf[0]) << 8) + buf[1];
			printk(ILITEK_DEBUG_LEVEL "%s, protocol version:%d.%d\n", __func__, buf[0], buf[1]);
			if ((buf[0] == 0 &&  buf[1] == 0) || (buf[0] == 0xFF ||  buf[1] == 0xFF)) {
				#ifdef ILI_UPDATE_FW
				printk("ilitek protocol version Exception so driver_upgrade_flag = true\n");
				driver_upgrade_flag = true;
				#endif
			}
			//read touch resolution
			buf[0] = ILITEK_TP_CMD_READ_DATA_CONTROL;
			buf[1] = 0x20;
			if(ilitek_i2c_write_and_read(i2c.client, buf, 2, 10, buf, 0) < 0)
			{
				return -1;
			}
			buf[0] = 0x20;
			if(ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 10) < 0)
			{
				return -1;
			}
			//calculate the resolution for x and y direction
			i2c.max_x = buf[2];
			i2c.max_x+= ((int)buf[3]) * 256;
			i2c.max_y = buf[4];
			i2c.max_y+= ((int)buf[5]) * 256;
			i2c.min_x = buf[0];
			i2c.min_y = buf[1];
			i2c.x_ch = buf[6];
			i2c.y_ch = buf[7];
			//maximum touch point
			i2c.max_tp = buf[8];
			//key count
			i2c.keycount = buf[9];
			if ((i2c.max_tp == 0 &&  i2c.x_ch == 0  &&  i2c.y_ch == 0 && i2c.max_x == 0 &&  i2c.max_y == 0) || (i2c.max_tp == 0xFF ||  i2c.x_ch == 0xFF ||  i2c.y_ch == 0xFF || i2c.max_x == 0xFFFF ||  i2c.max_y == 0xFFFF)) {
				#ifdef ILI_UPDATE_FW
				printk("ilitek FW info Exception so driver_upgrade_flag = true\n");
				driver_upgrade_flag = true;
				#endif
			}
		}
	#else
		buf[0] = ILITEK_TP_CMD_GET_KERNEL_VERSION;
		ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 5);
		printk(ILITEK_DEBUG_LEVEL "%s, MCU KERNEL version:%d.%d.%d.%d.%d\n", __func__, buf[0], buf[1], buf[2],buf[3], buf[4]);
		if ((buf[0] == 0x03 || buf[0] == 0x09 || buf[1] == 0x23)) {
			serial23 = true;
			printk("ilitek serial23 = %d\n", serial23);
		}
		buf[0] = ILITEK_TP_CMD_GET_FIRMWARE_VERSION;
		if (!serial23) {
			if(ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 4) < 0)
			{
				return -1;
			}
			printk(ILITEK_DEBUG_LEVEL "%s, firmware version:%d.%d.%d.%d\n", __func__, buf[0], buf[1], buf[2],buf[3]);
			for(i = 0; i < 4; i++)
			{
				i2c.firmware_ver[i] = buf[i];
			}
			for(i = 4; i < 8; i++)
			{
				i2c.firmware_ver[i] = 0;
			}
		}
		else {
			if(ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 8) < 0)
			{
				return -1;
			}
			printk(ILITEK_DEBUG_LEVEL "%s, firmware version:%d.%d.%d.%d.%d.%d.%d.%d\n", __func__, buf[0], buf[1], buf[2],buf[3], buf[4], buf[5], buf[6],buf[7]);
			for(i = 0; i < 8; i++)
			{
				i2c.firmware_ver[i] = buf[i];
			}
			if (buf[4] == 0xff && buf[5] == 0xff && buf[6] == 0xff && buf[7] == 0xff) {
				
				printk("ilitek the firmware version last 4 bytes is 0xff \n");
				#if 0
				for(i = 0; i < 4; i++)
				{
					i2c.firmware_ver[i] = 0;
				}
				#endif
			}
			else {
				for(i = 0; i < 4; i++)
				{
					i2c.firmware_ver[i] = buf[i + 4];
				}
			}
		}
		res_len = 6;
		buf[0] = ILITEK_TP_CMD_GET_PROTOCOL_VERSION;
		if(ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 2) < 0)
		{
			return -1;
		}
		i2c.protocol_ver = (((int)buf[0]) << 8) + buf[1];
		printk(ILITEK_DEBUG_LEVEL "%s, protocol version: %d.%d\n", __func__, buf[0], buf[1]);
		if((i2c.protocol_ver & 0xFF00) == 0x200){
			res_len = 8;
		}
		else if((i2c.protocol_ver & 0xFF00) == 0x300){
			res_len = 10;
		}
		
		// read touch resolution
		i2c.max_tp = 2;
		buf[0] = ILITEK_TP_CMD_GET_RESOLUTION;
		if(ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, res_len) < 0)
		{
			return -1;
		}
		
		if((i2c.protocol_ver & 0xFF00) == 0x200){
			// maximum touch point
			i2c.max_tp = buf[6];
			// maximum button number
			i2c.max_btn = buf[7];
		}
		else if((i2c.protocol_ver & 0xFF00) == 0x300){
			// maximum touch point
			i2c.max_tp = buf[6];
			// maximum button number
			i2c.max_btn = buf[7];
			// key count
			i2c.keycount = buf[8];
		}
		
		// calculate the resolution for x and y direction
		i2c.max_x = buf[0];
		i2c.max_x+= ((int)buf[1]) * 256;
		i2c.max_y = buf[2];
		i2c.max_y+= ((int)buf[3]) * 256;
		i2c.x_ch = buf[4];
		i2c.y_ch = buf[5];
		
		printk(ILITEK_DEBUG_LEVEL "%s, max_x: %d, max_y: %d, ch_x: %d, ch_y: %d\n", 
		__func__, i2c.max_x, i2c.max_y, i2c.x_ch, i2c.y_ch);
		
		if((i2c.protocol_ver & 0xFF00) == 0x200){
			printk(ILITEK_DEBUG_LEVEL "%s, max_tp: %d, max_btn: %d\n", __func__, i2c.max_tp, i2c.max_btn);
		}
		else if((i2c.protocol_ver & 0xFF00) == 0x300){
			printk(ILITEK_DEBUG_LEVEL "%s, max_tp: %d, max_btn: %d, key_count: %d\n", __func__, i2c.max_tp, i2c.max_btn, i2c.keycount);
			
			//get key infotmation
			buf[0] = ILITEK_TP_CMD_GET_KEY_INFORMATION;
			if(ilitek_i2c_write_and_read(i2c.client, buf, 1, 10, buf, 29) < 0)
			{
				return -1;
			}
			if (i2c.keycount > 5){
				if(ilitek_i2c_write_and_read(i2c.client, buf, 0, 10, buf + 29, 25) < 0)
				{
					return -1;
				}
			}
			
			i2c.key_xlen = (buf[0] << 8) + buf[1];
			i2c.key_ylen = (buf[2] << 8) + buf[3];
			printk(ILITEK_DEBUG_LEVEL "%s, key_xlen: %d, key_ylen: %d\n", __func__, i2c.key_xlen, i2c.key_ylen);
			
			//print key information
			for(i = 0; i < i2c.keycount; i++){
				i2c.keyinfo[i].id = buf[i*5+4]; 
				i2c.keyinfo[i].x = (buf[i*5+5] << 8) + buf[i*5+6];
				i2c.keyinfo[i].y = (buf[i*5+7] << 8) + buf[i*5+8];
				i2c.keyinfo[i].status = 0;
				printk(ILITEK_DEBUG_LEVEL "%s, key_id: %d, key_x: %d, key_y: %d, key_status: %d\n", __func__, i2c.keyinfo[i].id, i2c.keyinfo[i].x, i2c.keyinfo[i].y, i2c.keyinfo[i].status);
			}
		}
	#endif
	printk(ILITEK_DEBUG_LEVEL "%s, min_x: %d, max_x: %d, min_y: %d, max_y: %d, ch_x: %d, ch_y: %d, max_tp: %d, key_count: %d\n"
			, __func__, i2c.min_x, i2c.max_x, i2c.min_y, i2c.max_y, i2c.x_ch, i2c.y_ch, i2c.max_tp, i2c.keycount);
	return 0;
}

/*
   description
   when adapter detects the i2c device, this function will be invoked.
   parameters
   client
   i2c client data
   id
   i2c data
   return
   status
 */
static int ilitek_request_io_port(struct i2c_client *client)
{
	int err = 0;

	struct device *dev = &(client->dev);
	struct device_node *np = dev->of_node;
	#ifdef CONFIG_OF
	i2c.reset_gpio = of_get_named_gpio(np, "ilitek,reset-gpio", 0);
	#else
	i2c.reset_gpio = RESET_GPIO;
	#endif
	if (i2c.reset_gpio < 0)
	{
		printk("rst_gpio = %d\n",i2c.reset_gpio);
		return 0;
	}
	printk(ILITEK_ERROR_LEVEL "%d,%s,ilitek_request_io_port\n",__LINE__, __func__);
	#ifdef CONFIG_OF
	i2c.irq_gpio = of_get_named_gpio(np, "ilitek,irq-gpio", 0);
	#else
	i2c.irq_gpio = IRQ_GPIO;
	#endif
	if (i2c.irq_gpio < 0)
	{
		printk("========fail==========iirq_gpio = %d\n",i2c.irq_gpio);
		return 0;
	}
	printk(ILITEK_ERROR_LEVEL "%d,%s,ilitek_request_io_port\n",__LINE__, __func__);
	err= gpio_request(i2c.reset_gpio, "ilitek,reset-gpio");
	if (err) {
		printk("Failed to request reset_gpio \n");
	}
	err = gpio_direction_output(i2c.reset_gpio, 0);
	if (err) {
		printk("Failed to direction output rest GPIO err");
		return 0;
	}
	printk(ILITEK_ERROR_LEVEL "%d,%s,ilitek_request_io_port\n",__LINE__, __func__);
	msleep(20);
	//gpio_set_value_cansleep(i2c.reset_gpio, 1);

	printk(ILITEK_ERROR_LEVEL "%d,%s,ilitek_request_io_port\n",__LINE__, __func__);
	return err;

}

static int ilitek_power_on(void) {
	int ret = 0;
	
	i2c.vdd = regulator_get(&i2c.client->dev, "vdd");
	if (IS_ERR(i2c.vdd)) {
		printk("regulator_get vdd fail\n");
		return -EINVAL;
	}
	ret = regulator_set_voltage(i2c.vdd,2600000,3300000);
	if (ret < 0) {
		printk("regulator_set_voltage vdd fail\n");
		return -EINVAL;
	}
	ret = regulator_enable(i2c.vdd);
	if (ret < 0) {
		printk("regulator_enable vdd fail\n");
		return -EINVAL;
	}
	
	#if 1
	i2c.vcc_i2c = regulator_get(&i2c.client->dev, "vcc_i2c");
	if (IS_ERR(i2c.vcc_i2c)) {
		printk("regulator_get vcc_i2c fail\n");
		return -EINVAL;
	}
	ret = regulator_set_voltage(i2c.vcc_i2c,1800000,1800000);
	if (ret < 0) {
		printk("regulator_set_voltage vcc_i2c fail\n");
		return -EINVAL;
	}
	ret = regulator_enable(i2c.vcc_i2c);
	if (ret < 0) {
		printk("regulator_enable vcc_i2c fail\n");
		return -EINVAL;
	}
	#endif
	return ret;
}

/*
   description
   i2c suspend function
   parameters
   client
   i2c client data
   mesg
   suspend data
   return
   return status
 */

int ilitek_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{

#if !defined GESTURE || GESTURE == GESTURE_2120
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
#endif
#ifdef ILITEK_ESD_CHECK	
	cancel_delayed_work_sync(&esd_work);
#endif
#ifdef ILI_UPDATE_FW
#ifdef UPDATE_THREADE
	if (update_wait_flag == 1) {
		printk("%s, ilitek waiting update so return\n", __func__);
		return 0;
	}
#endif
#endif
printk("Enter ilitek_i2c_suspend \n");
#ifdef GESTURE	
#if GESTURE == GESTURE_2120
	ilitek_system_resume = 0;
	printk("Enter ilitek_i2c_suspend 0x01 0x00, 0x0A 0x01\n");
	cmd[0] = 0x01;
	cmd[1] = 0x00;
	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, 0x01 0x00 set tp suspend err, ret %d\n", __func__, ret);
	}
	msleep(30);
	cmd[0] = 0x0A;
	cmd[1] = 0x01;
	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, 0x0A 0x01 set tp suspend err, ret %d\n", __func__, ret);
	}
	enable_irq_wake(i2c.client->irq);
#elif GESTURE == GESTURE_DRIVER
	ilitek_system_resume = 0;
	printk("Enter ilitek_i2c_suspend enable_irq_wake\n");
	enable_irq_wake(i2c.client->irq);
#endif
#else	
	if(i2c.valid_irq_request != 0){
		ilitek_i2c_irq_disable();
	}
	else{
		i2c.stop_polling = 1;
		printk(ILITEK_DEBUG_LEVEL "%s, stop i2c thread polling\n", __func__);
	}
#if IC2120
	printk("Enter ilitek_i2c_suspend 0x01 0x00, 0x02 0x00\n");
	cmd[0] = 0x01;
	cmd[1] = 0x00;
	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, 0x01 0x00 set tp suspend err, ret %d\n", __func__, ret);
	}
	msleep(30);
	cmd[0] = ILITEK_TP_CMD_SLEEP;
	cmd[1] = 0x00;
	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, 0x02 0x00 set tp suspend err, ret %d\n", __func__, ret);
	}
#else
	msgs_cmd[0].len = 1;
	cmd[0] = ILITEK_TP_CMD_SLEEP;
	ret = ilitek_i2c_transfer(client, msgs_cmd, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, 0x02 0x00 set tp suspend err, ret %d\n", __func__, ret);
	}
#endif
#endif
#ifdef HALL_CHECK
#if HALL_CHECK == HALL_CHECK_OTHER
		del_timer_sync(&ilitek_hall_check_timer);
#endif
#endif //end of HALL_CHECK

	ilitek_touch_release_all_point(1);
	return 0;
}


/*
   description
   i2c resume function
   parameters
   client
   i2c client data
   return
   return status
 */
int ilitek_i2c_resume(struct i2c_client *client)
{
	printk("ilitek_i2c resume Enter\n");
#ifdef GESTURE
	//gesture_flag = 0;
	ilitek_system_resume = 1;
	//disable_irq_wake(i2c.client->irq); 
#endif
#ifdef ILITEK_ESD_CHECK
	queue_delayed_work(esd_wq, &esd_work, delay);	 
#endif
	#ifdef ILI_UPDATE_FW
	#ifdef UPDATE_THREADE
	if (update_wait_flag == 1) {
		printk("%s, ilitek waiting update so return\n", __func__);
		return 0;
	}
	#endif
	#endif
	printk("ilitek i2c.reset_request_success = %d\n", i2c.reset_request_success);
	if(i2c.reset_request_success)
	{
		ilitek_reset(i2c.reset_gpio);
	}

	if(i2c.valid_irq_request != 0){
		ilitek_i2c_irq_enable();
	}
	else{
		i2c.stop_polling = 0;
		printk(ILITEK_DEBUG_LEVEL "%s, start i2c thread polling\n", __func__);
	}
#ifdef HALL_CHECK
#if HALL_CHECK == HALL_CHECK_OTHER
	if(!curr_hall_state)
	{
		//half mode
		ilitek_into_hall_halfmode();
	}
	//ilitek_hall_check_timer.expires = jiffies + msecs_to_jiffies(HALL_TIMROUT_PERIOD);
	//add_timer(&ilitek_hall_check_timer);
	mod_timer(&ilitek_hall_check_timer, jiffies + msecs_to_jiffies(HALL_TIMROUT_PERIOD));

#elif HALL_CHECK == HALL_CHECK_HW
	if (!tpd_sensitivity_status) {
		ilitek_into_hall_halfmode();
	}
#endif

#endif //end of HALL_CHECK

	return 0;
}
/*
   description
   when adapter detects the i2c device, this function will be invoked.
   parameters
   client
   i2c client data
   id
   i2c data
   return
   status
 */
static int ilitek_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
#ifdef DEBUG_NETLINK
	struct netlink_kernel_cfg cfg = {
		.groups = 0,
		.input	= udp_receive,
	};
#endif

	int ret = 0, tmp = 0;
	#ifdef ILI_UPDATE_FW
	#ifdef UPDATE_THREADE
	struct task_struct *thread_update = NULL;
	#endif
	#ifndef FORCE_UPDATE
	int i = 0;
	#endif
	#endif
	pr_err("Enter ilitek_i2c_probe +++++++++++++++\n");
	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
		printk(ILITEK_ERROR_LEVEL "%s, I2C_FUNC_I2C not support\n", __func__);
		return -1;
	}
	ret = ilitek_should_load_driver();
	if(ret < 0){
		return ret;
	}
	if (client->addr == 0x5D) {
		printk("ilitek set addr = 0x41\n");
		client->addr = 0x41;
	}

	// initialize global variable
	#ifdef TOOL
	memset(&dev_ilitek, 0, sizeof(struct dev_data));
	#endif
	memset(&i2c, 0, sizeof(struct i2c_data));

	// initialize mutex object
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37)
	init_MUTEX(&i2c.wr_sem);
#else
	sema_init(&i2c.wr_sem,1);
#endif

	i2c.wr_sem.count = 1;

	i2c.report_status = 1;

	ret = ilitek_register_prepare();
	if(ret < 0){
		return ret;
	}
	
	i2c.client = client;
	printk(ILITEK_DEBUG_LEVEL "%s, i2c new style format\n", __func__);

	ret = ilitek_power_on();
	if (ret) {
		printk("%s, power on failed\n", __func__);
		return ret;
	}

	//
	ret = ilitek_request_io_port(client);
	if (ret != 0)
	{
		printk("io error");
		return ret;
	}
	ret = gpio_request(i2c.irq_gpio, "irq-gpio"); //Request IO
	if (ret < 0)
	{
		pr_err("gpio_request failed\n");
		return ret;
	}
	printk(ILITEK_ERROR_LEVEL "%d,%s,ilitek_i2c_probe success\n",__LINE__, __func__);
	gpio_direction_input(i2c.irq_gpio);
	
	i2c.client->irq  = gpio_to_irq(i2c.irq_gpio);
	printk("ilitek i2c.irq_gpio = %d, i2c.client->irq = %d\n", i2c.irq_gpio, i2c.client->irq);
	i2c.valid_i2c_register = 1;
	printk(ILITEK_DEBUG_LEVEL "%s, add i2c device, success\n", __func__);
	if(i2c.client == NULL){
		printk(ILITEK_ERROR_LEVEL "%s, no i2c board information\n", __func__);
		return -1;
	}
	if((i2c.client->addr == 0) || (i2c.client->adapter == 0)){
		printk(ILITEK_ERROR_LEVEL "%s, invalid register\n", __func__);
		return ret;
	}
	#if 1
	ret = ilitek_request_init_reset();
	if(ret < 0){
		printk("ilitek request reset err\n");
		i2c.reset_request_success = 0;
	}
	else{
		printk("ilitek request reset success\n");
		i2c.reset_request_success = 1;
	}
	#endif
	gpio_direction_output(i2c.reset_gpio,1);
	//msleep(50);
	mdelay(10);
	gpio_direction_output(i2c.reset_gpio,0);
	//msleep(50);
	mdelay(10);
	gpio_direction_output(i2c.reset_gpio,1);
	mdelay(10);
	tmp = 0;
	#if !IC2120
	mdelay(100);
	#else
	for (ret = 0; ret < 30; ret++ ) {
		tmp = ilitek_poll_int();
		printk("ilitek int status = %d\n", tmp);
		if (tmp == 0) {
			break;
		}
		else {
			mdelay(5);
		}
	}
	
	if (tmp >= 30) {
		
	#ifdef ILI_UPDATE_FW
		printk("ilitek reset but int not pull low so driver_upgrade_flag = true\n");
		driver_upgrade_flag = true;
	#endif
	}
	else 
	#endif
	{
		// read touch parameter
		ret = ilitek_i2c_read_tp_info();
		if(ret < 0){
			printk("ilitek read tp info fail\n");
			printk("ilitek free gpio\n");
			if (gpio_is_valid(i2c.irq_gpio)) {
				gpio_free(i2c.irq_gpio);
			}
			if (gpio_is_valid(i2c.reset_gpio)) {
				gpio_free(i2c.reset_gpio);
			}
			return ret;
		}
	}
	ret = ilitek_handle_irqorpolling();
	
	i2c.input_dev = input_allocate_device();
	if(i2c.input_dev == NULL){
		printk(ILITEK_ERROR_LEVEL "%s, allocate input device, error\n", __func__);
		return -1;
	}
#ifdef ILI_UPDATE_FW
	ret = 0;
#ifndef FORCE_UPDATE
	#if !IC2120
	if (serial23) {
		for(i = 0; i < 8; i++)
		{
			printk("ilitek i2c.firmware_ver[%d] = %d, firmware_ver[%d] = %d\n", i, i2c.firmware_ver[i], i, CTPM_FW[i + 18]);
			if((i2c.firmware_ver[i] > CTPM_FW[i + 18]) || ((i == 7) && (i2c.firmware_ver[7] == CTPM_FW[7 + 18])))
			{
				ret = 1;
				printk("ilitek_upgrade_firmware Do not need update\n"); 
				//return 1;
				break;
			}
			else if(i2c.firmware_ver[i] < CTPM_FW[i + 18])
			{
				printk("ilitek_upgrade_firmware  need update\n"); 
				ret = 0;
				break;
			}
		}
	}
	else
	#endif
	{
#if 1
		for(i = 0; i < 4; i++)
		{
			printk("i2c.firmware_ver[%d] = %d, firmware_ver[%d] = %d\n", i, i2c.firmware_ver[i], i, CTPM_FW[i + 18]);
			if((i2c.firmware_ver[i] > CTPM_FW[i + 18]) || ((i == 3) && (i2c.firmware_ver[3] == CTPM_FW[3 + 18])))
			{
				ret = 1;
				printk("ilitek_upgrade_firmware Do not need update\n"); 
				//return 1;
				break;
			}
			else if(i2c.firmware_ver[i] < CTPM_FW[i + 18])
			{
				break;
			}
		}
#else
		for(i = 0; i < 4; i++)
		{
			printk("i2c.firmware_ver[%d] = %d, firmware_ver[%d] = %d\n", i, i2c.firmware_ver[i], i, CTPM_FW[i + 18]);
			if((i2c.firmware_ver[i] != CTPM_FW[i + 18]))
			{
				printk("ilitek_upgrade_firmware FW Version is not same upgrade\n"); 
				break;
			}
			else if(i == 3)
			{
				ret = 1;
				printk("ilitek_upgrade_firmware FW Version is same! Do not need update\n"); 
				//return 1;
			}
		}
#endif
	}
#endif 

	#ifndef UPDATE_THREADE
	if (ret == 0) {
		ret = ilitek_upgrade_firmware();
		if(ret==2) printk("update end\n");
		else if(ret==3) printk("i2c communication error\n");

		if(i2c.reset_request_success){
			ilitek_reset(i2c.reset_gpio);
		}

		// read touch parameter
		ret=ilitek_i2c_read_tp_info();
		if(ret < 0)
		{
			return ret;
		}
	}
	ret = 1;
	#else
	
	if (ret == 0) {
		thread_update= kthread_run(ilitek_i2c_update_thread, NULL, "ilitek_i2c_updatethread");
		if(thread_update == (struct task_struct*)ERR_PTR){
			thread_update = NULL;
			printk(ILITEK_ERROR_LEVEL "%s,thread_update kthread create, error\n", __func__);
		}
	}
	#endif
#else
	ret = 1;
#endif
	if ((ret == 1)) {
		// register input device
		ilitek_set_input_param(i2c.input_dev, i2c.max_tp, i2c.max_x, i2c.max_y);
		ret = input_register_device(i2c.input_dev);
		if(ret){
			printk(ILITEK_ERROR_LEVEL "%s, register input device, error\n", __func__);
			return ret;
		}
		printk(ILITEK_ERROR_LEVEL "%s, register input device, success\n", __func__);
	}
	i2c.valid_input_register = 1;
	
	
#ifdef CLOCK_INTERRUPT
#ifdef REPORT_THREAD
	i2c.thread = kthread_run(ilitek_i2c_touchevent_thread, NULL, "ilitek_i2c_thread");
	if(i2c.thread == (struct task_struct*)ERR_PTR){
		i2c.thread = NULL;
		printk(ILITEK_ERROR_LEVEL "%s, kthread create, error\n", __func__);
	}
#endif
#endif

#ifdef GESTURE
	ilitek_system_resume = 1;
	input_set_capability(i2c.input_dev, EV_KEY, KEY_POWER);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_W);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_LEFT);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_RIGHT);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_UP);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_DOWN);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_O);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_C);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_E);
	input_set_capability(i2c.input_dev, EV_KEY, KEY_M);
#endif

#ifdef TOOL
	ret = create_tool_node();
#endif

#ifdef SENSOR_TEST
#ifdef SYS_ATTR_FILE
	ilitek_sensor_test_init();
#endif
#endif

#ifdef HALL_CHECK
#if HALL_CHECK == HALL_CHECK_OTHER
	ilitek_hall_check_init();	
#elif HALL_CHECK == HALL_CHECK_HW
	ilitek_hall_check_hw_init();	
#endif //end of HALL_CHECK
#endif
	Report_Flag=0;

#if defined(CONFIG_FB)
		i2c.fb_notif.notifier_call = fb_notifier_callback;

		ret = fb_register_client(&i2c.fb_notif);

		if (ret)
			dev_err(&i2c.client->dev, "Unable to register fb_notifier: %d\n",
					ret);

#elif defined(CONFIG_HAS_EARLYSUSPEND)
		i2c.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		i2c.early_suspend.suspend = ilitek_i2c_early_suspend;
		i2c.early_suspend.resume = ilitek_i2c_late_resume;
		register_early_suspend(&i2c.early_suspend);
#endif

#ifdef ILITEK_ESD_CHECK
	 INIT_DELAYED_WORK(&esd_work, ilitek_touch_esd_func);
	 esd_wq = create_singlethread_workqueue("esd_wq");	 
	 if (!esd_wq) {
		 return -ENOMEM;
	 }
	 queue_delayed_work(esd_wq, &esd_work, delay);
#endif 

	ilitek_set_finish_init_flag();

#ifdef CONFIG_HY_DRV_ASSIST
	ctp_assist_register_attr("ic",&ilitek_ic_show,NULL);
	ctp_assist_register_attr("fw_ver",&ilitek_fw_ver_show,NULL);
	ctp_assist_register_attr("tp_vendor",&ilitek_tp_vendor_show,NULL);
#endif

#ifdef DEBUG_NETLINK
	netlink_sock = netlink_kernel_create(&init_net,21,&cfg);
#endif
	return 0;
}



/*
   description
   when the i2c device want to detach from adapter, this function will be invoked.
   parameters
   client
   i2c client data
   return
   status
 */
static int ilitek_i2c_remove(struct i2c_client *client)
{
	printk( "%s\n", __func__);
	i2c.stop_polling = 1;
	// delete i2c driver
	if(i2c.client->irq != 0){
		if(i2c.valid_irq_request != 0){
			free_irq(i2c.client->irq, &i2c);
			printk(ILITEK_DEBUG_LEVEL "%s, free irq\n", __func__);
			if(i2c.irq_work_queue){
				destroy_workqueue(i2c.irq_work_queue);
				printk(ILITEK_DEBUG_LEVEL "%s, destory work queue\n", __func__);
			}
		}
#ifdef CLOCK_INTERRUPT
#ifdef REPORT_THREAD
		if(i2c.thread != NULL){
			kthread_stop(i2c.thread);
			printk(ILITEK_DEBUG_LEVEL "%s, stop i2c thread\n", __func__);
		}
#endif
#endif
	}
	else{
		if(i2c.thread != NULL){
			kthread_stop(i2c.thread);
			printk(ILITEK_DEBUG_LEVEL "%s, stop i2c thread\n", __func__);
		}
	}
	if(i2c.valid_input_register != 0){
		input_unregister_device(i2c.input_dev);
		printk(ILITEK_DEBUG_LEVEL "%s, unregister i2c input device\n", __func__);
	}

	// delete character device driver
	#ifdef TOOL
		remove_tool_node();
	#endif
	
	#ifdef SENSOR_TEST
	#ifdef SYS_ATTR_FILE
		ilitek_sensor_test_deinit();
	#endif
	#endif
	#ifdef HALL_CHECK
	#if HALL_CHECK == HALL_CHECK_HW
		ilitek_hall_check_hw_deinit();	
	#endif
	#endif
	printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);
	return 0;
}



/*
   description
   initiali function for driver to invoke.
   parameters

   nothing
   return
   status
 */
static int ilitek_init(void)
{
	int ret = 0;
	DBG("Enter\n");

	printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);

	// register i2c device
	ret = i2c_add_driver(&ilitek_ts_driver);
	if(ret != 0)
	{
		printk(ILITEK_ERROR_LEVEL "%s, add i2c device, error\n", __func__);
		return ret;
	}

	return 0;
}

/*
   description
   driver exit function
   parameters
   none
   return
   nothing
 */
static void ilitek_exit(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&i2c.early_suspend);
#endif
	// delete i2c driver
	if(i2c.client->irq != 0){
		if(i2c.valid_irq_request != 0){
			free_irq(i2c.client->irq, &i2c);
			printk(ILITEK_DEBUG_LEVEL "%s, free irq\n", __func__);
			if(i2c.irq_work_queue){
				destroy_workqueue(i2c.irq_work_queue);
				printk(ILITEK_DEBUG_LEVEL "%s, destory work queue\n", __func__);
			}
		}
		#ifdef CLOCK_INTERRUPT
		#ifdef REPORT_THREAD
		if(i2c.thread != NULL){
			kthread_stop(i2c.thread);
			printk(ILITEK_DEBUG_LEVEL "%s, stop i2c thread\n", __func__);
		}
		#endif
		#endif
	}
	else{
		if(i2c.thread != NULL){
			kthread_stop(i2c.thread);
			printk(ILITEK_DEBUG_LEVEL "%s, stop i2c thread\n", __func__);
		}
	}
	if(i2c.valid_i2c_register != 0){
		i2c_del_driver(&ilitek_ts_driver);
		printk(ILITEK_DEBUG_LEVEL "%s, delete i2c driver\n", __func__);
	}
	if(i2c.valid_input_register != 0){
		input_unregister_device(i2c.input_dev);
		printk(ILITEK_DEBUG_LEVEL "%s, unregister i2c input device\n", __func__);
	}

	// delete character device driver
	#ifdef TOOL
	remove_tool_node();
	#endif
	printk(ILITEK_DEBUG_LEVEL "%s\n", __func__);
}

/* set init and exit function for this module */
module_init(ilitek_init);
module_exit(ilitek_exit);

// module information
MODULE_AUTHOR("Steward_Fu");
MODULE_DESCRIPTION("ILITEK I2C touch driver for Android platform");
MODULE_LICENSE("GPL");


#include "ilitek_ts.h"

#ifdef DEBUG_NETLINK
extern bool debug_flag;
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

	 struct dev_data dev_ilitek;
	 struct proc_dir_entry * proc_ilitek;
	 int create_tool_node(void);
	 int remove_tool_node(void);
	extern int driver_information[];
	extern char Report_Flag;

	extern volatile char int_Flag;
	extern volatile char update_Flag;
	extern int update_timeout;


	extern char EXCHANG_XY;
	extern char REVERT_X;
	extern char REVERT_Y;
	extern char DBG_FLAG,DBG_COR;
	extern struct i2c_data i2c;
	#ifdef GESTURE
	extern int ilitek_system_resume;
	#if GESTURE == GESTURE_DRIVER
	extern int getstatus;
	extern short ilitek_gesture_model_value_x(int i);
	extern short ilitek_gesture_model_value_y(int i);
	extern void ilitek_readgesturelist(void);
	#endif
	#endif
// define the application command
#define ILITEK_IOCTL_BASE                       100
#define ILITEK_IOCTL_I2C_WRITE_DATA             _IOWR(ILITEK_IOCTL_BASE, 0, unsigned char*)
#define ILITEK_IOCTL_I2C_WRITE_LENGTH           _IOWR(ILITEK_IOCTL_BASE, 1, int)
#define ILITEK_IOCTL_I2C_READ_DATA              _IOWR(ILITEK_IOCTL_BASE, 2, unsigned char*)
#define ILITEK_IOCTL_I2C_READ_LENGTH            _IOWR(ILITEK_IOCTL_BASE, 3, int)
#define ILITEK_IOCTL_USB_WRITE_DATA             _IOWR(ILITEK_IOCTL_BASE, 4, unsigned char*)
#define ILITEK_IOCTL_USB_WRITE_LENGTH           _IOWR(ILITEK_IOCTL_BASE, 5, int)
#define ILITEK_IOCTL_USB_READ_DATA              _IOWR(ILITEK_IOCTL_BASE, 6, unsigned char*)
#define ILITEK_IOCTL_USB_READ_LENGTH            _IOWR(ILITEK_IOCTL_BASE, 7, int)

#define ILITEK_IOCTL_DRIVER_INFORMATION		    _IOWR(ILITEK_IOCTL_BASE, 8, int)
#define ILITEK_IOCTL_USB_UPDATE_RESOLUTION      _IOWR(ILITEK_IOCTL_BASE, 9, int)

#define ILITEK_IOCTL_I2C_INT_FLAG	            _IOWR(ILITEK_IOCTL_BASE, 10, int)
#define ILITEK_IOCTL_I2C_UPDATE                 _IOWR(ILITEK_IOCTL_BASE, 11, int)
#define ILITEK_IOCTL_STOP_READ_DATA             _IOWR(ILITEK_IOCTL_BASE, 12, int)
#define ILITEK_IOCTL_START_READ_DATA            _IOWR(ILITEK_IOCTL_BASE, 13, int)
#define ILITEK_IOCTL_GET_INTERFANCE				_IOWR(ILITEK_IOCTL_BASE, 14, int)//default setting is i2c interface
#define ILITEK_IOCTL_I2C_SWITCH_IRQ				_IOWR(ILITEK_IOCTL_BASE, 15, int)

#define ILITEK_IOCTL_UPDATE_FLAG				_IOWR(ILITEK_IOCTL_BASE, 16, int)
#define ILITEK_IOCTL_I2C_UPDATE_FW				_IOWR(ILITEK_IOCTL_BASE, 18, int)
#define ILITEK_IOCTL_RESET						_IOWR(ILITEK_IOCTL_BASE, 19, int)
#define ILITEK_IOCTL_INT_STATUS						_IOWR(ILITEK_IOCTL_BASE, 20, int)

#ifdef DEBUG_NETLINK
#define ILITEK_IOCTL_DEBUG_SWITCH						_IOWR(ILITEK_IOCTL_BASE, 22, int)
#endif

#ifdef GESTURE
#define ILITEK_IOCTL_I2C_GESTURE_FLAG			_IOWR(ILITEK_IOCTL_BASE, 26, int)
#define ILITEK_IOCTL_I2C_GESTURE_RETURN			_IOWR(ILITEK_IOCTL_BASE, 27, int)
#define ILITEK_IOCTL_I2C_GET_GESTURE_MODEL		_IOWR(ILITEK_IOCTL_BASE, 28, int)
#define ILITEK_IOCTL_I2C_LOAD_GESTURE_LIST		_IOWR(ILITEK_IOCTL_BASE, 29, int)
#endif

extern int out_use;

/*
   description
   open function for character device driver
   prarmeters
   inode
   inode
   filp
   file pointer
   return
   status
 */
static int ilitek_file_open(struct inode *inode, struct file *filp)
{
	out_use = 1;
	DBG("%s out_use = %d \n",__func__, out_use);
	return 0;
}
/*
   description
   calibration function
   prarmeters
   count
   buffer length
   return
   status
 */
static int ilitek_i2c_calibration(size_t count)
{

	int ret;
	unsigned char buffer[128]={0};
	struct i2c_msg msgs[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = count, .buf = buffer,}
	};

	buffer[0] = ILITEK_TP_CMD_ERASE_BACKGROUND;
	msgs[0].len = 1;
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	if(ret < 0){
		printk(ILITEK_DEBUG_LEVEL "%s, i2c erase background, failed\n", __func__);
	}
	else{
		printk(ILITEK_DEBUG_LEVEL "%s, i2c erase background, success\n", __func__);
	}

	buffer[0] = ILITEK_TP_CMD_CALIBRATION;
	msgs[0].len = 1;
	msleep(2000);
	ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
	msleep(1000);
	return ret;
}
/*
   description
   read calibration status
   prarmeters
   count
   buffer length
   return
   status
 */
static int ilitek_i2c_calibration_status(size_t count)
{
	int ret;
	unsigned char buffer[128]={0};
	struct i2c_msg msgs[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = count, .buf = buffer,}
	};
	buffer[0] = ILITEK_TP_CMD_CALIBRATION_STATUS;
	ilitek_i2c_transfer(i2c.client, msgs, 1);
	msleep(500);
	buffer[0] = ILITEK_TP_CMD_CALIBRATION_STATUS;
	ilitek_i2c_write_and_read(i2c.client, buffer, 1, 10, buffer, 4);
	printk("%s, i2c calibration status:0x%X\n",__func__,buffer[0]);
	ret=buffer[0];
	return ret;
}
/*
   description
   write function for character device driver
   prarmeters
   filp
   file pointer
   buf
   buffer
   count
   buffer length
   f_pos
   offset
   return
   status
 */
static ssize_t ilitek_file_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int ret;
	unsigned char buffer[128]={0};

	// before sending data to touch device, we need to check whether the device is working or not
	if(i2c.valid_i2c_register == 0){
		printk(ILITEK_ERROR_LEVEL "%s, i2c device driver doesn't be registered\n", __func__);
		return -1;
	}

	// check the buffer size whether it exceeds the local buffer size or not
	if(count > 128){
		printk(ILITEK_ERROR_LEVEL "%s, buffer exceed 128 bytes\n", __func__);
		return -1;
	}

	// copy data from user space
	ret = copy_from_user(buffer, buf, count-1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, copy data from user space, failed", __func__);
		return -1;
	}

	// parsing command
	if(strcmp(buffer, "calibrate") == 0){
		ret=ilitek_i2c_calibration(count);
		if(ret < 0){
			printk(ILITEK_DEBUG_LEVEL "%s, i2c send calibration command, failed\n", __func__);
		}
		else{
			printk(ILITEK_DEBUG_LEVEL "%s, i2c send calibration command, success\n", __func__);
		}
		ret=ilitek_i2c_calibration_status(count);
		if(ret == 0x5A){
			printk(ILITEK_DEBUG_LEVEL "%s, i2c calibration, success\n", __func__);
		}
		else if (ret == 0xA5){
			printk(ILITEK_DEBUG_LEVEL "%s, i2c calibration, failed\n", __func__);
		}
		else{
			printk(ILITEK_DEBUG_LEVEL "%s, i2c calibration, i2c protoco failed\n", __func__);
		}
		return count;
	}else if(strcmp(buffer, "dbg") == 0){
		DBG_FLAG=!DBG_FLAG;
		printk("%s, %s DBG message(%X).\n",__func__,DBG_FLAG?"Enabled":"Disabled",DBG_FLAG);
	}
	#ifdef DEBUG_NETLINK
	else if(strcmp(buffer, "dbg_flag") == 0){
		debug_flag =!debug_flag;
		printk("%s, %s debug_flag message(%X).\n",__func__,debug_flag?"Enabled":"Disabled",debug_flag);
	}
	else if(strcmp(buffer, "enable") == 0){
		enable_irq(i2c.client->irq);
		printk("%s, %s enable_irq(i2c.client->irq)DBG message(%X).\n",__func__,DBG_FLAG?"Enabled":"Disabled",DBG_FLAG);
	}else if(strcmp(buffer, "disable") == 0){
		disable_irq(i2c.client->irq);
		printk("%s, %s disable_irq(i2c.client->irq) DBG message(%X).\n",__func__,DBG_FLAG?"Enabled":"Disabled",DBG_FLAG);
	}else if(strcmp(buffer, "irq_status") == 0){
		printk("%s, gpio_get_value(i2c.irq_gpio) = %d.\n",__func__, gpio_get_value(i2c.irq_gpio));
	}
	#endif
#ifdef GESTURE
	else if(strcmp(buffer, "gesture") == 0){
		ilitek_system_resume =!ilitek_system_resume;
		printk("ilitek ilitek_system_resume = %d\n", ilitek_system_resume);
	}
#endif
	else if(strcmp(buffer, "dbgco") == 0){
		DBG_COR=!DBG_COR;
		printk("%s, %s DBG COORDINATE message(%X).\n",__func__,DBG_COR?"Enabled":"Disabled",DBG_COR);
	}else if(strcmp(buffer, "info") == 0){
		ilitek_i2c_read_tp_info();
	}else if(strcmp(buffer, "report") == 0){
		Report_Flag=!Report_Flag;
	}else if(strcmp(buffer, "chxy") == 0){
		EXCHANG_XY=!EXCHANG_XY;
	}else if(strcmp(buffer, "revx") == 0){
		REVERT_X=!REVERT_X;
	}else if(strcmp(buffer, "revy") == 0){
		REVERT_Y=!REVERT_Y;
	}else if(strcmp(buffer, "suspd") == 0){
		pm_message_t pmsg;
		pmsg.event = 0;
		ilitek_i2c_suspend(i2c.client, pmsg);
	}else if(strcmp(buffer, "resm") == 0){
		ilitek_i2c_resume(i2c.client);
	}
	else if(strcmp(buffer, "stop_report") == 0){
		i2c.report_status = 0;
		printk("The report point function is disable.\n");
	}else if(strcmp(buffer, "start_report") == 0){
		i2c.report_status = 1;
		printk("The report point function is enable.\n");
	}else if(strcmp(buffer, "update_flag") == 0){
		printk("update_Flag=%d\n",update_Flag);
	}else if(strcmp(buffer, "reset") == 0){
		printk("start reset\n");
		if(i2c.reset_request_success)
			ilitek_reset(i2c.reset_gpio);
		printk("end reset\n");
	}
	printk("ilitek return count = %d\n", (int)count);
	return count;
}

/*
   description
   ioctl function for character device driver
   prarmeters
   inode
   file node
   filp
   file pointer
   cmd
   command
   arg
   arguments
   return
   status
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static long ilitek_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int  ilitek_file_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	static unsigned char buffer[64]={0};
	static int len = 0, i;
	int ret;
	struct i2c_msg msgs[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = len, .buf = buffer,}
	};

	// parsing ioctl command
	switch(cmd){
		case ILITEK_IOCTL_I2C_WRITE_DATA:
			ret = copy_from_user(buffer, (unsigned char*)arg, len);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, copy data from user space, failed\n", __func__);
				return -1;
			}
			ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c write, failed\n", __func__);
				return -1;
			}
			break;
		case ILITEK_IOCTL_I2C_READ_DATA:
			msgs[0].flags = I2C_M_RD;

			ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c read, failed\n", __func__);
				return -1;
			}
			ret = copy_to_user((unsigned char*)arg, buffer, len);

			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, copy data to user space, failed\n", __func__);
				return -1;
			}
			break;
		case ILITEK_IOCTL_I2C_WRITE_LENGTH:
		case ILITEK_IOCTL_I2C_READ_LENGTH:
			len = arg;
			break;
		case ILITEK_IOCTL_DRIVER_INFORMATION:
			for(i = 0; i < 7; i++){
				buffer[i] = driver_information[i];
			}
			ret = copy_to_user((unsigned char*)arg, buffer, 7);
			break;
		case ILITEK_IOCTL_I2C_UPDATE:
			break;
		case ILITEK_IOCTL_I2C_INT_FLAG:
			if(update_timeout == 1){
				buffer[0] = int_Flag;
				ret = copy_to_user((unsigned char*)arg, buffer, 1);
				if(ret < 0){
					printk(ILITEK_ERROR_LEVEL "%s, copy data to user space, failed\n", __func__);
					return -1;
				}
			}
			else
				update_timeout = 1;

			break;
		case ILITEK_IOCTL_START_READ_DATA:
			i2c.stop_polling = 0;
			if(i2c.client->irq != 0 )
				ilitek_i2c_irq_enable();
			i2c.report_status = 1;
			printk("The report point function is enable.\n");
			break;
		case ILITEK_IOCTL_STOP_READ_DATA:
			i2c.stop_polling = 1;
			if(i2c.client->irq != 0 )
				ilitek_i2c_irq_disable();
			i2c.report_status = 0;
			printk("The report point function is disable.\n");
			break;
		case ILITEK_IOCTL_RESET:
			ilitek_reset(i2c.reset_gpio);
			break;
		case ILITEK_IOCTL_INT_STATUS:
			put_user(gpio_get_value(i2c.irq_gpio), (int *)arg);
			break;	
		case ILITEK_IOCTL_I2C_SWITCH_IRQ:
			ret = copy_from_user(buffer, (unsigned char*)arg, 1);
			if (buffer[0] == 0)
			{
				if(i2c.client->irq != 0 ){
					printk("ilitek_i2c_irq_disable ready.  \n");
					ilitek_i2c_irq_disable();
					
					//gpio_direction_output(i2c.irq_gpio,0);
				}
			}
			else
			{
				if(i2c.client->irq != 0 ){
					printk("ilitek_i2c_irq_enable ready. \n");
					ilitek_i2c_irq_enable();
					
					//gpio_direction_input(i2c.irq_gpio);
				}
			}
			break;
		#ifdef DEBUG_NETLINK
		case ILITEK_IOCTL_DEBUG_SWITCH:
			ret = copy_from_user(buffer, (unsigned char*)arg, 1);
			printk("ilitek The debug_flag = %d.\n", buffer[0]);
			if (buffer[0] == 0)
			{
				debug_flag = false;
			}
			else if (buffer[0] == 1)
			{
				debug_flag = true;
			}
			break;
		#endif
		case ILITEK_IOCTL_UPDATE_FLAG:
			update_timeout = 1;
			update_Flag = arg;
			DBG("%s,update_Flag=%d\n",__func__,update_Flag);
			break;
		case ILITEK_IOCTL_I2C_UPDATE_FW:
			ret = copy_from_user(buffer, (unsigned char*)arg, 35);
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, copy data from user space, failed\n", __func__);
				return -1;
			}
			int_Flag = 0;
			update_timeout = 0;
			msgs[0].len = buffer[34];
			ret = ilitek_i2c_transfer(i2c.client, msgs, 1);
#ifndef CLOCK_INTERRUPT
			ilitek_i2c_irq_enable();
#endif
			if(ret < 0){
				printk(ILITEK_ERROR_LEVEL "%s, i2c write, failed\n", __func__);
				return -1;
			}
			break;

			#ifdef GESTURE
			case ILITEK_IOCTL_I2C_GESTURE_FLAG:
				ilitek_system_resume = arg;
				printk("%s,gesture_flag=%d\n",__func__,ilitek_system_resume);
				break;
			#if GESTURE == GESTURE_DRIVER
			case ILITEK_IOCTL_I2C_GESTURE_RETURN:
				buffer[0] = getstatus;
				printk("%s,getstatus=%d\n",__func__,getstatus);
				ret = copy_to_user((unsigned char*)arg, buffer, 1);
				if(ret < 0){
					printk(ILITEK_ERROR_LEVEL "%s, copy data to user space, failed\n", __func__);
					return -1;
				}
				//getstatus = 0;
				break;
			#if GESTURE_FUN==GESTURE_FUN_1
			case ILITEK_IOCTL_I2C_GET_GESTURE_MODEL:
				for(i = 0; i<32 ;i=i+2){
					buffer[i] = ilitek_gesture_model_value_x(i/2);
					buffer[i+1]= ilitek_gesture_model_value_y(i/2);
					printk("x[%d]=%d,y[%d]=%d\n",i/2,buffer[i],i/2,buffer[i+1]);
				}
				ret = copy_to_user((unsigned char*)arg, buffer, 32);
				if(ret < 0){
					printk(ILITEK_ERROR_LEVEL "%s, copy data to user space, failed\n", __func__);
					return -1;
				}
				//getstatus = 0;
				break;
			case ILITEK_IOCTL_I2C_LOAD_GESTURE_LIST:
				printk("start\n");
				ilitek_readgesturelist();
				printk("end--------------\n");
				break;
			#endif
			#endif
			#endif				
		default:
			return -1;
	}
	return 0;
}

/*
   description
   read function for character device driver
   prarmeters
   filp
   file pointer
   buf
   buffer
   count
   buffer length
   f_pos
   offset
   return
   status
 */
static ssize_t ilitek_file_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

/*
   description
   close function
   prarmeters
   inode
   inode
   filp
   file pointer
   return
   status
 */
static int ilitek_file_close(struct inode *inode, struct file *filp)
{
	out_use = 0;
	DBG("%s out_use = %d \n",__func__, out_use);
	return 0;
}

// declare file operations
struct file_operations ilitek_fops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
	.unlocked_ioctl = ilitek_file_ioctl,
#else
	.ioctl = ilitek_file_ioctl,
#endif
	.read = ilitek_file_read,
	.write = ilitek_file_write,
	.open = ilitek_file_open,
	.release = ilitek_file_close,
};

int create_tool_node(void) {
	int ret = 0;
	// allocate character device driver buffer
	ret = alloc_chrdev_region(&dev_ilitek.devno, 0, 1, ILITEK_FILE_DRIVER_NAME);
	if(ret){
		printk(ILITEK_ERROR_LEVEL "%s, can't allocate chrdev\n", __func__);
		return ret;
	}
	printk(ILITEK_DEBUG_LEVEL "%s, register chrdev(%d, %d)\n", __func__, MAJOR(dev_ilitek.devno), MINOR(dev_ilitek.devno));

	// initialize character device driver
	cdev_init(&dev_ilitek.cdev, &ilitek_fops);
	dev_ilitek.cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_ilitek.cdev, dev_ilitek.devno, 1);
	if(ret < 0){
		printk(ILITEK_ERROR_LEVEL "%s, add character device error, ret %d\n", __func__, ret);
		return ret;
	}
	dev_ilitek.class = class_create(THIS_MODULE, ILITEK_FILE_DRIVER_NAME);
	if(IS_ERR(dev_ilitek.class)){
		printk(ILITEK_ERROR_LEVEL "%s, create class, error\n", __func__);
		return ret;
	}
	device_create(dev_ilitek.class, NULL, dev_ilitek.devno, NULL, "ilitek_ctrl");
	proc_ilitek = proc_create("ilitek_ctrl", 0666, NULL, &ilitek_fops);
	if(proc_ilitek == NULL) {
		printk("111111111 proc_create(ilitek_ctrl, 0666, NULL, &ilitek_fops) fail\n");
	}
	return 0;
}

int remove_tool_node(void) {
	cdev_del(&dev_ilitek.cdev);
	unregister_chrdev_region(dev_ilitek.devno, 1);
	device_destroy(dev_ilitek.class, dev_ilitek.devno);
	class_destroy(dev_ilitek.class);
	if (proc_ilitek) {
		proc_remove(proc_ilitek);
	}
	return 0;
}
#endif

/**
 * gf_milanf.c
 * GOODIX Milan-F Chip driver code
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <asm/uaccess.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/completion.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/notifier.h> //add for fb
#include <linux/fb.h>       //add for fb
#include "gf-common.h"
#include "gf-milanf.h"
#include "gf-regs.h"
#include "gf-configs.h"
#include "linux/of_gpio.h"
#include "linux/regulator/consumer.h"

/************ Micro Definitions **********/
#define SPI_DEV_NAME            "spidev"
#define GOODIX_DEV_NAME         "goodix_fp"
#define	CHRD_DRIVER_NAME		"goodix_fp_spi"
#define	CLASS_NAME				"goodix_fp"
#define SPIDEV_MAJOR			 154	
#define N_SPI_MINORS			 32	
#define GF_FASYNC                1
#define SENSOR_COL               108
#define SENSOR_ROW               88
#define GF_INPUT_NAME            "tiny4412-key"
/************ Global Variables **********/
static struct class *gf_spi_class;
static unsigned bufsiz = 14260; //88*108*1.5+4 Bytes
static unsigned char g_frame_buf[14260]={0};
static unsigned short g_vendorID;
static unsigned char g_sendorID[16] = {0};
static unsigned short g_fdt_delta = 0;
static unsigned short g_tcode_img = 0;
static DECLARE_BITMAP(minors, N_SPI_MINORS);
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static int g_irq_enabled = 1;
//static int g_screem_on = 0; //fb add
static int g_reset_flag = 0;
static unsigned short g_vendorID;

struct gf_key_map key_map[] =
{
	{  "POWER",  KEY_POWER  },
#ifdef CONFIG_TP_HAVE_KEY
	{  "HOME" ,  KEY_HOME   },
	{  "MENU" ,  KEY_MENU   },
	{  "BACK" ,  KEY_BACK   },
#endif
	{  "UP"   ,  KEY_UP     },
	{  "DOWN" ,  KEY_DOWN   },
	{  "LEFT" ,  KEY_LEFT   },
	{  "RIGHT",  KEY_RIGHT  },
	{  "FORCE",  KEY_F9     },
	{  "CLICK",  KEY_F19    },
};

static void gf_reg_key_kernel(gf_dev_t *gf_dev)
{
	int i;

	set_bit(EV_KEY, gf_dev->input->evbit); //tell the kernel is key event
	for(i = 0; i< ARRAY_SIZE(key_map); i++) {
		set_bit(key_map[i].val, gf_dev->input->keybit);
	}

	gf_dev->input->name = GF_INPUT_NAME;
	if (input_register_device(gf_dev->input))
		printk(KERN_ERR "Failed to register GF as input device.\n");
}

void readOTPByte(gf_dev_t* gf_dev,unsigned short usBank,
					unsigned short usAddr,unsigned char* pValue)
{
    unsigned short data = 0;
    gf_spi_write_word(gf_dev,GF_OTP_ADDR1,0x0020);
    gf_spi_write_word(gf_dev,GF_OTP_ADDR2,usBank);
    gf_spi_write_word(gf_dev,GF_OTP_ADDR1,0x0021);
    gf_spi_write_word(gf_dev,GF_OTP_ADDR1,0x0020);

    gf_spi_write_word(gf_dev,GF_OTP_ADDR2,((usAddr<<8) & (0xFFFF)));
    gf_spi_write_word(gf_dev,GF_OTP_ADDR1,0x0022);
    gf_spi_write_word(gf_dev,GF_OTP_ADDR1,0x0020);
    gf_spi_read_word(gf_dev,GF_OTP_ADDR3,&data);

    *pValue = data & 0xFF;
}

void gf_parse_otp(gf_dev_t* gf_dev,unsigned char* pSensorID)
{
    unsigned short i = 0;
    unsigned char ucOTP[32] = {0};
	unsigned char Tcode,Diff;
	unsigned short Tcode_use,Diff_use,Diff_256;
	
    for(i=0;i<32;i++)
    {
        readOTPByte(gf_dev,(i>>2),(i & 0x03),ucOTP+i);
		pr_info("OTP_BYTE[%d] = 0x%x \n",i,ucOTP[i]);
    }
			
    memcpy(pSensorID,ucOTP,16);
	Tcode = (ucOTP[22] & 0xF0) >> 4;
	Diff = ucOTP[22] & 0x0F;

	if((Tcode != 0) && (Diff != 0) && ((ucOTP[22] & ucOTP[23]) == 0)) 
	{
		Tcode_use = (Tcode + 1)*16;
		Diff_use = (Diff + 2)*100;

		Diff_256 = (Diff_use * 256) / Tcode_use;
		pr_info("%s Tcode:0x%x(%d),Diff:0x%x(%d)\n",__func__,Tcode,Tcode,Diff,Diff);
		pr_info("%s Tcode_use:0x%x(%d),Diff_use:0x%x(%d),Diff_256:0x%x(%d)\n",
			        __func__,Tcode_use,Tcode_use,Diff_use,Diff_use,Diff_256,Diff_256);
		g_fdt_delta = (((Diff_256/3)>>4)<<8)|0x7F;
		g_tcode_img = Tcode_use;		
	}
	pr_info("%s g_tcode_img:0x%x,g_fdt_delta:0x%x",__func__,g_tcode_img,g_fdt_delta);
} 


static int gf_write_configs(gf_dev_t *gf_dev,struct gf_configs* config,int len)
{
	int cnt;
	int length = len;
	int ret = 0;
	for(cnt=0;cnt< length;cnt++)
	{		
		ret = gf_spi_write_word(gf_dev,config[cnt].addr,config[cnt].value);
		if(ret < 0){
			printk(KERN_ERR "%s failed. \n",__func__);
			return ret;
		}
	}

	return 0;
}
static void gf_irq_cfg(gf_dev_t* gf_dev)
{
	gpio_request_one(gf_dev->irq_gpio, GPIOF_IN, "gf_irq");
	gpio_free(gf_dev->irq_gpio);
}

static void gf_enable_irq(gf_dev_t* gf_dev)
{
	if(g_irq_enabled) {
		printk(KERN_ERR "%s irq has been enabled.\n",__func__);
	} else {
		enable_irq(gf_dev->irq);
		g_irq_enabled = 1;
	}
}

static void gf_disable_irq(gf_dev_t* gf_dev)
{
	if(g_irq_enabled) {
		g_irq_enabled = 0;
		disable_irq(gf_dev->irq);
	} else {
		printk(KERN_ERR "%s irq has been disabled.\n",__func__);
	}
}

//fb add start
/*
   static int goodix_fb_state_chg_callback(struct notifier_block *nb,
   unsigned long val, void *data)
   {
   gf_dev_t *gf_dev;
   struct fb_event *evdata = data;
   unsigned int blank;
   unsigned char idle_cmd = 0xc0;
   unsigned char ff_cmd = 0xc4;

   if (val != FB_EARLY_EVENT_BLANK)
   return 0;
   printk(KERN_ERR "%s go to the goodix_fb_state_chg_callback value = %d\n",
   __func__, (int)val);
   gf_dev = container_of(nb, gf_dev_t, notifier);
   if (evdata && evdata->data && val == FB_EARLY_EVENT_BLANK && gf_dev) {
   blank = *(int *)(evdata->data);
   switch (blank) {
   case FB_BLANK_POWERDOWN:
   g_screem_on = 1;
   printk(KERN_ERR "%s Screem off, and write cfg, send cmd. \n",__func__);

   gf_spi_send_cmd(gf_dev,&idle_cmd, 1);
   mdelay(2);
   gf_write_configs(gf_dev,ff_cfg,sizeof(ff_cfg)/sizeof(struct gf_configs));
   gf_spi_send_cmd(gf_dev,&ff_cmd, 1);
   mdelay(1);

   break;
   case FB_BLANK_UNBLANK:
   g_screem_on = 0;
   printk(KERN_ERR "%s Screem on.\n",__func__);
   break;
   default:
   printk(KERN_ERR "%s defalut\n", __func__);
   break;
   }
   }
   return NOTIFY_OK;
   }*/
//fb add end

static void gf_hw_reset(gf_dev_t* gf_dev, u8 delay_ms)
{
	/**
	 *  MilanF chip reset condition: reset pin low duration 1 ms
	 */
    gpio_request_one(gf_dev->rst_gpio,GPIOF_OUT_INIT_HIGH, "gf_rst");
    gpio_set_value(gf_dev->rst_gpio, 1);
    
    gpio_set_value(gf_dev->rst_gpio, 0);
	//mdelay(1);
	mdelay(3);
    gpio_set_value(gf_dev->rst_gpio, 1);
	//mdelay(1);
}

/************* File Operations interfaces ********/
static ssize_t gf_read(struct file *filp, char __user *buf, 
		size_t count,loff_t *f_pos)
{
	gf_dev_t *gf_dev = filp->private_data;
	int status = 0;
	int len = 0;

	if(buf == NULL || count > bufsiz)
	{
		printk(KERN_ERR "%s input parameters invalid. bufsiz = %d,count = %d \n",
				__func__,bufsiz,(int)count);
		return -EMSGSIZE;
	}

	len = gf_spi_read_data(gf_dev,0xAAAA,count,g_frame_buf);
	//printk(KERN_ERR "%s read length = %d \n",__func__,len);
	status = copy_to_user(buf, g_frame_buf, count);
	if(status != 0) {
		printk(KERN_ERR "%s copy_to_user failed. status = %d \n",__func__,status);
		return -EFAULT;
	}
	return 0;
}

static ssize_t gf_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	return 0;
#if 0
	gf_dev_t *gf_dev = filp->private_data;
	int ret;
	unsigned char* tmp_buf = kzalloc(count,GFP_KERNEL);
	unsigned short* cfg_buf = tmp_buf;

	if(cfg_buf == NULL) {
		printk(KERN_ERR "%s alloc buf failed.\n",__func__);
		return -1;
	}

	ret = copy_from_user(tmp_buf, buf, count);
	if(ret != 0) {
		printk(KERN_ERR "%s copy_from_user failed.\n", __func__);
		return -1;
	}
	printk(KERN_ERR "\n");
	for(ret = 0; ret<count;ret++)
		printk(KERN_ERR "0x%x \n", tmp_buf[ret]);

	printk(KERN_ERR "\n");

	printk(KERN_ERR "\n");
	for(ret = 0; ret<count/2;ret++)
		printk(KERN_ERR "0x%x \n", cfg_buf[ret]);

	printk(KERN_ERR "\n");

	return 0;
#endif
}

static long gf_ioctl(struct file *filp, unsigned int cmd, 
		unsigned long arg)
{
	gf_dev_t *gf_dev = NULL;
	struct gf_ioc_transfer *ioc = NULL;
	struct gf_key gf_key={0};
	u8* tmpbuf = NULL;  
	int ret = 0; 
	int retval = 0;
	int err = 0;
	unsigned char command = 0;
	unsigned char config_type = 0;
	int i;
	struct gf_configs* p_cfg = NULL;
	unsigned char cfg_len = 0;
			
	//printk(KERN_ERR "%s enter \n", __func__);
	if (_IOC_TYPE(cmd) != GF_IOC_MAGIC)
		return -ENOTTY;
	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	gf_dev = (gf_dev_t *)filp->private_data;
#if 0 //for debug
	printk(KERN_ERR "%s CMD = 0x%x,_IOC_DIR:0x%x,_IOC_TYPE:0x%x,IOC_NR:0x%x,IOC_SIZE:0x%x \n"
			,__func__,cmd,_IOC_DIR(cmd),_IOC_TYPE(cmd),_IOC_NR(cmd),_IOC_SIZE(cmd));
	/*if(_IOC_DIR(cmd) == 0x3){
		printk(KERN_ERR "%s In kernel sizeof(*ioc) = 0x%x,From user space sizeof(*ioc) = 0x%x \n",
				__func__,sizeof(*ioc),_IOC_SIZE(cmd));
	}*/
#endif //for debug
	switch(cmd) 
	{
		case GF_IOC_RW:
			ioc = kzalloc(sizeof(*ioc), GFP_KERNEL);
			if(ioc == NULL) {
				printk(KERN_ERR "kzalloc ioc failed. \n");
				retval = -ENOMEM;
				break;
			}

			/*copy command data from user to kernel.*/
			if(copy_from_user(ioc, (struct gf_ioc_transfer*)arg, sizeof(*ioc))){
				printk(KERN_ERR "Failed to copy command from user to kernel.\n");
				retval = -EFAULT;
				break;
			}

			tmpbuf = kzalloc(ioc->len, GFP_KERNEL);
			if(tmpbuf == NULL) {
				printk(KERN_ERR "kzalloc buf failed. \n");
				retval = -ENOMEM;
				break;
			}
			if((ioc->len > bufsiz)||(ioc->len == 0)) {
				printk(KERN_ERR "%s The request length[%d] is longer than supported maximum buffer length[%d].\n"
						,__func__,ioc->len,bufsiz);
				retval = -EMSGSIZE;
				break;
			}


			if(ioc->cmd == GF_R) {
				//printk(KERN_ERR "%s Read data ioc->addr= 0x%x, ioc->len = 0x%x,ioc->buf = 0x%x\n"
				//				 ,__func__,ioc->addr, ioc->len,ioc->buf);

				mutex_lock(&gf_dev->frame_lock);
				gf_spi_read_data(gf_dev, ioc->addr, ioc->len, tmpbuf);
				mutex_unlock(&gf_dev->frame_lock);

				mutex_lock(&gf_dev->buf_lock);
			ret = copy_to_user((void __user *)ioc->buf, tmpbuf, ioc->len);
				mutex_unlock(&gf_dev->buf_lock);

				if(ret) {
					printk(KERN_ERR "%s Failed to copy data from kernel to user.\n",__func__);
					retval = -EFAULT;
					break;
				}
			} else if (ioc->cmd == GF_W) {
				//printk(KERN_ERR "%s Write data from 0x%x, len = 0x%x\n",__func__,ioc->addr,ioc->len);
			ret = copy_from_user(tmpbuf, (void __user *)ioc->buf, ioc->len);

				if(ret){
					printk(KERN_ERR "%s Failed to copy data from user to kernel.\n",__func__);
					retval = -EFAULT;
					break;
				}
				mutex_lock(&gf_dev->frame_lock);
				gf_spi_write_data(gf_dev, ioc->addr, ioc->len, tmpbuf);
				mutex_unlock(&gf_dev->frame_lock);

			} else {
				printk(KERN_ERR "%s Error command for ioc->cmd.\n",__func__);	
				retval = -EFAULT;
			}	    
			break;
		case GF_IOC_CMD:
			//printk(KERN_ERR "%s GF_IOC_CMD \n",__func__);
			retval = __get_user(command ,(u32 __user*)arg);
			//printk(KERN_ERR "%s GF_IOC_CMD command is %x \n",__func__,command);
			gf_spi_send_cmd(gf_dev,&command,1);
		mdelay(1);
			break;
		case GF_IOC_CONFIG:
			retval = __get_user(config_type, (u32 __user*)arg);
			//add by pinot
			printk("%s first config_type is %d \n",__func__,config_type);
			if(config_type == CONFIG_FDT_DOWN){
				if(g_vendorID == 0x0000)
				{
					p_cfg = v0_fdt_down_cfg;
					cfg_len = sizeof(v0_fdt_down_cfg)/sizeof(struct gf_configs);
				}
			gf_write_configs(gf_dev,p_cfg,cfg_len);			
			break;
		}
		else if(config_type == CONFIG_NAV_FDT_DOWN) {
		    if(g_vendorID == 0x0000)
			{
				p_cfg = v0_nav_fdt_down_cfg;
				cfg_len = sizeof(v0_nav_fdt_down_cfg)/sizeof(struct gf_configs);
			}
				gf_write_configs(gf_dev,p_cfg,cfg_len);
				break;
			}
			else if(config_type == CONFIG_FDT_UP){
				if(g_vendorID == 0x0000)
				{
					p_cfg = v0_fdt_up_cfg;
					cfg_len = sizeof(v0_fdt_up_cfg)/sizeof(struct gf_configs);
				}
				gf_write_configs(gf_dev,p_cfg,cfg_len);
				break;
			}
			else if(config_type == CONFIG_FF){
				if(g_vendorID == 0x0000)
				{
					p_cfg = v0_ff_cfg;
					cfg_len = sizeof(v0_ff_cfg)/sizeof(struct gf_configs);
				}
				gf_write_configs(gf_dev,p_cfg,cfg_len);
				printk("%s config_type_FF is ok %d \n",__func__,config_type);
				break;
			}
			else if(config_type == CONFIG_NAV){
				if(g_vendorID == 0x0000)
				{
					p_cfg = v0_nav_cfg;
					cfg_len = sizeof(v0_nav_cfg)/sizeof(struct gf_configs);
				}
				gf_write_configs(gf_dev,p_cfg,cfg_len);
				break;
			}
			else if(config_type == CONFIG_IMG){
				if(g_vendorID == 0x0000)
				{
					p_cfg = v0_img_cfg;
					cfg_len = sizeof(v0_img_cfg)/sizeof(struct gf_configs);
				}
				gf_write_configs(gf_dev,p_cfg,cfg_len);
				break;
			}
			else if(config_type == CONFIG_NAV_IMG){			
				if(g_vendorID == 0x0000)
				{
					p_cfg = v0_nav_img_cfg;
					cfg_len = sizeof(v0_nav_img_cfg)/sizeof(struct gf_configs);
				}
				gf_write_configs(gf_dev,p_cfg,cfg_len);
				break;
			}
			else if(config_type == GF_CFG_NAV_IMG_MAN) {
				if(g_vendorID == 0x0000)
				{
					p_cfg = v0_nav_img_manual_cfg;
					cfg_len = sizeof(v0_nav_img_manual_cfg)/sizeof(struct gf_configs);
				}
				gf_write_configs(gf_dev,p_cfg,cfg_len);
				break;
			}
			else{
				pr_info("%s unknow config_type is %d \n",__func__,config_type);
				break;
			}
		case GF_IOC_RESET:
			printk(KERN_ERR "%s GF_IOC_RESET.\n",__func__);
			g_reset_flag = 1;
			gf_hw_reset(gf_dev,0);			
			break;
		case GF_IOC_ENABLE_IRQ:
			printk(KERN_ERR "%s ++++++++++++ GF_IOC_ENABLE_IRQ \n",__func__);
			gf_enable_irq(gf_dev);
			break;
		case GF_IOC_DISABLE_IRQ:
			printk(KERN_ERR "%s ------------ GF_IOC_DISABLE_IRQ \n",__func__);
			gf_disable_irq(gf_dev);
			break;
		case GF_IOC_SENDKEY:			
			if(copy_from_user(&gf_key,(struct gf_key*)arg, sizeof(struct gf_key))) {
				printk(KERN_ERR "%s GF_IOC_SENDKEY failed to copy data from user.\n",__func__);
				retval = -EFAULT;
				break;
			}
			for(i = 0; i< ARRAY_SIZE(key_map); i++) {
				if(key_map[i].val == gf_key.key){
					input_report_key(gf_dev->input, gf_key.key, gf_key.value);
					input_sync(gf_dev->input);
					break;
				}
			}

			if(i == ARRAY_SIZE(key_map)) {
				printk(KERN_ERR "key %d not support yet \n", gf_key.key);
				retval = -EFAULT;
			}
			
			break;
		default:
			printk(KERN_ERR "%s gf doesn't support this command.\n",__func__);
			printk(KERN_ERR "%s CMD = 0x%x,_IOC_DIR:0x%x,_IOC_TYPE:0x%x,IOC_NR:0x%x,IOC_SIZE:0x%x\n",
					__func__,cmd,_IOC_DIR(cmd),_IOC_TYPE(cmd),_IOC_NR(cmd),_IOC_SIZE(cmd));
			retval = -EFAULT;
			break;
	}

	if(tmpbuf != NULL){
		kfree(tmpbuf);
		tmpbuf = NULL;
	}
	if(ioc != NULL) {
		kfree(ioc);
		ioc = NULL;
	}

	return retval;
}

#ifdef CONFIG_COMPAT
static long gf_compat_ioctl(struct file *filp, unsigned int cmd, 
						unsigned long arg)
{
	return gf_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif //CONFIG_COMPAT
static unsigned int gf_poll(struct file *filp,struct poll_table_struct *wait)
{
    //gf_dev_t *gf_dev = filp->private_data;
 	pr_info("%s called.\n",__func__);
    return 0;
}

static void goodix_spi_work_func(struct work_struct *work)
{
	gf_dev_t * gf_dev =NULL;
	int ret = 0;
	unsigned short irq_status = 0;
        gf_dev = container_of(work, gf_dev_t, work);
	ret = gf_spi_read_word(gf_dev,GF_IRQ_CTRL3,&irq_status);
	//printk("%s,irq_status = %d,ret=%d\n",__func__,(int)irq_status,ret);
	if(irq_status != 0x8) {
	if(gf_dev->async) {
		printk(KERN_ERR"kill_fasync!\n");
		kill_fasync(&gf_dev->async,SIGIO,POLL_IN);
		}
	}
}
	
static irqreturn_t gf_irq(int irq, void* handle)
{
	gf_dev_t *gf_dev = (gf_dev_t *)handle;
	if(!wake_lock_active(&gf_dev->finger_wake_lock))
	{
		wake_lock_timeout(&gf_dev->finger_wake_lock,2*HZ);
		printk("Finger is locked\n");
        }
//	unsigned short irq_status = 0;
//	printk("glx %s g_reset_flag=%d\n",__func__,g_reset_flag);
	if(g_reset_flag == 1){
		g_reset_flag = 0;
		return IRQ_HANDLED;
	}

	queue_work(gf_dev->spi_wq, &gf_dev->work);
//	gf_spi_read_word(gf_dev,GF_IRQ_CTRL3,&irq_status);
//	if(irq_status != 0x8) {
//	if(gf_dev->async) {
//		kill_fasync(&gf_dev->async,SIGIO,POLL_IN);
//		}
//	}

	return IRQ_HANDLED;
}

static int gf_open(struct inode *inode, struct file *filp)
{
	gf_dev_t *gf_dev;
    int cnt = 0;
	int	status = -ENXIO;
    unsigned short reg = 0;
    unsigned short chip_id_1 = 0;
	unsigned short chip_id_2 = 0;
	//printk(KERN_ERR "%s BUILD INFO:l;sdkfjalskdjfa;lskdjfaslkdjfa;sl%s,%s\n",__func__,__DATE__,__TIME__);

	mutex_lock(&device_list_lock);

	list_for_each_entry(gf_dev, &device_list, device_entry) {
		if(gf_dev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if(status == 0) {
		mutex_lock(&gf_dev->buf_lock);
		if( gf_dev->buffer == NULL) {
			gf_dev->buffer = kzalloc(bufsiz + GF_RDATA_OFFSET, GFP_KERNEL);
			if(gf_dev->buffer == NULL) {
				dev_dbg(&gf_dev->spi->dev, "open/ENOMEM\n");
				status = -ENOMEM;
			}
		}	
		mutex_unlock(&gf_dev->buf_lock);

		if(status == 0) {
			gf_dev->users++;
			filp->private_data = gf_dev;
            /*1. disable irq*/
            gf_disable_irq(gf_dev);
            /*2. set chip to init state.*/
            gf_hw_reset(gf_dev,0);
            while(cnt < 10){
                gf_spi_read_word(gf_dev,GF_IRQ_CTRL3,&reg);
                pr_info("%s IRQ status : 0x%x ,cnt = %d",__func__,reg,cnt);
                if(reg == 0x0100) {
                    gf_spi_write_word(gf_dev,GF_IRQ_CTRL2,0x0100);
                    break;
                }
                cnt++;
            }
            /*Get vendor id, chip id and parse OTPs*/
        	gf_spi_read_word(gf_dev,GF_VENDORID_ADDR,&g_vendorID);
        	pr_info("%s vendorID is 0x%04X \n", __func__, g_vendorID);
        	gf_spi_read_word(gf_dev,GF_CHIP_ID0,&chip_id_1);
        	gf_spi_read_word(gf_dev,GF_CHIP_ID1,&chip_id_2);
        	pr_info("%s chipID is 0x%04x 0x%04x \n",__func__,chip_id_1,chip_id_2);
        	pr_info("%s OTP Byte[0] ~ Byte[31] is: \n",__func__);
        	gf_parse_otp(gf_dev,g_sendorID);

        	if(g_tcode_img!=0 && g_fdt_delta!=0)
        	{
        		v0_fdt_down_cfg[5].value = g_fdt_delta;
        		v0_ff_cfg[5].value = g_fdt_delta;
        		v0_nav_fdt_down_cfg[5].value = g_fdt_delta;
        		v0_fdt_up_cfg[5].value = (((g_fdt_delta>>8)-2)<<8)|0x7F;
        		v0_img_cfg[0].value = g_tcode_img;
        		pr_info("%s use img_tcode:0x%x fdt_delta:0x%x fdt_up_delta:0x%x from OTP\n",
        			      __func__,g_tcode_img,g_fdt_delta,((((g_fdt_delta>>8)-2)<<8)|0x7F));
        	}
			nonseekable_open(inode, filp);
			printk(KERN_ERR "%s Succeed to open device, bufsiz=%d. irq = %d\n",__func__,bufsiz,gf_dev->spi->irq);
			gf_enable_irq(gf_dev);
		}

	} 
	else {
		printk(KERN_ERR "%s No device for minor %d\n",__func__,iminor(inode));
	}

	mutex_unlock(&device_list_lock);

	return status;
}

#ifdef GF_FASYNC
static int gf_fasync(int fd, struct file *filp, 
		int mode)
{
	gf_dev_t *gf_dev = filp->private_data;
	int ret;

	ret = fasync_helper(fd, filp, mode, &gf_dev->async);

	return ret;
}
#endif

static int gf_release(struct inode *inode, struct file *filp)
{
	gf_dev_t *gf_dev;
	int    status = 0;
	unsigned char idle_cmd = 0xC0;
	mutex_lock(&device_list_lock);
	gf_dev = filp->private_data;
	filp->private_data = NULL;

	/*last close??*/
	gf_dev->users --;
	if(!gf_dev->users) {
		printk(KERN_ERR "%s gf_realease called.\n",__func__);
		printk(KERN_ERR "%s disble_irq. irq = %d\n",__func__,gf_dev->irq);
		//disable_irq(gf_dev->spi->irq);
		gf_disable_irq(gf_dev);
	}
	pr_info("%s set chip to IDLE.\n",__func__);
	gf_spi_send_cmd(gf_dev,&idle_cmd,1);
	mutex_unlock(&device_list_lock);

	return status;
}

static const struct file_operations gf_fops = {
	.owner =	THIS_MODULE,
	.write =	gf_write,
	.read =		gf_read,
    .unlocked_ioctl = gf_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gf_compat_ioctl,
#endif// CONFIG_COMPAT
	.open =		gf_open,
	.release =	gf_release,
	.poll   = gf_poll,
#ifdef GF_FASYNC
	.fasync = gf_fasync,
#endif
};

#ifdef CONFIG_OF
static int gf_get_of_pdata(struct device *dev,
					struct gf_platform_data *pdata)
{
	struct device_node *node = dev->of_node;

	pdata->irq_gpio = of_get_named_gpio_flags(node,
			"gf_irq_gpio", 0, NULL);
	if (pdata->irq_gpio < 0) {
		dev_err(dev, "%s: Could not find irq gpio\n", __func__);
		goto of_err;
	}

	pdata->reset_gpio = of_get_named_gpio_flags(node,
			"gf_rst_gpio", 0, NULL);
	if (pdata->reset_gpio < 0) {
		dev_err(dev, "%s: Could not find reset gpio\n", __func__);
		goto of_err;
	}

	pdata->cs_gpio = of_get_named_gpio_flags(node,
			"gf_cs_gpio", 0, NULL);
	if (pdata->cs_gpio < 0)
		pdata->cs_gpio = -EINVAL;

	return 0;

of_err:
	pdata->reset_gpio   = -EINVAL;
	pdata->irq_gpio     = -EINVAL;
	pdata->cs_gpio      = -EINVAL;

	return -ENODEV;
}

#if 0
static int gf_pin_init(struct gf_platform_data *pdata)
{
	int err = 0;

	if (pdata->reset_gpio != -EINVAL) {
		err = gpio_request(pdata->reset_gpio, "gf_reset");
		if (!err) {
			err = gpio_direction_output(pdata->reset_gpio, 1);
			if (err) {
				gpio_free(pdata->reset_gpio);

				return err;
			}

			gpio_free(pdata->reset_gpio);
		}
	}

	if (pdata->irq_gpio != -EINVAL) {
		err = gpio_request(pdata->irq_gpio, "gf_irq");
		if (!err) {
			err = gpio_direction_input(pdata->irq_gpio);
			if (err) {
				gpio_free(pdata->irq_gpio);

				return err;
			}

			gpio_free(pdata->irq_gpio);
		}
	}
}
#endif
#endif

static int gf_power_init(gf_dev_t *gf_dev, bool on)
{
	int err = -EINVAL;

	if (true) {
		gf_dev->vdd = regulator_get(&gf_dev->spi->dev, "vdd");
		if (gf_dev->vdd != NULL) {
			err = regulator_enable(gf_dev->vdd);
			if (err)
				dev_err(&gf_dev->spi->dev, "error in enable vreg 2v8 fp\n");
		} else
			dev_err(&gf_dev->spi->dev, "error in get vreg 2v8 fp\n");
	}

	return err;
}

#if 1
static int gf_parse_dt(struct device *spi_dev, gf_dev_t *gf_drv)
{
	struct device_node *node = spi_dev->of_node;
	unsigned int flags = 0;
	
	gf_drv->rst_gpio = of_get_named_gpio_flags(node, "gf_rst_gpio", 0, &flags);
	gf_drv->irq_gpio = of_get_named_gpio_flags(node, "gf_irq_gpio", 0, &flags);
	//gf_drv->cs_gpio = of_get_named_gpio(node, "spi_cs_gpio", 0);
	//gf_drv->miso_gpio = of_get_named_gpio(node, "goodix,spi_miso_gpio", 0);
	
	printk("...... rst_gpio:%d irq_gpio:%d, ......\n", gf_drv->rst_gpio, gf_drv->irq_gpio);
	return 0;
}
#endif
static int gf_probe(struct spi_device *spi)
{    
	int status;
	int err = 0;   
	unsigned long minor;
	int tt = 0;
	struct gf_platform_data gf_pdata;
	gf_dev_t *gf_dev = NULL;

	unsigned char idle_cmd = 0xc0;

	gf_dev = kzalloc(sizeof(*gf_dev), GFP_KERNEL);

	if (!gf_dev){
		printk(KERN_ERR "%s Failed to alloc memory for gf device.\n",__func__);
		return -ENOMEM;
	}
	
    	/* Initialize the driver data */
	gf_parse_dt(&spi->dev, gf_dev);  //add by miaocht

	gf_dev->spi = spi;
	spi_set_drvdata(spi, gf_dev);

	INIT_LIST_HEAD(&gf_dev->device_entry);   

	mutex_lock(&device_list_lock);

	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		gf_dev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(gf_spi_class, &spi->dev, gf_dev->devt,
				gf_dev, GOODIX_DEV_NAME);
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	} else {
		printk(KERN_ERR "%s no minor number available!\n",__func__);
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&gf_dev->device_entry, &device_list);
	}

	mutex_unlock(&device_list_lock);

	gf_dev->buffer = kzalloc(bufsiz + GF_RDATA_OFFSET, GFP_KERNEL);
	if(gf_dev->buffer == NULL) {
		printk(KERN_ERR "%s can not alloc gf_dev->buffer.\n",__func__);
		kfree(gf_dev);
		return -ENOMEM;
	}
	
#if CONFIG_OF
	err = gf_get_of_pdata(&spi->dev, &gf_pdata);
	if (err)
		return 0;

	spi->dev.platform_data = &gf_pdata;
#if 0
	err = gf_pin_init(&gf_pdata);
	if (err)
		return 0;
#endif
#endif
	gpio_request(gf_pdata.reset_gpio, "gf_rst");  //gf_reset
	gpio_direction_input(gf_pdata.reset_gpio);
	gpio_set_value(gf_pdata.reset_gpio, 0);
//	gpio_set_value(gf_pdata.irq_gpio, 0);
	mdelay(50);
	gf_power_init(gf_dev, true);
//	gpio_set_value(gf_pdata.irq_gpio, 1);
	mdelay(50);
	gpio_direction_output(gf_pdata.reset_gpio, 1);
	spin_lock_init(&gf_dev->spi_lock);	
	mutex_init(&gf_dev->buf_lock);
	mutex_init(&gf_dev->frame_lock);
	wake_lock_init(&gf_dev->finger_wake_lock, WAKE_LOCK_SUSPEND,"fingerprint_wake_lock");
	//init_waitqueue_head(&gf_dev->waiter);

#if 0
	gf_dev->fpthread = kthread_run(gf_frame_handler, (void*)gf_dev,
			"fpthread");
	if(IS_ERR(gf_dev->fpthread)) {
		printk(KERN_ERR "Failed to create kernel thread: %ld\n", 
				PTR_ERR(gf_dev->fpthread));
	} 
#endif

	/*register device within input system.*/	
	gf_dev->input = input_allocate_device();
	if(gf_dev->input == NULL) {
		printk(KERN_ERR "Failed to allocate input device.\n");
		status = -ENOMEM;
		kfree(gf_dev->buffer);
		kfree(gf_dev);
		return -ENOMEM;
	}
	//fb add start
	//gf_dev->notifier = goodix_noti_block;
	//fb_register_client(&gf_dev->notifier);
	//fb add end
	//__set_bit(EV_KEY, gf_dev->input->evbit);
	//__set_bit(KEY_HOME, gf_dev->input->keybit);	
	//__set_bit(KEY_POWER, gf_dev->input->keybit);

	//gf_dev->input->name = "tiny4412-key";
	//if(input_register_device(gf_dev->input)) {
	//  printk(KERN_ERR "Failed to register input device.\n");
	//}
	
	gf_reg_key_kernel(gf_dev);

	gf_dev->spi->mode = SPI_MODE_0; 
	gf_dev->spi->max_speed_hz = 1000*1000; //gxl
	gf_dev->spi->bits_per_word = 8; 
	spi_setup(gf_dev->spi);

	gf_irq_cfg(gf_dev);          	 
	gf_dev->irq = gpio_to_irq(gf_pdata.irq_gpio);
	gf_dev->rst_gpio = gf_pdata.reset_gpio;

	printk(KERN_ERR "%s gf interrupt NO. = %d\n",__func__,gf_dev->irq);
#if 1
	err = request_threaded_irq(gf_dev->irq, NULL, gf_irq, 
			IRQF_TRIGGER_RISING | IRQF_ONESHOT,
			"gf_irq" , gf_dev);
#else
	err = request_irq(gf_dev->spi->irq, gf_irq, 
			IRQ_TYPE_EDGE_RISING,//IRQ_TYPE_LEVEL_HIGH,
			dev_name(&gf_dev->spi->dev), gf_dev);
#endif
	if(!err) {
		enable_irq_wake(gf_dev->irq);
		gf_disable_irq(gf_dev);
	}
	
	gf_dev->spi_wq = create_singlethread_workqueue("spi_wq");
	INIT_WORK(&gf_dev->work, goodix_spi_work_func);

	gf_hw_reset(gf_dev,0);	//chuntian		

	gf_spi_write_word(gf_dev,0x0124,0x0100);  
	gf_spi_write_word(gf_dev,0x0204,0x0000);

	/*Get vendor ID*/
	gf_spi_read_word(gf_dev,0x0006,&g_vendorID);
	pr_info("%s vendorID = 0x%04x",__func__,g_vendorID);

#if 1
	/*SPI test to read chip ID 002202A0*/
	for(tt=0;tt<100;tt++) {
		unsigned short chip_id_1 = 0;
		unsigned short chip_id_2 = 0;
		gf_spi_read_word(gf_dev,0x0000,&chip_id_1);
		gf_spi_read_word(gf_dev,0x0002,&chip_id_2);

		printk(KERN_ERR "%s chip id is 0x%04x 0x%04x \n",__func__,chip_id_2,chip_id_1);
	}
#endif
	gf_spi_send_cmd(gf_dev,&idle_cmd,1);
    mdelay(1);
	printk(KERN_ERR "%s GF installed.\n",__func__);
	printk("--------  Func:%s   end   ----------\n", __func__);

	return status;
}

static int gf_remove(struct spi_device *spi)
{
	gf_dev_t *gf_dev = spi_get_drvdata(spi);
#if 0    
	if (!gf_dev->fpthread){
		printk(KERN_ERR "stop fpthread\n");
		kthread_stop(gf_dev->fpthread);
		gf_dev->fpthread = NULL;
	}
#endif
	if(gf_dev->spi->irq) {
		free_irq(gf_dev->spi->irq, gf_dev);
	}

	gf_dev->spi = NULL;
	spi_set_drvdata(spi, NULL);

#if 0	
	del_timer_sync(&gf_dev->gf_timer);
	cancel_work_sync(&gf_dev->spi_work);
#endif
	/*
	   if(gf_dev->spi_wq != NULL) {
	   flush_workqueue(gf_dev->spi_wq);
	   destroy_workqueue(gf_dev->spi_wq);
	   }
	 */
	/* prevent new opens */
	mutex_lock(&device_list_lock);

	list_del(&gf_dev->device_entry);
	device_destroy(gf_spi_class, gf_dev->devt);
	clear_bit(MINOR(gf_dev->devt), minors);
	if (gf_dev->users == 0) {
		if(gf_dev->input != NULL)
			input_unregister_device(gf_dev->input);

		if(gf_dev->buffer != NULL)
			kfree(gf_dev->buffer);
		kfree(gf_dev);
	}

	mutex_unlock(&device_list_lock);
	printk(KERN_ERR "%s gf_remove called.\n",__func__); 
	return 0;
}

//static int gf_suspend(struct spi_device *spi, pm_message_t mesg)
static int gf_suspend(struct device *dev)
{
	//gf_dev_t *gf_dev = spi_get_drvdata(spi);
	printk(KERN_ERR "%s device suspend.\n",__func__);

	return 0;
}

//static int gf_resume(struct spi_device *spi)
static int gf_resume(struct device *dev)
{
	//gf_dev_t *gf_dev = spi_get_drvdata(spi);
	printk(KERN_ERR "%s device resume.\n",__func__);
	return 0;
}

 static const struct dev_pm_ops gf_pm = {
     .suspend = gf_suspend,
     .resume = gf_resume,
};


#ifdef CONFIG_OF
static struct of_device_id gf_of_match[] = {
	{ .compatible = "gf,goodix", },
	{}
};

MODULE_DEVICE_TABLE(of, gf_of_match);
#endif

static struct spi_driver gf_spi_driver = {
	.driver = {
		.name =		SPI_DEV_NAME,
		.owner =	THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = gf_of_match,
#endif
		.pm = &gf_pm,
	},
	.probe =	gf_probe,
	.remove =	gf_remove,
//	.suspend = gf_suspend,
//	.resume = gf_resume,
};

static int __init gf_init(void)
{
	int status;

	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, CHRD_DRIVER_NAME, &gf_fops);
	if (status < 0){
		printk(KERN_ERR "%s Failed to register char device!\n",__func__);

		return status;
	}
	gf_spi_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(gf_spi_class)) {
		unregister_chrdev(SPIDEV_MAJOR, gf_spi_driver.driver.name);
		printk(KERN_ERR "%s Failed to create class.\n",__func__);

		return PTR_ERR(gf_spi_class);
	}
	status = spi_register_driver(&gf_spi_driver);
	if (status < 0) {
		class_destroy(gf_spi_class);
		unregister_chrdev(SPIDEV_MAJOR, gf_spi_driver.driver.name);
		printk(KERN_ERR "%s Failed to register SPI driver.\n",__func__);
	}
	return status;
}

static void __exit gf_exit(void)
{
	spi_unregister_driver(&gf_spi_driver);
	class_destroy(gf_spi_class);
	unregister_chrdev(SPIDEV_MAJOR, gf_spi_driver.driver.name);
}
module_init(gf_init);
module_exit(gf_exit);

MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");

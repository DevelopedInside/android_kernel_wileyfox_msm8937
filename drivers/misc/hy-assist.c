/**************************/
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/freezer.h>
//#include <mach/gpio.h>
#include <linux/module.h>
#include <linux/hy-assist.h>

#define DEBUG 0

#define set_ATTR(dev,name) \
		do { \
			dev##_assist_##name##_attr.show = _show; \
			dev##_assist_##name##_attr.store = _store; \
		} while(0)

#define ASSIST_CLASS_NAME "assist"
#define CTP_DEVICE_NAME	"ctp"
#define ACCEL_DEVICE_NAME "accel"
#define ALSPS_DEVICE_NAME "alsps"

struct class *assist_class;
struct device *ctp_dev;
struct device *accel_dev;
struct device *alsps_dev;

static ssize_t attr_assist_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("%s:default(null)\n",__func__);

	return sprintf(buf, "not set\n");
}
#if 0
static ssize_t attr_assist_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	printk("%s:default(null)\n",__func__);

	return count;
}
#endif

/*ctp attribute */
static struct device_attribute ctp_assist_ic_attr = {
	.attr = {
		.name = "ic",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};

static struct device_attribute ctp_assist_fw_ver_attr = {
	.attr = {
		.name = "fw_ver",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};

static struct device_attribute ctp_assist_tp_vendor_attr = {
	.attr = {
		.name = "tp_vendor",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};

static struct device_attribute ctp_assist_exist_attr = {
	.attr = {
		.name = "exist",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};

/*sensors attribute*/
static struct device_attribute accel_assist_ic_attr = {
	.attr = {
		.name = "ic",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};
static struct device_attribute accel_assist_vendor_attr = {
	.attr = {
		.name = "vendor",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};
static struct device_attribute accel_assist_exist_attr = {
	.attr = {
		.name = "exist",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};

static struct device_attribute alsps_assist_ic_attr = {
	.attr = {
		.name = "ic",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};
static struct device_attribute alsps_assist_vendor_attr = {
	.attr = {
		.name = "vendor",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};
static struct device_attribute alsps_assist_exist_attr = {
	.attr = {
		.name = "exist",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};
static struct device_attribute alsps_assist_ps_data_attr = {
	.attr = {
		.name = "ps_data",
		.mode = S_IRUGO,
	},
	.show = &attr_assist_show,
};

/*register function*/
int ctp_assist_register_attr(char *attr_name,
		ATTR_FUNC_SHOW _show, ATTR_FUNC_STORE _store)
{
	if (!strcmp(attr_name,"ic"))
		set_ATTR(ctp,ic);
	else if (!strcmp(attr_name,"fw_ver"))
		set_ATTR(ctp,fw_ver);
	else if (!strcmp(attr_name,"tp_vendor"))
		set_ATTR(ctp,tp_vendor);
	else if (!strcmp(attr_name,"exist"))
		set_ATTR(ctp,exist);
	else
		return -1;
	return 0;
}

int accel_assist_register_attr(char *attr_name,
		ATTR_FUNC_SHOW _show, ATTR_FUNC_STORE _store)
{
	if (!strcmp(attr_name,"ic"))
		set_ATTR(accel,ic);
	else if (!strcmp(attr_name,"vendor"))
		set_ATTR(accel,vendor);
	else if (!strcmp(attr_name,"exist"))
		set_ATTR(accel,exist);
	else
		return -1;
	return 0;
}

int alsps_assist_register_attr(char *attr_name,
		ATTR_FUNC_SHOW _show, ATTR_FUNC_STORE _store)
{
	if (!strcmp(attr_name,"ic"))
		set_ATTR(alsps,ic);
	else if (!strcmp(attr_name,"vendor"))
		set_ATTR(alsps,vendor);
	else if (!strcmp(attr_name,"exist"))
		set_ATTR(alsps,exist);
	else if (!strcmp(attr_name,"ps_data"))
		set_ATTR(alsps,ps_data);
	else
		return -1;
	return 0;
}

static int __init assist_init(void)
{
	pr_info("assist_init\n");

        assist_class = class_create(THIS_MODULE, ASSIST_CLASS_NAME);
        if (IS_ERR(assist_class ))
                pr_err("Failed to create class(drivers assist)!\n");

	/*ctp device and node*/
        ctp_dev = device_create(assist_class,NULL, 0, NULL, CTP_DEVICE_NAME);
        if (IS_ERR(ctp_dev))
                pr_err("Failed to create device(ctp device)!\n");

        if (device_create_file(ctp_dev, &ctp_assist_ic_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", ctp_assist_ic_attr.attr.name);

        if (device_create_file(ctp_dev, &ctp_assist_fw_ver_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", ctp_assist_fw_ver_attr.attr.name);

	if (device_create_file(ctp_dev, &ctp_assist_tp_vendor_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", ctp_assist_tp_vendor_attr.attr.name);

        if (device_create_file(ctp_dev, &ctp_assist_exist_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", ctp_assist_exist_attr.attr.name);

	/*accel device and node*/
        accel_dev = device_create(assist_class,NULL, 0, NULL, ACCEL_DEVICE_NAME);
        if (IS_ERR(accel_dev))
                pr_err("Failed to create device(gsensor device)!\n");

        if (device_create_file(accel_dev, &accel_assist_ic_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", accel_assist_ic_attr.attr.name);
		
	if (device_create_file(accel_dev, &accel_assist_vendor_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", accel_assist_vendor_attr.attr.name);
	
	if (device_create_file(accel_dev, &accel_assist_exist_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", accel_assist_exist_attr.attr.name);

	/*alsps device and node*/
        alsps_dev = device_create(assist_class,NULL, 0, NULL, ALSPS_DEVICE_NAME);
        if (IS_ERR(alsps_dev))
                pr_err("Failed to create device(gsensor device)!\n");

        if (device_create_file(alsps_dev, &alsps_assist_ic_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", alsps_assist_ic_attr.attr.name);
		
	if (device_create_file(alsps_dev, &alsps_assist_vendor_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", alsps_assist_vendor_attr.attr.name);
	
	if (device_create_file(alsps_dev, &alsps_assist_exist_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", alsps_assist_exist_attr.attr.name);

	if (device_create_file(alsps_dev, &alsps_assist_ps_data_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", alsps_assist_ps_data_attr.attr.name);

	return 0;
}
static void __exit assist_exit(void)
{
	pr_info("assist_exit\n");
}

module_init(assist_init);
module_exit(assist_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<lnxseed3@163.com>");

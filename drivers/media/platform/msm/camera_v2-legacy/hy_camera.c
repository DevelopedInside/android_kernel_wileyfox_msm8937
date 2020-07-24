/*

 *
 */

#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/freezer.h>
//#include <mach/gpio.h>
#include <linux/module.h>
#include <media/hy_camera.h>

#define CAMERA_CLASS_NAME "camera"
#define	REAR_CAMERA_DEVICE_NAME	"rear_camera"
#define FRONT_CAMERA_DEVICE_NAME "front_camera"

struct class *cam_class;
struct device *rear_cam_dev;
struct device *front_cam_dev;

struct hycit_data_k rear_cam_private_data,front_cam_private_data;
bool rear_cam_probed,front_cam_probed;

/*hymost 0515/2015 add*/

static ssize_t rear_cam_ic_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", rear_cam_private_data.sensor_ic_name);
}
static ssize_t rear_cam_module_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", rear_cam_private_data.sensor_module_name);
}
#if 0
static ssize_t rear_cam_pixel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", rear_cam_private_data.pixel_sizes);
}
static ssize_t rear_cam_focus_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", rear_cam_private_data.focus_type);
}
static ssize_t rear_cam_fvalue_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", rear_cam_private_data.f_value);
}
static ssize_t rear_cam_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", rear_cam_private_data.param_version);
}
static ssize_t rear_cam_flash_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", rear_cam_private_data.flash_name);
}
static ssize_t rear_cam_flash_module_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", rear_cam_private_data.flash_module_name);
}
#endif
static ssize_t rear_cam_probe_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", rear_cam_probed);
}
/*rear camera attribute */

static struct device_attribute rear_cam_ic_attr = {
	.attr = {
		.name = "ic",
		.mode = S_IRUGO,
	},
	.show = &rear_cam_ic_show,
};

static struct device_attribute rear_cam_module_attr = {
	.attr = {
		.name = "module",
		.mode = S_IRUGO,
	},
	.show = &rear_cam_module_show,
};
#if 0
static struct device_attribute rear_cam_pixel_attr = {
	.attr = {
		.name = "pixel",
		.mode = S_IRUGO,
	},
	.show = &rear_cam_pixel_show,
};

static struct device_attribute rear_cam_focus_attr = {
	.attr = {
		.name = "focus",
		.mode = S_IRUGO,
	},
	.show = &rear_cam_focus_show,
};

static struct device_attribute rear_cam_fvalue_attr = {
	.attr = {
		.name = "fvalue",
		.mode = S_IRUGO,
	},
	.show = &rear_cam_fvalue_show,
};

static struct device_attribute rear_cam_param_attr = {
	.attr = {
		.name = "param",
		.mode = S_IRUGO,
	},
	.show = &rear_cam_param_show,
};

static struct device_attribute rear_cam_flash_attr = {
	.attr = {
		.name = "flash",
		.mode = S_IRUGO,
	},
	.show = &rear_cam_flash_show,
};

static struct device_attribute rear_cam_flash_module_attr = {
	.attr = {
		.name = "flash_module",
		.mode = S_IRUGO,
	},
	.show = &rear_cam_flash_module_show,
};
#endif
static struct device_attribute rear_cam_probe_attr = {
	.attr = {
		.name = "probe",
		.mode = S_IRUGO,
	},
	.show = &rear_cam_probe_show,
};

static ssize_t front_cam_ic_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", front_cam_private_data.sensor_ic_name);
}
static ssize_t front_cam_module_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", front_cam_private_data.sensor_module_name);
}
#if 0
static ssize_t front_cam_pixel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", front_cam_private_data.pixel_sizes);
}
static ssize_t front_cam_focus_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", front_cam_private_data.focus_type);
}
static ssize_t front_cam_fvalue_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", front_cam_private_data.f_value);
}
static ssize_t front_cam_param_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", front_cam_private_data.param_version);
}
#endif
static ssize_t front_cam_probe_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", front_cam_probed);
}

/*front camera attribute */

static struct device_attribute front_cam_ic_attr = {
	.attr = {
		.name = "ic",
		.mode = S_IRUGO,
	},
	.show = &front_cam_ic_show,
};

static struct device_attribute front_cam_module_attr = {
	.attr = {
		.name = "module",
		.mode = S_IRUGO,
	},
	.show = &front_cam_module_show,
};
#if 0
static struct device_attribute front_cam_pixel_attr = {
	.attr = {
		.name = "pixel",
		.mode = S_IRUGO,
	},
	.show = &front_cam_pixel_show,
};

static struct device_attribute front_cam_focus_attr = {
	.attr = {
		.name = "focus",
		.mode = S_IRUGO,
	},
	.show = &front_cam_focus_show,
};

static struct device_attribute front_cam_fvalue_attr = {
	.attr = {
		.name = "fvalue",
		.mode = S_IRUGO,
	},
	.show = &front_cam_fvalue_show,
};

static struct device_attribute front_cam_param_attr = {
	.attr = {
		.name = "param",
		.mode = S_IRUGO,
	},
	.show = &front_cam_param_show,
};
#endif
static struct device_attribute front_cam_probe_attr = {
	.attr = {
		.name = "probe",
		.mode = S_IRUGO,
	},
	.show = &front_cam_probe_show,
};

/*register function*/
void rear_cam_register_attr(struct hycit_data_k b_private_data, bool is_probed)
{
	rear_cam_private_data = b_private_data;
	rear_cam_probed = is_probed;
}
void front_cam_register_attr(struct hycit_data_k f_private_data, bool is_probed)
{
	front_cam_private_data = f_private_data;
	front_cam_probed = is_probed;
}

static int __init camera_init(void)
{
	pr_info("camera_init\n");

        cam_class = class_create(THIS_MODULE, CAMERA_CLASS_NAME);
        if (IS_ERR(cam_class ))
                pr_err("Failed to create camera class !!!\n");

	/*rear camera device and node*/
        rear_cam_dev = device_create(cam_class,NULL, 0, NULL, REAR_CAMERA_DEVICE_NAME);
        if (IS_ERR(rear_cam_dev))
                pr_err("Failed to create device(rear camera)!\n");

        if (device_create_file(rear_cam_dev, &rear_cam_ic_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", rear_cam_ic_attr.attr.name);

        if (device_create_file(rear_cam_dev, &rear_cam_module_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", rear_cam_module_attr.attr.name);
#if 0
        if (device_create_file(rear_cam_dev, &rear_cam_pixel_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", rear_cam_pixel_attr.attr.name);

        if (device_create_file(rear_cam_dev, &rear_cam_focus_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", rear_cam_focus_attr.attr.name);

        if (device_create_file(rear_cam_dev, &rear_cam_fvalue_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", rear_cam_fvalue_attr.attr.name);

        if (device_create_file(rear_cam_dev, &rear_cam_param_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", rear_cam_param_attr.attr.name);

        if (device_create_file(rear_cam_dev, &rear_cam_flash_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", rear_cam_flash_attr.attr.name);

        if (device_create_file(rear_cam_dev, &rear_cam_flash_module_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", rear_cam_flash_module_attr.attr.name);
#endif
        if (device_create_file(rear_cam_dev, &rear_cam_probe_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", rear_cam_probe_attr.attr.name);

	/*front camera device and node*/
        front_cam_dev = device_create(cam_class,NULL, 0, NULL, FRONT_CAMERA_DEVICE_NAME);
        if (IS_ERR(front_cam_dev))
                pr_err("Failed to create device(front camera)!\n");

        if (device_create_file(front_cam_dev, &front_cam_ic_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", front_cam_ic_attr.attr.name);

        if (device_create_file(front_cam_dev, &front_cam_module_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", front_cam_module_attr.attr.name);
#if 0
        if (device_create_file(front_cam_dev, &front_cam_pixel_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", front_cam_pixel_attr.attr.name);

        if (device_create_file(front_cam_dev, &front_cam_focus_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", front_cam_focus_attr.attr.name);

        if (device_create_file(front_cam_dev, &front_cam_fvalue_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", front_cam_fvalue_attr.attr.name);

        if (device_create_file(front_cam_dev, &front_cam_param_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", front_cam_param_attr.attr.name);
#endif
        if (device_create_file(front_cam_dev, &front_cam_probe_attr) < 0)
                pr_err("Failed to create device file(%s)!\n", front_cam_probe_attr.attr.name);

	return 0;
}
static void __exit camera_exit(void)
{
	pr_info("camera_exit\n");
}

module_init(camera_init);
module_exit(camera_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<linux@163.com>");

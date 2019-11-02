#ifndef __CTP_ASSIST_H__
#define __CTP_ASSIST_H__

#include <asm/uaccess.h>
#include <linux/sysfs.h>

typedef ssize_t (*ATTR_FUNC_SHOW)(struct device *dev, \
                struct device_attribute *attr,char *buf);
        

typedef ssize_t (*ATTR_FUNC_STORE)(struct device *dev, \
                struct device_attribute *attr,const char *buf, size_t count);


int ctp_assist_register_attr(char *attr_name,
                ATTR_FUNC_SHOW _show, ATTR_FUNC_STORE _store);

int accel_assist_register_attr(char *attr_name,
                ATTR_FUNC_SHOW _show, ATTR_FUNC_STORE _store);

int alsps_assist_register_attr(char *attr_name,
		ATTR_FUNC_SHOW _show, ATTR_FUNC_STORE _store);

#endif /* __CYTTSP_CORE_H__ */

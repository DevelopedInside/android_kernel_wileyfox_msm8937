/*
 */


#ifndef __HY_CAMERA_H__
#define __HY_CAMERA_H__

#include <asm/uaccess.h>
#include <linux/sysfs.h>
#include <media/msm_cam_sensor.h>

void rear_cam_register_attr(struct hycit_data_k b_private_data, bool is_probed);
void front_cam_register_attr(struct hycit_data_k f_private_data, bool is_probed);

#endif /* __HY_CAMERA_H__ */

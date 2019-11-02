#ifndef __USB_CUSTOM_CFG_H__
#define __USB_CUSTOM_CFG_H__
#define USB_MODE_PATH "/sys/class/android_usb/android0/usb_mode"
#define USB_INIT_MODE   8
#define USB_TEST_MODE   49//'1'
#define USB_USER_MODE   48 // '0'
// ynn add for custom pid and name start
#ifdef CONFIG_ARCH_MSM8909_X29_650L // for example the project config
#define USB_VENDOR_NAME     "KAZAM"
#define USB_PRODUCT_NAME    ""
#define USB_SERIAL_NUMBER   "KAZAM_PHONE"
#define USB_DEFAULT_PID         0xA572
#define USB_USER_VID            0x1EBF//0x20E1
#else
#define USB_VENDOR_NAME     "Linux"
#define USB_PRODUCT_NAME    "File-CD Gadget"
#define USB_SERIAL_NUMBER   "1234567890ABCDEF"
#define USB_DEFAULT_PID         0x9026
#define USB_DEFAULT_ADB_PID     0x9025
#define USB_USER_VID            0x05C6
#endif
// ynn add for custom pid and name end

#endif /* !__USB_CUSTOM_CFG_H__ */

/**
 * goodix milanf header file
 */

#ifndef __GF_MILANF_H
#define __GF_MILANF_H

#include <linux/types.h>

#define GF_NET_EVENT_FB_BLACK 0
#define GF_NET_EVENT_FB_UNBLACK 1

struct gf_configs {
	unsigned short addr;
	unsigned short value;
};

#define CONFIG_FDT_DOWN  1
#define CONFIG_FDT_UP    2
#define CONFIG_FF        3
#define CONFIG_NAV       4
#define CONFIG_IMG       5
#define CONFIG_NAV_IMG   6
#define GF_CFG_NAV_IMG_MAN   7
#define CONFIG_NAV_FDT_DOWN  8

#define GF_BUF_STA_MASK		(0x1<<7)
#define	GF_BUF_STA_READY	(0x1<<7)
#define	GF_BUF_STA_BUSY		(0x0<<7)

#define	GF_IMAGE_MASK		(1<<6)
#define	GF_IMAGE_ENABLE		(1<<6)
#define	GF_IMAGE_DISABLE	(0x0)

#define	GF_KEY_MASK			(0x1<<5)
#define	GF_KEY_ENABLE		(0x1<<5)
#define	GF_KEY_DISABLE		(0x0)

#define	GF_KEY_STA			(0x1<<4)

/******************** IOCTL ****************/
#define  GF_IOC_MAGIC    'g' 

struct gf_ioc_transfer {
	u8	cmd;
	u8 reserve;
	u16	addr;
	u32 len;	
	unsigned char* buf;	
};

struct gf_key {
	unsigned int key;
	int value;
};

struct gf_key_map
{
    char *name;
    unsigned short val;
};

#define  GF_IOC_RESET	       _IO(GF_IOC_MAGIC, 0)
#define  GF_IOC_RW	           _IOWR(GF_IOC_MAGIC, 1, struct gf_ioc_transfer)
#define  GF_IOC_CMD            _IOW(GF_IOC_MAGIC,2,unsigned char)
#define  GF_IOC_CONFIG         _IOW(GF_IOC_MAGIC,3,unsigned char)
#define  GF_IOC_ENABLE_IRQ     _IO(GF_IOC_MAGIC,4)
#define  GF_IOC_DISABLE_IRQ    _IO(GF_IOC_MAGIC,5)
#define  GF_IOC_SENDKEY        _IOW(GF_IOC_MAGIC,6,struct gf_key)
#define  GF_IOC_MAXNR          7

#endif //__GF_MILANF_H

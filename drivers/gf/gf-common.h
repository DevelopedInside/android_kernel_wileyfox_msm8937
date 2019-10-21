/**
 * goodix common header file
 */

#ifndef __GF_COMMON_H
#define __GF_COMMON_H

#include <linux/wakelock.h>

typedef struct {
	dev_t			     devt;
	struct spi_device	 *spi;
	struct list_head	 device_entry;
	struct input_dev     *input;
	struct workqueue_struct *spi_wq;
	struct work_struct   work;
	struct wake_lock     finger_wake_lock;
	struct mutex         buf_lock;
	struct mutex         frame_lock;
	struct timer_list  	 gf_timer;
	struct task_struct*  fpthread;
	struct fasync_struct *async;
	struct notifier_block notifier; //fb add
	struct regulator *vdd;
	spinlock_t           spi_lock;
	wait_queue_head_t    waiter;
    unsigned int	     users;
	int 				 irq;
	u8			         *buffer;
	u8			         buf_status;
	u32                  irq_gpio;
	u32                  rst_gpio;
}gf_dev_t;

struct gf_platform_data {
	int irq_gpio;
	int reset_gpio;
	int cs_gpio;
};

/*******************TINY4412 hardware platform*******************/
#define 	GF_RST_PIN   	EXYNOS4_GPX1(5)
#define 	GF_IRQ_PIN   	EXYNOS4_GPX1(4)
#define 	GF_IRQ_NUM   	gpio_to_irq(GF_IRQ_PIN)
#define		GF_MISO_PIN	    EXYNOS4_GPB(2)
#define     GF_FLASH_BYPASS_PIN EXYNOS4_GPK3(0)

/****************Goodix hardware****************/
#define GF_W          	0xF0
#define GF_R          	0xF1
#define GF_WDATA_OFFSET	(0x3)
#define GF_RDATA_OFFSET	(0x4)

/****************Function prototypes*****************/

extern int  gf_spi_read_bytes(gf_dev_t* gf_dev, u16 addr, u16 data_len, u8* rx_buf);
extern int  gf_spi_write_bytes(gf_dev_t* gf_dev, u16 addr, u16 data_len, u8* tx_buf);
extern int  gf_spi_read_word(gf_dev_t* gf_dev, u16 addr, u16* value);
extern int  gf_spi_write_word(gf_dev_t* gf_dev, u16 addr, u16 value);
//extern int  gf_spi_read_data(gf_dev_t* gf_dev, u16 addr, int len, u8* value, bool endian_exchange);
extern int  gf_spi_read_data(gf_dev_t* gf_dev, u16 addr, int len, u8* value);
extern int  gf_spi_read_data_bigendian(gf_dev_t* gf_dev, u16 addr, int len, u8* value);
extern int  gf_spi_write_data(gf_dev_t* gf_dev, u16 addr, int len, u8* value);
extern int  gf_spi_send_cmd(gf_dev_t* gf_dev, unsigned char* cmd, int len);

#endif //__GF_COMMON_H

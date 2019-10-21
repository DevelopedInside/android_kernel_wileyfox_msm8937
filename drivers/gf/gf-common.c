/**
 * gf_common.c
 * GOODIX SPI common handle file
 */
#include <linux/mutex.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/completion.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include "gf-common.h"

#define SPI_ASYNC 1

/******************** Function Definitions *************************/
#ifdef SPI_ASYNC
static void gf_spi_complete(void *arg)
{
    complete(arg);
}
#endif 

int gf_spi_read_bytes(gf_dev_t* gf_dev, u16 addr, u16 data_len, 
	                         u8* rx_buf)
{
#ifdef SPI_ASYNC
    DECLARE_COMPLETION_ONSTACK(write_done);
#endif 
	struct spi_message msg;
    struct spi_transfer *xfer;
    int ret = 0;

    xfer = kzalloc(sizeof(*xfer)*2, GFP_KERNEL);
    if(xfer == NULL){
		pr_err("%s, No memory for command.\n",__func__);
		return -ENOMEM;
    }

    /*send gf command to device.*/
    spi_message_init(&msg);
    rx_buf[0] = GF_W;
    rx_buf[1] = (u8)((addr >> 8)&0xFF);
    rx_buf[2] = (u8)(addr & 0xFF);
    xfer[0].tx_buf = rx_buf;
    xfer[0].len = 3;
    xfer[0].delay_usecs = 5;

    spi_message_add_tail(&xfer[0], &msg);

    /*if wanted to read data from gf. 
     *Should write Read command to device
     *before read any data from device.
     */
    spi_sync(gf_dev->spi, &msg);
    spi_message_init(&msg);
    memset(rx_buf, 0xff, data_len +4);
    rx_buf[3] = GF_R;
    xfer[1].tx_buf = &rx_buf[3];
    xfer[1].rx_buf = &rx_buf[3];
    xfer[1].len = data_len + 1;
    xfer[1].delay_usecs = 5;

    spi_message_add_tail(&xfer[1], &msg);

#ifdef SPI_ASYNC
    msg.complete = gf_spi_complete;
    msg.context = &write_done;

    spin_lock_irq(&gf_dev->spi_lock);
    ret = spi_async(gf_dev->spi, &msg);
    spin_unlock_irq(&gf_dev->spi_lock);
    if(ret == 0) {
	wait_for_completion(&write_done);
	if(msg.status == 0)
	    ret = msg.actual_length - 1;
    }
#else
    ret = spi_sync(gf_dev->spi, &msg);
    if(ret == 0){
	ret = msg.actual_length - 1;
    }
#endif
    //pr_info("%s ret = %d,actual_length = %d \n",__func__,ret,msg.actual_length);
    kfree(xfer);
    if(xfer != NULL)
	xfer = NULL;

    return ret;
}

int gf_spi_write_bytes(gf_dev_t* gf_dev, u16 addr, u16 data_len,
							  u8* tx_buf)
{
#ifdef SPI_ASYNC
    DECLARE_COMPLETION_ONSTACK(read_done);
#endif
    struct spi_message msg;
    struct spi_transfer *xfer;
    int ret = 0;

    xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
    if( xfer == NULL){
		pr_err("%s, No memory for command.\n",__func__);
		return -ENOMEM;
    }

    /*send gf command to device.*/
    spi_message_init(&msg);
    tx_buf[0] = GF_W;
    tx_buf[1] = (u8)((addr >> 8)&0xFF);
    tx_buf[2] = (u8)(addr & 0xFF);
    xfer[0].tx_buf = tx_buf;
    xfer[0].len = data_len + 3;
    xfer[0].delay_usecs = 5;

    spi_message_add_tail(xfer, &msg);
#ifdef SPI_ASYNC
    msg.complete = gf_spi_complete;
    msg.context = &read_done;

    spin_lock_irq(&gf_dev->spi_lock);
    ret = spi_async(gf_dev->spi, &msg);
    spin_unlock_irq(&gf_dev->spi_lock);
    if(ret == 0) {
	wait_for_completion(&read_done);
	if(msg.status == 0)
	    ret = msg.actual_length - GF_WDATA_OFFSET;
    }
#else
    ret = spi_sync(gf_dev->spi, &msg);
    if(ret == 0) {
	ret = msg.actual_length - GF_WDATA_OFFSET;
    }
#endif
    //pr_info("%s ret = %d,actual_length = %d \n",__func__,ret-2,msg.actual_length);
    kfree(xfer);
    if(xfer != NULL)
		xfer = NULL;

    return ret-2;// ret-2: because have 2 bytes represent for length.
}

int gf_spi_read_word(gf_dev_t* gf_dev, u16 addr, u16* value)
{
    int status = 0;
    u8* buf = NULL;

    mutex_lock(&gf_dev->buf_lock);
    status = gf_spi_read_bytes(gf_dev, addr, 2, gf_dev->buffer);
    buf = gf_dev->buffer + GF_RDATA_OFFSET;
    *value = ((u16)(buf[0]<<8)) | buf[1];
    mutex_unlock(&gf_dev->buf_lock);

    return status;
}

int gf_spi_write_word(gf_dev_t* gf_dev, u16 addr, u16 value)
{
	int status = 0;
    mutex_lock(&gf_dev->buf_lock);
    gf_dev->buffer[GF_WDATA_OFFSET] = 0x00;
    gf_dev->buffer[GF_WDATA_OFFSET+1] = 0x01;
    gf_dev->buffer[GF_WDATA_OFFSET+2] = (u8)(value>>8);
    gf_dev->buffer[GF_WDATA_OFFSET+3] = (u8)(value & 0x00ff);
    status = gf_spi_write_bytes(gf_dev, addr, 4, gf_dev->buffer);
    mutex_unlock(&gf_dev->buf_lock);

    return status;
}

void endian_exchange(int len, u8* buf)
{
    int i;
    u8 buf_tmp;
    for(i=0; i< len/2; i++)
    {   
		buf_tmp = buf[2*i+1];
		buf[2*i+1] = buf[2*i] ;
		buf[2*i] = buf_tmp;
    }
}

int gf_spi_read_data(gf_dev_t* gf_dev, u16 addr, int len, u8* value)
{
    int status;

    mutex_lock(&gf_dev->buf_lock);
    status = gf_spi_read_bytes(gf_dev, addr, len, gf_dev->buffer);
    memcpy(value, gf_dev->buffer+GF_RDATA_OFFSET, len);
    mutex_unlock(&gf_dev->buf_lock);

    return status;
}

int gf_spi_read_data_bigendian(gf_dev_t* gf_dev, u16 addr,int len, u8* value)
{
    int status;

    mutex_lock(&gf_dev->buf_lock);
    status = gf_spi_read_bytes(gf_dev, addr, len, gf_dev->buffer);
    memcpy(value, gf_dev->buffer+GF_RDATA_OFFSET, len);
    mutex_unlock(&gf_dev->buf_lock);	
	
   	endian_exchange(len,value);
    return status;
}

int gf_spi_write_data(gf_dev_t* gf_dev, u16 addr, int len, u8* value)
{
    int status =0;
    //int i = 0;
    unsigned short addr_len = 0;
    unsigned char* buf = NULL;

    if (len > 1024 * 10){
		pr_err("%s length is large.\n",__func__);
		return -1;
    }

    addr_len = len / 2;  

    buf = kzalloc(len + 2, GFP_KERNEL);
    if (buf == NULL){
		pr_err("%s, No memory for buffer.\n",__func__);
		return -ENOMEM;
    }

    buf[0] = (unsigned char) ((addr_len & 0xFF00) >> 8);
    buf[1] = (unsigned char) (addr_len & 0x00FF);
    memcpy(buf+2, value, len);
    endian_exchange(len,buf+2);

    mutex_lock(&gf_dev->buf_lock);
    memcpy(gf_dev->buffer+GF_WDATA_OFFSET, buf, len+2);
    kfree(buf);

    status = gf_spi_write_bytes(gf_dev, addr, len+2, gf_dev->buffer);
    mutex_unlock(&gf_dev->buf_lock);

    return status;
}

int gf_spi_send_cmd(gf_dev_t* gf_dev, unsigned char* cmd, int len)
{
    struct spi_message msg;
    struct spi_transfer *xfer;
    int ret;
	
	spi_message_init(&msg);
    xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
	if( xfer == NULL){
		pr_err("%s, No memory for command.\n",__func__);
		return -ENOMEM;
    }
    xfer->tx_buf = cmd;
    xfer->len = len;
    xfer->delay_usecs = 5;
    spi_message_add_tail(xfer, &msg);
    ret = spi_sync(gf_dev->spi, &msg);
    	
    kfree(xfer);
    if(xfer != NULL)
		xfer = NULL;

	return ret;
}

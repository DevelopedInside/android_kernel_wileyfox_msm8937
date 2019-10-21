#ifndef __GF_REGS_H
#define __GF_REGS_H

#define GF_VENDORID_ADDR    0x0006
#define GF_CHIP_ID0         0x0000
#define GF_CHIP_ID1         0x0002
#define GF_OTP_ADDR1        0x102
#define GF_OTP_ADDR2        0x104
#define GF_OTP_ADDR3        0x106


#define GF_IRQ_CTRL0        0x0120  /* RW, Interrupt enable*/
#define GF_IRQ_CTRL2        0x0124
#define GF_IRQ_CTRL3        0x0126  /* RO, Interrupt flags*/
#define GF_FDT_AREA_NUM     0x0CA   /* RW, threshold of touch num.*/

#define GF_MODE_CTRL0       0x0020  /* RW, osc and timer wakeup*/
#define GF_MODE_CTRL1       0x0022  /* RW, timer related*/
#define GF_MODE_CTRL2       0x0024  /* RW, timer threshold*/
#define GF_MODE_CTRL3       0x0026  /* RW, timer threshold*/

#define GF_PIXEL_CTRL0      0x0050
#define GF_PIXEL_CTRL1      0x0052
#define GF_PIXEL_CTRL2      0x0054
#define GF_PIXEL_CTRL3      0x0056   /* RW, start block*/
#define GF_PIXEL_CTRL4      0x0058   /* RW, block length*/
#define GF_PIXEL_CTRL6      0x005C   

#define GF_FDT              0x0080  /* RW, FDT enable and Up/Down*/ 
#define GF_FDT_DELTA        0x0082
#define GF_CMP_NUM_CTRL     0x0BA   /* RW, compare num. threshold*/ 
#define GF_FDT_THR0         0x084   /* RW, compare base and aver. base*/
#define GF_FDT_THR1         0x086
#define GF_FDT_THR2         0x088
#define GF_FDT_THR3         0x08A
#define GF_FDT_THR4         0x08C
#define GF_FDT_THR5         0x08E
#define GF_FDT_THR6         0x090
#define GF_FDT_THR7         0x092
#define GF_FDT_THR8         0x094
#define GF_FDT_THR9         0x096
#define GF_FDT_THR10        0x098
#define GF_FDT_THR11        0x09A
#define GF_ENCRYPT_EN       0x0070
#define GF_ENCRYPT_CTRL1    0x0072
#define GF_ENCRYPT_CTRL2    0x0074

#define GF_DATA_BUFFER        0xAAAA
#endif

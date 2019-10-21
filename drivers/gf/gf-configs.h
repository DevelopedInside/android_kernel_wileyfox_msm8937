/******
  *** gf_configs.h specific the configs of MilanF Chip
  *** Different vendor id with different configs
  *** There exist 4 different vendor id from 0x0000 to 0x0011
 ***/
#ifndef __GF_CONFIGS_H
#define __GF_CONFIGS_H

#include "gf-milanf.h"
#include "gf-regs.h"

/** 
 * vendor id = 0x0000 (nower used)
 */
struct gf_configs v0_fdt_down_cfg[]=
{
	{GF_IRQ_CTRL0, 0x0482}, /*enable fdt INT*/
	{GF_MODE_CTRL0,0x4010},
	{GF_MODE_CTRL1,0x2001}, 
	{GF_MODE_CTRL2,0x0014}, 
	{GF_PIXEL_CTRL6,0x0100},
	{GF_FDT_DELTA,0x157F},
	{GF_FDT_AREA_NUM,0x0007},
	{GF_FDT,0x0401},        /*fdt enabel, detect down*/
};
struct gf_configs v0_nav_fdt_down_cfg[]=
{
	{GF_IRQ_CTRL0, 0x0482}, /*enable fdt INT*/
	{GF_MODE_CTRL0,0x4010},
	{GF_MODE_CTRL1,0x2001}, 
	{GF_MODE_CTRL2,0x0014}, 
	{GF_PIXEL_CTRL6,0x0100},
	{GF_FDT_DELTA,0x157F},
	{GF_FDT_AREA_NUM,0x0000},
	{GF_FDT,0x0001},        /*fdt enabel, detect down*/
};
struct gf_configs v0_fdt_up_cfg[]=
{
	{GF_IRQ_CTRL0, 0x0482}, /*enable fdt INT*/
	{GF_MODE_CTRL0,0x4010},
	{GF_MODE_CTRL1,0x2001}, 
	{GF_MODE_CTRL2,0x0014}, 
	{GF_PIXEL_CTRL6,0x0100},        
	{GF_FDT_DELTA,0x137F},
	{GF_FDT,0x0003},        /*fdt enabel, detect up*/
};
struct gf_configs v0_ff_cfg[]=
{
	{GF_IRQ_CTRL0, 0x0402}, /*enable fdt INT*/
	{GF_MODE_CTRL0,0x4010},	
	{GF_MODE_CTRL1,0x2001}, 
	{GF_MODE_CTRL2,0x0032}, 
	{GF_PIXEL_CTRL6,0x0100},       
	{GF_FDT_DELTA,0x157F},
	{GF_FDT,0x0001},        /*fdt enabel, detect down*/	
};
struct gf_configs v0_nav_cfg[]=
{
	{GF_IRQ_CTRL0, 0x0408}, /*enable data_int INT*/
	{GF_MODE_CTRL1,0x0810}, /*manual mode*/
	{GF_PIXEL_CTRL6,0x0100},
	{GF_FDT,0x0001},        /*fdt enabel, detect down*/
};
struct gf_configs v0_nav_img_cfg[]=
{
	{GF_PIXEL_CTRL6,0x0080},       /*set rate 128*/
	{GF_IRQ_CTRL0,0x0408},         /*enable data_int INT*/
	{GF_PIXEL_CTRL1,0x0008},       /*set one_frame mode*/
	{GF_PIXEL_CTRL0,0x0501},	
};
struct gf_configs v0_img_cfg[]=
{
	{GF_PIXEL_CTRL6,0x0080},       /*set rate 128*/
	{GF_IRQ_CTRL0,0x0408},         /*enable data_int INT*/
	{GF_PIXEL_CTRL1,0x0008},       /*set one_frame mode*/
	{GF_PIXEL_CTRL0,0x0501},	
};
struct gf_configs v0_nav_img_manual_cfg[]=
{
	{GF_PIXEL_CTRL6,0x0080},
	{GF_IRQ_CTRL0,0x0408},
	{GF_PIXEL_CTRL1,0x0004}, /*Manual Mode Enable*/
	{GF_PIXEL_CTRL0,0x0501},
	{GF_PIXEL_CTRL2,0x0100}, /*Shadow Mode Select*/ 
};
/** 
 * vendor id = 0x0001 (to be added)
 */
/** 

 * vendor id = 0x0010 (to be added)
 */
 
/** 
 * vendor id = 0x0011 (to be added)
 */

#endif //__GF_CONFIGS_H


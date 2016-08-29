#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <asm/atomic.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include "ilitek_gesture.h"
#include "ilitek_gesture_parameter.h"
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <asm/processor.h>
#include <linux/kernel.h>

#define s8 char
#define s16 short
#define s32 int

#define meta_delta_x		16
#define meta_delta_y		16
#define NormP	16
#define PointMaxNum	350

//================================== Slide Define ====================================
#define slide_wide_channel       15 //CH*10
#define slide_length_channel     40 //CH*10

#define slide_wide	    (720/12)*slide_wide_channel/10
#define slide_length	(1280/21)*slide_length_channel/10

#define slide_x		0
#define slide_y		1

static unsigned char  feature_point_is_less;
//s16 point_count = 0;
//s16 index_sample_x;
//s16 index_sample_y = 0;
static unsigned short sample_point_count=0;
static s16  result_x[NormP],result_y[NormP];
static short  result_x1[NormP],result_y1[NormP];
static s16  max_x,min_x,max_y,min_y;
static s16  min_x_is,min_x_ie,min_x_i2;//miller
static s16 slope_x1,slope_x2,slope_x1_d,slope_x2_d;//miller
static s16  data_sample_x[PointMaxNum],data_sample_y[PointMaxNum];
static s16  start_x = 0;
static s16  start_y = 0;
static char b_buf[65535];
static int get_status = 0;
static unsigned char  GestureMatch_Run_Flag=0;
static s16  match_status=-1;
static void ilitek_fetch_object_sample(s16 curx,s16 cury);
static void ilitek_fetch_object_sample_2level(s16 *pdata_sample_x,s16 *pdata_sample_y);
static void ilitek_sampledata_to_standardizing(s16 *pdata_sample_x,s16 *pdata_sample_y);
static void ilitek_sampledata_to_normalizing(s16 *pdata_sample_x,s16 *pdata_sample_y);
static void ilitek_feature_to_transform(void);
static void ilitek_feature_to_match(void);
static void ilitek_slide_to_match(int SlideDirect);

short ilitek_gesture_model_value_x(int i);
short ilitek_gesture_model_value_y(int i);

void ilitek_readgesturelist(void){
	int ret,r,temp0,temp1,temp2,temp3,temp4,temp5,temp6,temp7,temp8,temp9,temp10,temp11,temp12,temp13,temp14,temp15,i,j;
	char *test,*p,*temp_str;
	struct file *filp;
	mm_segment_t oldfs;
	test = kmalloc(200,GFP_KERNEL);
	temp_str = kmalloc(10,GFP_KERNEL);
	oldfs=get_fs();
	set_fs(KERNEL_DS);
	msleep(100);
	memset(temp_str, 0, 10);
	filp=filp_open("/sdcard/gesture.txt",O_CREAT|O_RDWR,0777);
	if(IS_ERR(filp)) {
		printk("\nFile Open Error:%s\n","/sdcard/gesture.txt");
	}
	if(!filp->f_op) {
		printk("\nFile Operation Method Error!!\n");
	}
	printk("define E_N=%d\n",E_N);
	printk("\nFile Offset Position:%xh\n",(unsigned)filp->f_pos);
	ret=filp->f_op->read(filp,b_buf,sizeof(b_buf),&filp->f_pos);
	printk("\n%d,Read %xh bytes \n%s\n",__LINE__,ret,b_buf);
	p = strstr(b_buf,"ilitek_model_number");
	r = strcspn(p,"\n");
	memset(test, 0, 200);
	memset(ilitek_e_str, 0, 50);
	memset(ilitek_e, 0, 50);
	strncpy(test,p,r);
	printk("\n%d,Read %xh bytes \n%s\n",__LINE__,r,test);
	sscanf(test,"%s %d",temp_str,&i);
	printk("%s,%d\n",temp_str,i);
	sscanf(test,"%s = %d",temp_str,&i);
	printk("%s,%d\n",temp_str,i);
	ilitek_model_number = i;
	printk("%d,ilitek_model_number=%d\n",__LINE__,ilitek_model_number);
	p = strstr(p,"{");
	r = strcspn(p,"}");
	memset(test, 0, 0x3d);
	strncpy(test,p,r);
	printk("\n%d,Read %xh bytes \n%s\n",__LINE__,r,test);
	printk("---------------------\n");
	for(i = 2,j = 0;i < r; i=i+4,j++){
		printk("ilitek_e_str[%d]=%c\n",j,ilitek_e_str[j]);
		memcpy(&ilitek_e_str[j],test+i,1);
		printk("ilitek_e_str[%d]=%c\n",j,ilitek_e_str[j]);
	}
	printk("---------------------\n");
	for(i=0;i<ilitek_model_number*2;i++){
	printk("---------------------\n");
		p = strstr(p,"{");
		p = strstr(p,"0");
		r = strcspn(p,"}");
		memset(test, 0, 0x3d);
		strncpy(test,p,r);
		printk("\n%d,Read %xh bytes \n%s\n",__LINE__,r,test);
		sscanf(test,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",&temp0,&temp1,&temp2,&temp3,&temp4,&temp5,&temp6,&temp7,&temp8,&temp9,&temp10,&temp11,&temp12,&temp13,&temp14,&temp15);
		ilitek_e[i][0] = temp0;
		ilitek_e[i][1] = temp1;
		ilitek_e[i][2] = temp2;
		ilitek_e[i][3] = temp3;
		ilitek_e[i][4] = temp4;
		ilitek_e[i][5] = temp5;
		ilitek_e[i][6] = temp6;
		ilitek_e[i][7] = temp7;
		ilitek_e[i][8] = temp8;
		ilitek_e[i][9] = temp9;
		ilitek_e[i][10] = temp10;
		ilitek_e[i][11] = temp11;
		ilitek_e[i][12] = temp12;
		ilitek_e[i][13] = temp13;
		ilitek_e[i][14] = temp14;
		ilitek_e[i][15] = temp15;
		printk("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",i,temp0,temp1,temp2,temp3,temp4,temp5,temp6,temp7,temp8,temp9,temp10,temp11,temp12,temp13,temp14,temp15);
		p = strstr(p,"\n");
		if(strncmp(p,"}",1)==0){
		//if(p==NULL){
			printk("---break---\n");
			break;
		}
	}
	filp_close(filp,NULL);
	set_fs(oldfs);
	
}
char ilitek_GetGesture() {
	printk(KERN_ALERT "%s,match_status=%d\n", __func__,match_status);
	memset(data_sample_x, 0, 350);
	memset(data_sample_y, 0, 350);
	memset(result_x, 0, 16);
	memset(result_y, 0, 16);
	
	if(match_status>=125){
		return 0;
	}
	printk(KERN_ALERT "get_status=%c,%d\n",ilitek_e_str[get_status],ilitek_e_str[get_status]);
	return ilitek_e_str[get_status];
}

short ilitek_gesture_model_value_x(int i)
{
	printk("%s\n",__func__);
	return result_x[i]; 
}
short ilitek_gesture_model_value_y(int i)
{
	printk("%s\n",__func__);
	return result_y[i]; 
}
char ilitek_GestureMatchProcess(unsigned char ucCurrentPointNum,unsigned char ucLastPointNum,unsigned short curx,unsigned short cury)
{
	ilitek_e_str[60]='d';
	ilitek_e_str[61]='u';
	ilitek_e_str[62]='l';
	ilitek_e_str[63]='r';

	printk(KERN_ALERT "%s\n", __func__);
	
    if (ucCurrentPointNum == 1)
    {
	    if (ucLastPointNum == 0)
	    {
	        GestureMatch_Run_Flag = 1;
		match_status=0;
	        //start get point datas
	    }
	  }
    else if (GestureMatch_Run_Flag > 0)
    {
	    if (ucCurrentPointNum == 0)
	    {
	        GestureMatch_Run_Flag = 4;
	        //finish get point datas
	    }
	    else
	    {
	    	  sample_point_count=0;
	        GestureMatch_Run_Flag = 0;
	        //give up the match process.
	    }
    }
    //else
    //{
        //GestureMatch_Run_Flag = 0;
        //none done;
    //}
    if(GestureMatch_Run_Flag > 0)
    {
        //start match
         ilitek_fetch_object_sample(curx,cury);
		 ////printk("GestureMatch_Run_Flag=%d,match_status=%d",GestureMatch_Run_Flag,match_status);
	 return match_status;
    }
    else
    {
    	match_status=-1;
    	return 0;
    }
    // printk("s_str=%c\n",ilitek_e_str[51]);
}
//EXPORT_SYMBOL(GestureMatchProcess);
static void ilitek_fetch_object_sample(s16 curx,s16 cury)
{
	//s16 i,j;
	s16 widerx;
	s16 widery;
	if(GestureMatch_Run_Flag==1)
	{//init parameters
		//printk("ilitek_fetch_object_sample %d\n",__LINE__);
		start_x = curx;
		start_y = cury;
		data_sample_x[0] = start_x;
		data_sample_y[0] = start_y;
		max_x = start_x;
		min_x = start_x;
		max_y = start_y;
		min_y = start_y;
		sample_point_count = 1;
		GestureMatch_Run_Flag=2;//continue get datas...
		
	}
	else if(GestureMatch_Run_Flag==2)
	{
		s16 cur_dx,cur_dy;
		if(curx >= start_x) 
		{
			cur_dx = curx - start_x;
		}
		else
		{
			cur_dx = start_x - curx;
		}

		if(cury >= start_y) 
		{
			cur_dy = cury - start_y;
		}
		else
		{
			cur_dy = start_y - cury;
		}
		
		if((cur_dx > meta_delta_x) || (cur_dy > meta_delta_y))
		{
			start_x = curx;
			start_y = cury;
			data_sample_x[sample_point_count] = start_x;
			data_sample_y[sample_point_count] = start_y;
			//printk( "data_sample_x [%d]= %d ,data_sample_y[%d]=%d\n ",sample_point_count,data_sample_x[sample_point_count],sample_point_count,data_sample_y[sample_point_count]);
			
			if(curx > max_x)
				max_x = curx;				
			else if (curx <= min_x)
				min_x = curx;

			if(cury > max_y)
				max_y = cury;				
			else if (cury <= min_y)
				min_y = cury;

			sample_point_count++;
			//printk("sample_point_count=%d\n",sample_point_count);
		}
		if(sample_point_count >= PointMaxNum)
		{
			//break;
			//printk("line=%d,sample_point_count=%d\n",__LINE__,sample_point_count);
			GestureMatch_Run_Flag=4;
		}
	//printk("curx=%d,cury=%d,start_x=%d,start_y=%d,cur_dx=%d,cur_dy,max_x=%d,min_x=%d,max_y=%d,min_y=%d\n",curx,cury,start_x,start_y,cur_dx,cur_dy,max_x,min_x,max_y,min_y);
	}
	if(GestureMatch_Run_Flag==4)
	{
		if(sample_point_count>4)
		{
			unsigned char wide_ratio;
	    	       {

		    	widerx=(max_x-min_x);
		    	widery=(max_y-min_y);
		        if(widerx>=widery)
		        	wide_ratio=widerx/widery;
			    else
			    	wide_ratio=widery/widerx;
	    	       }
            
			if(wide_ratio<=5)
			{
				ilitek_sampledata_to_standardizing(data_sample_x,data_sample_y);
				ilitek_fetch_object_sample_2level(data_sample_x,data_sample_y);
				ilitek_sampledata_to_normalizing(data_sample_x,data_sample_y);
				ilitek_feature_to_transform();
			} else {
				if(~((widerx > slide_wide) && (widery > slide_wide)))
				{
					if(widerx > slide_length)
					{
						ilitek_slide_to_match(slide_x);//horizontal slide
					}
					else if(widery > slide_length)
					{
						ilitek_slide_to_match(slide_y);//vertical slide
					}
				}
				else
				{
					printk("big length-width ratio\n");
					match_status=125;
				}
			}
		} else {
			//printk( "There is less featrue point ");
			match_status=127;
		}
		GestureMatch_Run_Flag=0;
		sample_point_count=0;
	}
}
static void ilitek_fetch_object_sample_2level(s16 *pdata_sample_x,s16 *pdata_sample_y)
{
	s16 i;
	s16 index;

	s8 curx;
	s8 cury;
	s8 cur_dx;
	s8 cur_dy;
	s8 start_x;
	s8 start_y;
	unsigned char insert_flag;
	s16 *pdata_out_x,*pdata_out_y;
	
	/////miller
	for(i=0;i<sample_point_count;i++)
	{
		if(pdata_sample_x[i]==0)
		{
			min_x_is=i;
			break;
		}
	}
	for(i=min_x_is;i<sample_point_count;i++)
	{
		if(pdata_sample_x[i]!=0)
		{
			min_x_ie=i-1;
			break;
		}
	}
	min_x_i2=(sample_point_count+2*min_x_ie)/3;
	slope_x1=0;
	slope_x2=0;
	slope_x1_d=0;
	slope_x2_d=0;

	if(pdata_sample_x[0]==0)
		pdata_sample_x[0]=1;
	if(pdata_sample_x[min_x_is*2/3]==0)
		pdata_sample_x[min_x_is*2/3]=1;
	if(pdata_sample_x[sample_point_count-1]==0)
		pdata_sample_x[sample_point_count-1]=1;
	if(pdata_sample_x[min_x_i2]==0)
		pdata_sample_x[min_x_i2]=1;
	
	slope_x1 = ((ABSSUB(pdata_sample_y[min_x_is],pdata_sample_y[0]))*100)/pdata_sample_x[0];
	slope_x2 = ((ABSSUB(pdata_sample_y[min_x_is*2/3],pdata_sample_y[min_x_is]))*100)/pdata_sample_x[min_x_is*2/3];
	slope_x1_d = ((ABSSUB(pdata_sample_y[min_x_ie],pdata_sample_y[sample_point_count-1]))*100)/pdata_sample_x[sample_point_count-1];
	slope_x2_d = ((ABSSUB(pdata_sample_y[min_x_i2],pdata_sample_y[min_x_ie]))*100)/pdata_sample_x[min_x_i2];
	
	
	if(sample_point_count<=50)
	{
		insert_flag=1;
		pdata_out_x=pdata_sample_x+sample_point_count;
		pdata_out_y=pdata_sample_y+sample_point_count;
	}
	else
	{
		insert_flag=0;
		pdata_out_x=pdata_sample_x;
		pdata_out_y=pdata_sample_y;
	}
	
	start_x = (s8)pdata_sample_x[0];
	start_y = (s8)pdata_sample_y[0];
	
	index = 1;
	for(i = 1;i < sample_point_count;i++)
	{
		//printk("ilitek_fetch_object_sample_2level,----pdata_sample_x[%d]=%d,pdata_sample_y[%d]=%d\n",i,pdata_sample_x[i],i,pdata_sample_y[i]);
	    curx = (s8)pdata_sample_x[i];
	    cury = (s8)pdata_sample_y[i];
	    //printk("ilitek_fetch_object_sample_2level,----curx[%d]=%d,cury[%d]=%d\n",i,curx,i,cury);
	    cur_dx = abs(curx - start_x);
	 //   if(cur_dx < 0) 
	 //   {
	 //   	cur_dx = -cur_dx;
	 //   }

	    cur_dy = abs(cury - start_y);
	 //   if(cur_dy < 0) 
	 //   {
	 //   	cur_dy = -cur_dy;
	 //   }
		//printk("cur_dx=%d,cur_dy=%d,",cur_dx,cur_dy);	  
    	if((cur_dx >= 2) || (cur_dy >= 2))
    	{
	       if(insert_flag>0)
            {
                       if((index-i+(sample_point_count<<1))<(PointMaxNum-4))
                       {
                             if((cur_dx >= 8) || (cur_dy >= 8))
                             {
                                 index=index+1;
                                 pdata_out_x[index] = (start_x+curx)>>1;
                                 pdata_out_y[index] = (start_y+cury)>>1;
                                 pdata_out_x[index-1] = (start_x+pdata_out_x[index])>>1;
                                 pdata_out_y[index-1] = (start_y+pdata_out_y[index])>>1;
                                 pdata_out_x[index+1] = (pdata_out_x[index]+curx)>>1;
                                 pdata_out_y[index+1] = (pdata_out_y[index]+cury)>>1;
                                 index=index+2;
								 //printk("index=%d,i=%d,",index,i);	    			
                             }
                             else if((cur_dx >= 4) || (cur_dy >= 4))
                             {
                                 pdata_out_x[index] = (s16)((start_x+curx)>>1);
                                 pdata_out_y[index] = (s16)((start_y+cury)>>1);
                                 index++;
								 //printk("index=%d,i=%d,",index,i);
                             }
                       }
                else
                {
                insert_flag=-1;
                }
            }
    		start_x = curx;
    		start_y = cury;
    		pdata_out_x[index] = (s16)curx;
    		pdata_out_y[index] = (s16)cury;
    		index++;
			//printk("index=%d,i=%d\n",index,i);
    	}	    			
	}
	sample_point_count = index;
	//printk("ilitek_fetch_object_sample_2level,sample_point_count=%d,index=%d\n",sample_point_count,index);	
	if(insert_flag!=0)
	{
		for(i = 1;i < sample_point_count;i++)
		{
    		pdata_sample_x[i] = pdata_out_x[i];
    		pdata_sample_y[i] = pdata_out_y[i];
		}
	}
}
static void ilitek_sampledata_to_standardizing(s16 *pdata_sample_x,s16 *pdata_sample_y)
{
	s16 i = 0;
	s16 betaDx = 0;
	s16 betaDy = 0;

	betaDx = max_x - min_x;
	if(betaDx <= 0)
		betaDx = 1;
	
	betaDy = max_y - min_y;
	if(betaDy <= 0)
		betaDy = 1;
	//printk("max_x=%d,min_x=%d,betaDx=%d,max_y=%d,min_y=%d,betaDy=%d\n",max_x,min_x,betaDx,max_y,min_y,betaDy);
	for(i = 0;i < sample_point_count;i++)
	{
		pdata_sample_x[i] = ((pdata_sample_x[i] - min_x)<<5)/betaDx;
		pdata_sample_y[i] = ((pdata_sample_y[i] - min_y)<<5)/betaDy;
		//printk("pdata_sample_x[%d]=%d,pdata_sample_y=[%d]=%d\n",i,pdata_sample_x[i],i,pdata_sample_y[i]);
	}
}

static void ilitek_sampledata_to_normalizing(s16 *pdata_sample_x,s16 *pdata_sample_y)
{
	unsigned short i,i0;
	unsigned short j,j0;
	s8 curv;
	s8 overlay;
	if(sample_point_count >= NormP)
	{
		feature_point_is_less = 0;

		for(i0 = 2;i0 <= sample_point_count;i0++)
		{
			j0 = (i0 <<4)/sample_point_count;//normalize to 16point
			if(j0 < 2) j0 = 2;
			i=i0-1;
			j=j0-1;
		
			curv = (s8)pdata_sample_x[i];
			overlay = (s8)pdata_sample_x[j];

			if(overlay == 0)
			{
				pdata_sample_x[j] = (s16)curv;
				if(i != j) pdata_sample_x[i] = 0;
			}
			else
			{
				if(i != j)
				{
					pdata_sample_x[j] = (s16)((curv + overlay)>>1);
					pdata_sample_x[i] = 0;
				}
			}

			curv = (s8)pdata_sample_y[i];
			overlay = (s8)pdata_sample_y[j];

			if(overlay == 0)
			{
				pdata_sample_y[j] = (s16)curv;
				if(i != j) pdata_sample_y[i] = 0;
			}
			else
			{
				if(i != j)
				{
					pdata_sample_y[j] =(s16)((curv + overlay)>>1);
					pdata_sample_y[i] = 0;
				}
			}
		}

		for(i = 0;i < NormP;i++)
		{
			//char  rx[NormP]={0,15,21,24,23,17,11,5,1,1,2,8,15,22,28,31};
			//char  ry[NormP]={25,16,11,5,0,1,3,8,15,21,27,30,31,29,24,21};
			//result_x[i] = (s8)rx[i];
			//result_y[i] = (s8)ry[i];
			result_x[i] = (s8)pdata_sample_x[i];
			result_y[i] = (s8)pdata_sample_y[i];
			result_x1[i] = (s8)pdata_sample_x[i];
			result_y1[i] = (s8)pdata_sample_y[i];
			//printk("result_x[%d]=%d,result_y[%d]=%d\n",i,result_x[i],i,result_y[i]);
			
		}	
		//printk("\n");
	}
	else
	{
		feature_point_is_less = 1;
	}
}

static void ilitek_feature_to_transform()
{
	//s8 curv;
	s16 startx,starty;
	s16 i = 0;

	if(!feature_point_is_less)
	{
		//printk("----%s,%d----,if(!feature_point_is_less)\n",__func__,__LINE__);
		startx=result_x[0];
		starty=result_y[0];
		result_x[0]=0;
		result_y[0]=0;
		result_x1[0]=0;
		result_y1[0]=0;
		////printk("result_x[0]=0,result_y[0]=0\n");
		for(i = 1;i < NormP;i++)
		{
			result_x1[i]=(result_x[i]-startx)<<1;
			result_y1[i]=(result_y[i]-starty)<<1;
			result_x[i]=(result_x[i]-startx)<<1;
			result_y[i]=(result_y[i]-starty)<<1;
		}
		ilitek_feature_to_match();
	}
	else
	{
		//printk("----%s,%d----else(!feature_point_is_less)\n",__func__,__LINE__);
		////printk(KERN_WARNING "There is less featrue point ");
		match_status=127;
	}
}
				   
static void ilitek_feature_to_match()
{
	unsigned short i,j;
	s16 rt;	s8 r1,r2,r,rm;
	s16 sx1,sx2;	
	s16 sx3,sx4,sx5;
	s16 sx6,sx7,sx8;
	s16 sy1,sy2;	
	s16 sy3,sy4,sy5;
	s16 sy6,sy7,sy8;
	unsigned short maxi=0;
	s8 maxr=0;
	s16 sumdiff_x,sumdiff_y;
    s16 max_dx,max_dy,max_d;
	s16  *pe;
	s16  *pe2;
	s8 x_flag=0,y_flag=0,*temp;
	get_status = 0;
#if 1
	printk("ilitek x ");
	for(j = 0;j < NormP;j++)
		printk("%d,",result_x[j]);
	printk("\n");
	printk("ilitek y ");
	for(j = 0;j < NormP;j++)
		printk("%d,",result_y[j]);
	printk("\n");
#endif
	for(i = 0;i < ilitek_model_number;i++)
	{
		sumdiff_x = 0;
		sumdiff_y = 0;
	    max_dx = 0;	    		max_dy = 0;
	    max_d=0;
		pe=ilitek_e[i << 1];
		pe2=ilitek_e[(i << 1)+1];
		//for(j = 0;j < NormP;j++)
		//	printk("pe[%d]=%c,",j,pe[i]);
		//printk("\n");	
		//for(j = 0;j < NormP;j++)
		//	printk("pe2[%d]=%d,",j,pe2[i]);
		//printk("\n");	
		//printk("-----i=%d\n",i);
		for(j = 1;j < NormP;j++)
		{
		    s16 tx,ty;
			x_flag = 0;
			y_flag = 0;
		    tx=abs(pe[j]-result_x[j]);
			if(tx>max_dx)
			{
				max_dx=tx;
				//if(i>0 && i<=3)
				//	printk("i=%d,pe[%d]=%d,result_x[%d]=%d,tx=%d\n",i,j,pe[j],j,result_x[j],tx);
			}
		    ty=abs(pe2[j]-result_y[j]);
			if(ty>max_dy)
			{
				max_dy=ty;
				//if(i>0 && i<=3)
				//	printk("i=%d,pe2[%d]=%d,result_y[%d]=%d,ty=%d\n",i,j,pe2[j],j,result_y[j],ty);
			}
			sumdiff_x=sumdiff_x+tx;
			sumdiff_y=sumdiff_y+ty;
			//if(i>0 && i<=3)
			//	printk("pe_x[%d]=%d,result_x[%d]=%d,pe_y[%d]=%d,result_y[%d]=%d,sumdiff_x=%d,sumdiff_y=%d\n",j,pe[j],j,result_x[j],j,pe2[j],j,result_y[j],sumdiff_x,sumdiff_y);
		    ty=tx+ty;
			if(ty>max_d)
			{
				////printk("----%s,%d----.if(ty>max_d)\n",__func__,__LINE__);
				max_d=ty;
			}
			
			//printk("----------------------max_dx[%d]=%d,max_dy[%d]=%d\n",j,max_dx,j,max_dy);
		}
		if((sumdiff_x>384)||(sumdiff_y>384))
		{
			//printk("if((sumdiff_x>384)||(sumdiff_y>384))\n");
			continue;
		}
		if((max_dx>=45)||(max_dy>=45))
		{
			//printk("if((max_dx>=45)||(max_dy>=45))\n");
			continue;
		}
	    rm=0;
	    if(max_d>=40)
	    {
			//printk("----%s,%d---- if(max_d>=40)\n",__func__,__LINE__);
	    	rm=((max_d-40)<<3)/(48-40);
	    	continue;
	    }
		
		sx1 = 0;	    		sx2 = 0;
		sx3 = 0;	    		sx4 = 0;	    		sx5 = 0;
		pe=ilitek_e[i << 1];
		for(j = 1;j < NormP;j++)
		{
			s16 ex,rx;
			ex=(s16)pe[j];
			rx=(s16)result_x[j];
			sx1 += rx;
			sx2 += ex;
			sx3 +=(rx * rx)>>4;
			sx4 += (ex * ex)>>4;
			sx5 += (rx * ex)>>4;
		}
		sx1 = sx1>>4;	    		sx2 = sx2>>4;
		sx6 = sx5 - (sx1*sx2);
		if(sx6<=0)
		{
			//printk("----%s,%d----if(sx6<=0)\n",__func__,__LINE__);
			continue;
		}
		sx7 = sx3 - (sx1*sx1);
		sx8 = sx4 - (sx2*sx2);
		if((sx7 == 0)||(sx8 == 0)){
			//printk("----%s,%d----if((sx7 == 0)||(sx8 == 0)){\n",__func__,__LINE__);
			r1 = 100;
		}
		else
		{
			sx6=sx6>>4;
			rt =sx6*25;
			rt=(s16)((s32)rt/(sx7>>4));
			rt=rt*sx6;
			r1=(s8)((s32)rt/(sx8>>6));
		}
		if(r1<56)
		{
			//printk("----%s,%d----if(r1<56)\n",__func__,__LINE__);
			continue;
		}

		sy1 = 0;	    		sy2 = 0;
		sy3 = 0;	    		sy4 = 0;	    		sy5 = 0;
		pe=ilitek_e[(i << 1)+1];
		for(j = 1;j < NormP;j++)
		{
			s16 ry,ey;
			ey=(s16)pe[j];
			ry=(s16)result_y[j];
			sy1 += ry;
			sy2 += ey;
			sy3 +=(ry * ry)>>4;
			sy4 +=(ey * ey)>>4;
			sy5 +=(ry * ey)>>4;
		}
		sy1 = sy1>>4;
		sy2 = sy2>>4;
		sy6 = sy5 - (sy1*sy2);
		if(sy6<=0)
		{
			//printk("----%s,%d----if(sy6<=0)\n",__func__,__LINE__);
			continue;
		}
		sy7 = sy3 - (sy1*sy1);
		sy8 = sy4 - (sy2*sy2);

		if((sy7 == 0)||(sy8 == 0)){
			r2 = 100;
		}
		else
		{
			//printk("----%s,%d----else((sy7 == 0)||(sy8 == 0))\n",__func__,__LINE__);
			sy6=sy6>>4;
			rt =sy6*25;
			rt=(s16)((s32)rt/(sy7>>4));
			rt=rt*sy6;
			r2=(s8)((s32)rt/(sy8>>6));
		}
		if(r2<56)
		{
			//printk("----%s,%d----if(r2<56)\n",__func__,__LINE__);
			continue;
		}

		r = (r1>>1)+(r2>>1)-rm;
		
		//szOutText = "";
    	//szOutText = szOutText + "#"  + "r: " + ilitek_e_str[i] + " (" + i + ") " + r + "\r\n";	
		//Report.WriteStringIntoReport(szOutText,0);
		////printk(KERN_WARNING "r: %c(%d), r  is %d\n",ilitek_e_str[i],i,r);
		//Toast.makeText(getApplicationContext(), "r:" + ilitek_e_str[i] + "(" + i + ")" + r,
				//Toast.LENGTH_SHORT).show();
		if(r>maxr)
		{
			
			maxr=r;
			maxi=i;
		}
		//printk("----%s,%d--maxi=%d,maxr=%d,\n",__func__,__LINE__,maxi,maxr);
	}


	if((maxr >= 70)&&(ilitek_e_str[maxi]!=0xff))
	{
		//int j;
		//for(j = 0;j < NormP;j++)
		//	printk("%d,",ilitek_e[maxi*2][j]);
		//printk("\n");	
		//for(j = 0;j < NormP;j++)
		//	printk("%d,",ilitek_e[maxi*2+1][j]);
		//printk("\n");
		
		if((maxi>=0 && maxi<=3)&&((slope_x2+slope_x2_d-slope_x1-slope_x1_d)<75))
			maxi=33;//c to <
		if((maxi>=33 && maxi<=38)&&((slope_x2+slope_x2_d-slope_x1-slope_x1_d)>=75))
			maxi=0;//< to c
		
		match_status=maxi+1;
		temp = &ilitek_e_str[maxi];
		//if(strncmp(temp,"c",1)==0){
		//	
		//	//get_status = 1;
		//	printk("%d,temp=%s,get_status=%d\n",__LINE__,temp,get_status);
		//}
		//else if(strncmp(temp,"e",1)==0){
		//	
		//	//get_status = 2;
		//	printk("%d,temp=%s,get_status=%d\n",__LINE__,temp,get_status);
		//}
		//else if(strncmp(temp,"m",1)==0){
		//	
		//	//get_status = 3;
		//	printk("%d,temp=%s,get_status=%d\n",__LINE__,temp,get_status);
		//}
		//else if(strncmp(temp,"w",1)==0){
		//	
		//	//get_status = 4;
		//	printk("%d,temp=%s,get_status=%d\n",__LINE__,temp,get_status);
		//}
		//else if(strncmp(temp,"o",1)==0){
		//	
		//	//get_status = 5;
		//	printk("%d,temp=%s,get_status=%d\n",__LINE__,temp,get_status);
		//}
		get_status = maxi;
		printk(KERN_ALERT "There max match char is %c(%d), r  is %d,match_status=%d,get_status=%d\n",ilitek_e_str[maxi],maxi,maxr,match_status,get_status);
		
		//break;
	} else {
		//printk("----%s,%d----\n",__func__,__LINE__);
		match_status=126;
		printk(KERN_ALERT "There input char is not match maxr=%d,match_status=%d,get_status=%d\n",maxr,match_status,get_status);
	}
}
static void ilitek_slide_to_match(int SlideDirect)
{
	s16 i;
	s16 cur_d1,cur_d2 = 0;
	cur_d1 = (SlideDirect == 0) ? (data_sample_x[1] - data_sample_x[0]) : (data_sample_y[1] - data_sample_y[0]);
	get_status = 0;
	for(i = 2;i < sample_point_count;i++)
	{
		if((cur_d1 > 0 && cur_d1 > cur_d2) || (cur_d1 < 0 && cur_d1 < cur_d2))
		{
			cur_d2 = cur_d1;
			cur_d1 = (SlideDirect == 0) ? (data_sample_x[i] - data_sample_x[0]) : (data_sample_y[i] - data_sample_y[0]);

			if(i == (sample_point_count - 2))
			{
				if(!SlideDirect && cur_d1 > 0)
				{
					get_status = 63;
					printk(">>>>>,get_status=%d\n",get_status);
				}
				else if(!SlideDirect && cur_d1 < 0)
				{
					get_status = 62;
					printk("<<<<<,get_status=%d\n",get_status);
				}
				else if(SlideDirect && cur_d1 > 0)
				{
					get_status = 60;
					printk("v,get_status=%d\n",get_status);
				}
				else if(SlideDirect && cur_d1 < 0)
				{
					get_status = 61;
					printk("^,get_status=%d\n",get_status);
				}
			}
		}
		else
		{
			continue;
		}
	}
}



#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define GPIO_LCD_BL_PWM_OUT  57 
#define GPIO_LCD_BL_EN_OUT   38   

#define LCD_BL_LEVEL 32
#define lcd_bl_max   lcd_bl_level-1
#define lcd_bl_min   0

static DEFINE_SPINLOCK(tps61061_lock);

static int himax_panel_initialized = 1;

static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int himax_reset;


static struct msm_panel_common_pdata * lcdc_himax_pdata;
static void gpio_lcd_emuspi_write_one_para(unsigned char addr, unsigned char para);
#if 0		
static void lcdc_himax_wakeup(void);
#endif
static void lcdc_himax_sleep(void);
static void lcdc_himax_init(void);

static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);
#ifdef CONFIG_MACH_JOE
static void lcdc_himax_sleep(void)
{
     
         gpio_lcd_emuspi_write_one_para(0x24, 0x38);
         mdelay(40);
         gpio_lcd_emuspi_write_one_para(0x24, 0x28);
         mdelay(40);
         gpio_lcd_emuspi_write_one_para(0x24, 0x00);
         

         gpio_lcd_emuspi_write_one_para(0x1E, 0x14);
         mdelay(10);
         gpio_lcd_emuspi_write_one_para(0x19, 0x02);
         mdelay(10);
         gpio_lcd_emuspi_write_one_para(0x19, 0x0A);
         mdelay(10);
         
         gpio_lcd_emuspi_write_one_para(0x1B, 0x40);
         mdelay(10);
         gpio_lcd_emuspi_write_one_para(0x3C, 0x00);
         mdelay(10);
         

         gpio_lcd_emuspi_write_one_para(0x19, 0x0B);
         mdelay(10);
         

         gpio_lcd_emuspi_write_one_para(0x17, 0x90);
         mdelay(10);                
  

}

#if 0		
static void lcdc_himax_wakeup(void)
{

	gpio_lcd_emuspi_write_one_para(0x17, 0x91);
	mdelay(10);
	gpio_lcd_emuspi_write_one_para(0x19, 0x0A);

	gpio_lcd_emuspi_write_one_para(0x17, 0x91);
	mdelay(10); 

	gpio_lcd_emuspi_write_one_para(0x1B, 0x13);
	gpio_lcd_emuspi_write_one_para(0x1A, 0x11);
	gpio_lcd_emuspi_write_one_para(0x1C, 0x0A);
	gpio_lcd_emuspi_write_one_para(0x1F, 0x58);
	mdelay(20);

	gpio_lcd_emuspi_write_one_para(0x19, 0x0A);
	mdelay(10);
	gpio_lcd_emuspi_write_one_para(0x19, 0x1A);
	mdelay(40);
	gpio_lcd_emuspi_write_one_para(0x19, 0x12);

	gpio_lcd_emuspi_write_one_para(0x1E, 0x2E);
	mdelay(100);


	gpio_lcd_emuspi_write_one_para(0x3C, 0xC0);
	gpio_lcd_emuspi_write_one_para(0x3D, 0x1C);
	gpio_lcd_emuspi_write_one_para(0x34, 0x38);
	gpio_lcd_emuspi_write_one_para(0x35, 0x38);         
	gpio_lcd_emuspi_write_one_para(0x24, 0x38);

	mdelay(40);

	gpio_lcd_emuspi_write_one_para(0x24, 0x3C);

}
#endif

static void lcdc_himax_init(void)
{
	gpio_direction_output(himax_reset, 1);
	mdelay(5);
	gpio_direction_output(himax_reset, 0);
	mdelay(10);
	gpio_direction_output(himax_reset, 1);
	mdelay(20);

	gpio_direction_output(spi_cs, 1);
	gpio_direction_output(spi_sdo, 1);
	gpio_direction_output(spi_sclk, 1);
	mdelay(20);
	gpio_lcd_emuspi_write_one_para(0x83, 0x02); 
	gpio_lcd_emuspi_write_one_para(0x85, 0x03); 
	gpio_lcd_emuspi_write_one_para(0x8B, 0x00); 
	gpio_lcd_emuspi_write_one_para(0x8C, 0x13); 
	gpio_lcd_emuspi_write_one_para(0x91, 0x01); 
	gpio_lcd_emuspi_write_one_para(0x83, 0x00); 
	mdelay(5);
	 
	gpio_lcd_emuspi_write_one_para(0x3E, 0xe2);//0xc4
	gpio_lcd_emuspi_write_one_para(0x3F, 0x26);//0x44
	gpio_lcd_emuspi_write_one_para(0x40, 0x00);//0x22
	gpio_lcd_emuspi_write_one_para(0x41, 0x55);//0x57
	gpio_lcd_emuspi_write_one_para(0x42, 0x06);//0x03
	gpio_lcd_emuspi_write_one_para(0x43, 0x17);//0x47
	gpio_lcd_emuspi_write_one_para(0x44, 0x21);//0x02
	gpio_lcd_emuspi_write_one_para(0x45, 0x77);//0x55
	gpio_lcd_emuspi_write_one_para(0x46, 0x01);//0x06
	gpio_lcd_emuspi_write_one_para(0x47, 0x0a);//0x4c
	gpio_lcd_emuspi_write_one_para(0x48, 0x05);//0x06
	gpio_lcd_emuspi_write_one_para(0x49, 0x02);//0x8c

	gpio_lcd_emuspi_write_one_para(0x2B, 0xF9); 
	mdelay(20);
	gpio_lcd_emuspi_write_one_para(0x17, 0x91); 
	gpio_lcd_emuspi_write_one_para(0x18, 0x3A); 
	gpio_lcd_emuspi_write_one_para(0x1B, 0x13);
	gpio_lcd_emuspi_write_one_para(0x1A, 0x11); 
	gpio_lcd_emuspi_write_one_para(0x1C, 0x0a); 
	gpio_lcd_emuspi_write_one_para(0x1F, 0x58); 

	mdelay(30);
	gpio_lcd_emuspi_write_one_para(0x19, 0x0A);             
	gpio_lcd_emuspi_write_one_para(0x19, 0x1A); 
	mdelay(50);
	gpio_lcd_emuspi_write_one_para(0x19, 0x12); 
	mdelay(50);
	gpio_lcd_emuspi_write_one_para(0x1E, 0x2e);
	mdelay(100);

	gpio_lcd_emuspi_write_one_para(0x3C, 0xC0); 
	gpio_lcd_emuspi_write_one_para(0x3D, 0x1C); 
	gpio_lcd_emuspi_write_one_para(0x34, 0x38);
	gpio_lcd_emuspi_write_one_para(0x35, 0x38);
	gpio_lcd_emuspi_write_one_para(0x24, 0x38);
	mdelay(50);
	gpio_lcd_emuspi_write_one_para(0x24, 0x3C);
	gpio_lcd_emuspi_write_one_para(0x16, 0x1C); 
	gpio_lcd_emuspi_write_one_para(0x3A, 0xce); 
	gpio_lcd_emuspi_write_one_para(0x01, 0x06); 
	gpio_lcd_emuspi_write_one_para(0x55, 0x00);



}
#else 
static void lcdc_himax_sleep(void)
{

	gpio_lcd_emuspi_write_one_para(0x24, 0x38);
	mdelay(40);

	gpio_lcd_emuspi_write_one_para(0x24, 0x28);
	mdelay(40);
	
	gpio_lcd_emuspi_write_one_para(0x24, 0x00);
	gpio_lcd_emuspi_write_one_para(0x1E, 0x14);
	mdelay(10);
	
	gpio_lcd_emuspi_write_one_para(0x19, 0x02);
	mdelay(10);

	gpio_lcd_emuspi_write_one_para(0x19, 0x0A);
	mdelay(10);

	gpio_lcd_emuspi_write_one_para(0x1B, 0x40);
	mdelay(10);

	gpio_lcd_emuspi_write_one_para(0x3c, 0x00);
	mdelay(10);

	gpio_lcd_emuspi_write_one_para(0x19, 0x0B);
	mdelay(10);
}

#if 0		
static void lcdc_himax_wakeup(void)
{
	gpio_lcd_emuspi_write_one_para(0x19, 0x0A);
	gpio_lcd_emuspi_write_one_para(0x1B, 0x14);
	gpio_lcd_emuspi_write_one_para(0x1A, 0x11);
	gpio_lcd_emuspi_write_one_para(0x1C, 0x0D);
	gpio_lcd_emuspi_write_one_para(0x1F, 0x42);
	mdelay(20);
	
	gpio_lcd_emuspi_write_one_para(0x19, 0x0A);
	gpio_lcd_emuspi_write_one_para(0x19, 0x1A);
	mdelay(40);
	gpio_lcd_emuspi_write_one_para(0x19, 0x12);
	mdelay(40);

	gpio_lcd_emuspi_write_one_para(0X1E, 0x2C);
	mdelay(100);

	gpio_lcd_emuspi_write_one_para(0x3C, 0x60);
	gpio_lcd_emuspi_write_one_para(0x3D, 0x1C);
	gpio_lcd_emuspi_write_one_para(0x34, 0x38);
	gpio_lcd_emuspi_write_one_para(0x35, 0x38);
	gpio_lcd_emuspi_write_one_para(0x24, 0x38);
	mdelay(40);

	gpio_lcd_emuspi_write_one_para(0x24, 0x3C);


}
#endif
static void lcdc_himax_init(void)
{
	gpio_direction_output(himax_reset, 1);
	udelay(10);
	gpio_direction_output(himax_reset, 0);
	udelay(50);
	gpio_direction_output(himax_reset, 1);

	gpio_direction_output(spi_cs, 1);
	gpio_direction_output(spi_sdo, 1);
	gpio_direction_output(spi_sclk, 1);
	mdelay(150);

	gpio_lcd_emuspi_write_one_para(0x83, 0x02);
	gpio_lcd_emuspi_write_one_para(0x85, 0x03);
	gpio_lcd_emuspi_write_one_para(0x8b, 0x00);
	gpio_lcd_emuspi_write_one_para(0x8c, 0x93);
	gpio_lcd_emuspi_write_one_para(0x91, 0x01);
	gpio_lcd_emuspi_write_one_para(0x83, 0x00);

	gpio_lcd_emuspi_write_one_para(0x3e, 0xb0);
	gpio_lcd_emuspi_write_one_para(0x3f, 0x03);
	gpio_lcd_emuspi_write_one_para(0x40, 0x10);
	gpio_lcd_emuspi_write_one_para(0x41, 0x56);
	gpio_lcd_emuspi_write_one_para(0x42, 0x13);
	gpio_lcd_emuspi_write_one_para(0x43, 0x46);
	gpio_lcd_emuspi_write_one_para(0x44, 0x23);
	gpio_lcd_emuspi_write_one_para(0x45, 0x76);
	gpio_lcd_emuspi_write_one_para(0x46, 0x00);
	gpio_lcd_emuspi_write_one_para(0x47, 0x5e);
	gpio_lcd_emuspi_write_one_para(0x48, 0x4f);
	gpio_lcd_emuspi_write_one_para(0x49, 0x40);
	
	gpio_lcd_emuspi_write_one_para(0x2b, 0xf9);
	mdelay(10);

	gpio_lcd_emuspi_write_one_para(0x1b, 0x14);
	gpio_lcd_emuspi_write_one_para(0x1a, 0x11);
	gpio_lcd_emuspi_write_one_para(0x1c, 0x0d);
	gpio_lcd_emuspi_write_one_para(0x1f, 0x42);
	mdelay(20);

	gpio_lcd_emuspi_write_one_para(0x19, 0x0a);
	gpio_lcd_emuspi_write_one_para(0x19, 0x1a);
	mdelay(40);

	gpio_lcd_emuspi_write_one_para(0x19, 0x12);
	mdelay(40);

	gpio_lcd_emuspi_write_one_para(0x1e, 0x2c);
	mdelay(100);

	
	gpio_lcd_emuspi_write_one_para(0x3c, 0x60);
	gpio_lcd_emuspi_write_one_para(0x3d, 0x1c);
	gpio_lcd_emuspi_write_one_para(0x34, 0x38);
	gpio_lcd_emuspi_write_one_para(0x35, 0x38);
	gpio_lcd_emuspi_write_one_para(0x24, 0x38);
	mdelay(40);

	gpio_lcd_emuspi_write_one_para(0x24, 0x3c);
	gpio_lcd_emuspi_write_one_para(0x16, 0x1c);
	gpio_lcd_emuspi_write_one_para(0x3a, 0xae);
	gpio_lcd_emuspi_write_one_para(0x01, 0x02);
	gpio_lcd_emuspi_write_one_para(0x55, 0x00);

}
#endif



#if 0
static void turn_off_backlight(void)
{
    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0);     
    udelay(800); 
}

static void decrease_lcd_backlight(void)
{
    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0);
    udelay(300);
    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,1);   
    udelay(5); 
}

static void disable_ctrl_lcd_backlight(void)
{
    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0); 
    udelay(800);
    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,1);         
    udelay(5); 
}
#endif

static __u32 last_bl_level = 6;  

static void lcdc_set_bl(struct msm_fb_data_type *mfd)
{
    
    __u32 new_bl_level = mfd->bl_level;
    __u32 cnt,diff;
    unsigned long flags;

    pr_info("[ZYF] lcdc_set_bl level=%d, %d\n", 
		   new_bl_level , mfd->panel_power_on);

    if(!mfd->panel_power_on)
    {
    	gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0);   
	    return;	
    }
		
    if(new_bl_level < 1)
    {
        new_bl_level = 0;
    }
    if (new_bl_level > 32)
    {
        new_bl_level = 32;
    }

    if(0 == new_bl_level)
    {
        	   gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0);    
    		   udelay(800); 
    } 
   else 
   {
          if(0 == last_bl_level)
          {
		  	gpio_direction_output(GPIO_LCD_BL_PWM_OUT,1);
			udelay(5); 
			last_bl_level = 16;
          }  
		  
	   if(new_bl_level == last_bl_level)	return;
		
	   if(new_bl_level > last_bl_level)   
	   {
		        diff = new_bl_level - last_bl_level;
		        for(cnt=0;cnt < diff;cnt++)
		        {  
		           spin_lock_irqsave(&tps61061_lock, flags);  
				   
			    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0);
			    udelay(10);
			    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,1);  
				
			    spin_unlock_irqrestore(&tps61061_lock, flags); 
				
			    udelay(3); 
		        }     	  
	   }
	   else  
	   {
		        diff = last_bl_level - new_bl_level;
		        for(cnt=0;cnt < diff;cnt++)
		        {  
		           spin_lock_irqsave(&tps61061_lock, flags); 
				   
			    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0);
			    udelay(200);
			    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,1);  
				
			    spin_unlock_irqrestore(&tps61061_lock, flags); 
				
			    udelay(3); 
		        }     
	   }
   }

    last_bl_level = new_bl_level;
	
}

static void spi_init(void)
{
	spi_sclk = *(lcdc_himax_pdata->gpio_num);
	spi_cs   = *(lcdc_himax_pdata->gpio_num + 1);
	spi_sdi  = *(lcdc_himax_pdata->gpio_num + 2);
	spi_sdo  = *(lcdc_himax_pdata->gpio_num + 3);
	himax_reset = *(lcdc_himax_pdata->gpio_num + 4);

	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_sdi, 0);

	gpio_set_value(spi_cs, 0);

}

static int lcdc_panel_on(struct platform_device *pdev)
{
	if(himax_panel_initialized==1) 
	{
		himax_panel_initialized = 0;
	}
	else 
	{
		spi_init();
		lcdc_himax_init();
		
	}
	return 0;
}

static int lcdc_panel_off(struct platform_device *pdev)
{
	
		lcdc_himax_sleep();
		gpio_direction_output(himax_reset, 0);
		mdelay(200);
		himax_panel_initialized = 0;
	return 0;
}




static void gpio_lcd_emuspi_write_one_para(unsigned char addr, unsigned char para)
{
	unsigned short  i;
	int j;

	i = addr | 0x7000;
	gpio_direction_output(spi_cs, 0);

	for (j = 0; j < 16; j++) {

		if ((i << j) & 0x8000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
	}
	gpio_direction_output(spi_cs, 1);

	i = para | 0x7200;
	gpio_direction_output(spi_cs, 0);
	for (j = 0; j < 16; j++) {

		if ((i << j) & 0x8000)
			gpio_direction_output(spi_sdo, 1);
		else
			gpio_direction_output(spi_sdo, 0);

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
	}
	gpio_direction_output(spi_cs, 1);

}

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	if(pdev->id == 0) {
		lcdc_himax_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = lcdc_panel_probe,
	.driver = {
		.name   = "lcdc_himax_wqvga",
	},
};

static struct msm_fb_panel_data lcdc_himax_panel_data = {
    .panel_info = {.bl_max = 32},
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};

static struct platform_device this_device = {
	.name   = "lcdc_himax_wqvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_himax_panel_data,
	}
};

static int __init lcdc_himax_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

	ret = platform_driver_register(&this_driver);
	if (ret) 
		return ret;

	pinfo = &lcdc_himax_panel_data.panel_info;
	pinfo->xres = 240;
	pinfo->yres = 400;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 8192000;

	pinfo->lcdc.h_back_porch = 4;
	pinfo->lcdc.h_front_porch = 4;
	pinfo->lcdc.h_pulse_width = 4;
	pinfo->lcdc.v_back_porch = 3;
	pinfo->lcdc.v_front_porch = 3;
	pinfo->lcdc.v_pulse_width = 1;
	pinfo->lcdc.border_clr = 0;
	pinfo->lcdc.underflow_clr = 0xff;	
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

	pr_info("himax panel loaded!\n");
	return ret;
}

module_init(lcdc_himax_panel_init);

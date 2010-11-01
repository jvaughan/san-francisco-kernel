


#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define GPIO_LCD_BL_SC_OUT 97

static int lcdc_samsung_regist = 1;
static boolean is_firsttime = true;

static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int lcd_panel_reset;

static __u32 last_bl_level = 16;

static DEFINE_SPINLOCK(tps61061_lock);

static struct msm_panel_common_pdata * lcd_panel_pdata;
static void lcd_panel_sleep(void);
static void lcd_panel_wakeup(void);
static void lcd_panel_init(void);
static void lcdc_set_bl(struct msm_fb_data_type *mfd);

static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);

static boolean bSleepWhenSuspend = false;

static void ILI9481_WriteReg(unsigned char SPI_COMMD)
{
	unsigned short SBit,SBuffer;
	unsigned char BitCounter;
	
	SBuffer=SPI_COMMD;
	gpio_direction_output(spi_cs, 0);	
	for(BitCounter=0;BitCounter<9;BitCounter++)
	{
		SBit = SBuffer&0x100;
		if(SBit)
			gpio_direction_output(spi_sdo, 1);
		else
			gpio_direction_output(spi_sdo, 0);
			
		gpio_direction_output(spi_sclk, 0);
		gpio_direction_output(spi_sclk, 1);
		SBuffer = SBuffer<<1;
	}
	gpio_direction_output(spi_cs, 1);
}

static void ILI9481_WriteData(unsigned char SPI_DATA)
{
	unsigned short SBit,SBuffer;
	unsigned char BitCounter;
	
	SBuffer=SPI_DATA | 0x100;
	gpio_direction_output(spi_cs, 0);
	
	for(BitCounter=0;BitCounter<9;BitCounter++)
	{
		SBit = SBuffer&0x100;
		if(SBit)
			gpio_direction_output(spi_sdo, 1);
		else
			gpio_direction_output(spi_sdo, 0);
			
		gpio_direction_output(spi_sclk, 0);
		gpio_direction_output(spi_sclk, 1);
		SBuffer = SBuffer<<1;
	}
	gpio_direction_output(spi_cs, 1);
}

static void lcd_panel_sleep(void)
{
	ILI9481_WriteReg(0x10);
}

static void lcd_panel_wakeup(void)
{
	ILI9481_WriteReg(0x11);
	ILI9481_WriteReg(0x29);
}

static void lcd_panel_init(void)
{
	mdelay(10);
	gpio_direction_output(lcd_panel_reset, 1);
	mdelay(1);
	gpio_direction_output(lcd_panel_reset, 0);
	mdelay(10);
	gpio_direction_output(lcd_panel_reset, 1);
	mdelay(100);

	mdelay(100);
	ILI9481_WriteReg(0x11);
	mdelay(20);//Delay(10*20);
	
		ILI9481_WriteReg(0xC6);
		ILI9481_WriteData(0x5B);
	
	ILI9481_WriteReg(0xD0);
	ILI9481_WriteData(0x07);
	ILI9481_WriteData(0x42);
	ILI9481_WriteData(0x18);
	ILI9481_WriteReg(0xD1);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x16);//07
	ILI9481_WriteData(0x0E);
	ILI9481_WriteReg(0xD2);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x02);
	ILI9481_WriteReg(0xC0);
	ILI9481_WriteData(0x10);
	ILI9481_WriteData(0x3B);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x02);
	ILI9481_WriteData(0x11);
	ILI9481_WriteReg(0xC5);
	ILI9481_WriteData(0x03);
	ILI9481_WriteReg(0xC8);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x32);
	ILI9481_WriteData(0x36);
	ILI9481_WriteData(0x45);
	ILI9481_WriteData(0x06);
	ILI9481_WriteData(0x16);
	ILI9481_WriteData(0x37);
	ILI9481_WriteData(0x75);
	ILI9481_WriteData(0x77);
	ILI9481_WriteData(0x54);
	ILI9481_WriteData(0x0C);
	ILI9481_WriteData(0x00);
	ILI9481_WriteReg(0x36);
	ILI9481_WriteData(0x0A);
	ILI9481_WriteReg(0x3A);
	ILI9481_WriteData(0x66);
	ILI9481_WriteReg(0x0C);
	ILI9481_WriteData(0x66);
	ILI9481_WriteReg(0x2A);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x3F);
	ILI9481_WriteReg(0x2B);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0xE0);
	mdelay(120);
	ILI9481_WriteReg(0xB4);
	ILI9481_WriteData(0x11);

	ILI9481_WriteReg(0x29);
	ILI9481_WriteReg(0x2C);
}

static int lcdc_panel_on(struct platform_device *pdev)
{
	if(!is_firsttime)
	{
		if(bSleepWhenSuspend)
		{
			lcd_panel_wakeup();
		}
		else
		{
			spi_init();
			lcd_panel_init();
		}
	}
	else
	{
		is_firsttime = false;
	}

	return 0;
}

static void lcdc_set_bl(struct msm_fb_data_type *mfd)
{
	__u32 new_bl_level = mfd->bl_level;
	__u32 cnt,diff;
	unsigned long flags;


	pr_info("[ZYF] lcdc_set_bl level=%d, %d\n", new_bl_level , mfd->panel_power_on);

	if(!mfd->panel_power_on)
	{
		gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);
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
		gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);    
		udelay(800); 
	} 
	else 
	{
		if(0 == last_bl_level)
		{
			gpio_direction_output(GPIO_LCD_BL_SC_OUT,1);
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
				gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);
				udelay(10);	 
				gpio_direction_output(GPIO_LCD_BL_SC_OUT,1);  
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
				gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);
				udelay(200);	    
				gpio_direction_output(GPIO_LCD_BL_SC_OUT,1);  
				spin_unlock_irqrestore(&tps61061_lock, flags);
				udelay(30);   
			}     
		}
	}

	last_bl_level = new_bl_level; 
}

static void spi_init(void)
{
	spi_sclk = *(lcd_panel_pdata->gpio_num);
	spi_cs   = *(lcd_panel_pdata->gpio_num + 1);
	spi_sdi  = *(lcd_panel_pdata->gpio_num + 2);
	spi_sdo  = *(lcd_panel_pdata->gpio_num + 3);
	lcd_panel_reset = *(lcd_panel_pdata->gpio_num + 4);

	gpio_set_value(spi_sclk, 1);
	gpio_set_value(spi_sdo, 1);
	gpio_set_value(spi_cs, 1);
}
static int lcdc_panel_off(struct platform_device *pdev)
{
	lcd_panel_sleep();

	if(!bSleepWhenSuspend)
	{
		gpio_direction_output(lcd_panel_reset, 0);
		gpio_direction_output(spi_sclk, 0);
		gpio_direction_output(spi_sdi, 0);
		gpio_direction_output(spi_sdo, 0);
		gpio_direction_output(spi_cs, 0);
	}

	return 0;
}

static struct msm_fb_panel_data lcd_lcdcpanel_panel_data = {
       .panel_info = {.bl_max = 32},
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcd_lcdcpanel_panel_data,
	}
};

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) {
		printk("use lead 320x480 panel driver!\n");
		
		lcd_panel_pdata = pdev->dev.platform_data;
		lcd_panel_pdata->panel_config_gpio(1);

		pinfo = &lcd_lcdcpanel_panel_data.panel_info;
		pinfo->xres = 320;
		pinfo->yres = 480;
		pinfo->type = LCDC_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 18;
		pinfo->fb_num = 2;

		pinfo->clk_rate = 10240000;
		
		pinfo->lcdc.h_back_porch = 3;
		pinfo->lcdc.h_front_porch = 3;
		pinfo->lcdc.h_pulse_width = 3;
		pinfo->lcdc.v_back_porch = 2;
		pinfo->lcdc.v_front_porch = 4;
		pinfo->lcdc.v_pulse_width = 2;
		pinfo->lcdc.border_clr = 0;	
		pinfo->lcdc.underflow_clr = 0xff;	
		pinfo->lcdc.hsync_skew = 3;

    		ret = platform_device_register(&this_device);
		
		return 0;
	}
	msm_fb_add_device(pdev);
	
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = lcdc_panel_probe,
	.driver = {
		.name   = "lcdc_panel_qvga",
	},
};



static int __init lcd_panel_panel_init(void)
{
	int ret;

	ret = platform_driver_register(&this_driver);

	if(lcdc_samsung_regist == 0)
	{
		printk("[lxw@lcd&fb]:unregist samsung driver!\n");
		platform_driver_unregister(&this_driver);
		return ret;
	}

	return ret;
}

module_init(lcd_panel_panel_init);





#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define GPIO_LCD_BL_SC_OUT 97
#define GPIO_LCD_BL_EN

#define LCD_BL_LEVEL 12
#define lcd_bl_max   lcd_bl_level-1
#define lcd_bl_min   0

static boolean is_firsttime = true;	
static boolean isOdd = true;				

static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int oled_reset;

static unsigned int array[12][42] = {	{0x40,0x05,0x41,0x3f,0x42,0x39,0x43,0x23,0x44,0x22,0x45,0x22,0x46,0x20,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x2c,0x54,0x21,0x55,0x22,0x56,0x21,0x60,0x05,0x61,0x3f,0x62,0x35,0x63,0x21,0x64,0x21,0x65,0x20,0x66,0x2b},
							{0x40,0x00,0x41,0x3f,0x42,0x10,0x43,0x2a,0x44,0x28,0x45,0x25,0x46,0x2d,0x50,0x00,0x51,0x3f,0x52,0x00,0x53,0x28,0x54,0x29,0x55,0x26,0x56,0x2b,0x60,0x00,0x61,0x3f,0x62,0x1e,0x63,0x2a,0x64,0x27,0x65,0x22,0x66,0x3d},
							{0x40,0x05,0x41,0x3f,0x42,0x25,0x43,0x20,0x44,0x22,0x45,0x1e,0x46,0x2f,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x21,0x55,0x1f,0x56,0x2f,0x60,0x05,0x61,0x3f,0x62,0x22,0x63,0x1f,0x64,0x1f,0x65,0x1c,0x66,0x3e},
							{0x40,0x05,0x41,0x3f,0x42,0x22,0x43,0x20,0x44,0x21,0x45,0x1e,0x46,0x32,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1f,0x54,0x20,0x55,0x1e,0x56,0x33,0x60,0x05,0x61,0x3f,0x62,0x1f,0x63,0x1f,0x64,0x1f,0x65,0x1b,0x66,0x42},
							{0x40,0x05,0x41,0x3f,0x42,0x22,0x43,0x1f,0x44,0x21,0x45,0x1d,0x46,0x35,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1f,0x54,0x20,0x55,0x1d,0x56,0x36,0x60,0x05,0x61,0x3f,0x62,0x1d,0x63,0x1e,0x64,0x1f,0x65,0x1a,0x66,0x46},
							{0x40,0x05,0x41,0x3f,0x42,0x1a,0x43,0x20,0x44,0x20,0x45,0x1c,0x46,0x38,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x20,0x55,0x1c,0x56,0x39,0x60,0x05,0x61,0x3f,0x62,0x19,0x63,0x1e,0x64,0x1e,0x65,0x19,0x66,0x4a},
							{0x40,0x05,0x41,0x3f,0x42,0x1b,0x43,0x1e,0x44,0x21,0x45,0x1c,0x46,0x3a,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x20,0x55,0x1c,0x56,0x3b,0x60,0x05,0x61,0x3f,0x62,0x17,0x63,0x1d,0x64,0x1f,0x65,0x19,0x66,0x4c},
							{0x40,0x05,0x41,0x3f,0x42,0x1d,0x43,0x1e,0x44,0x20,0x45,0x1b,0x46,0x3d,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1f,0x54,0x1f,0x55,0x1b,0x56,0x3e,0x60,0x05,0x61,0x3f,0x62,0x18,0x63,0x1d,0x64,0x1e,0x65,0x18,0x66,0x50},
							{0x40,0x05,0x41,0x3f,0x42,0x1a,0x43,0x1e,0x44,0x1f,0x45,0x1b,0x46,0x3f,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x1f,0x55,0x1b,0x56,0x40,0x60,0x05,0x61,0x3f,0x62,0x14,0x63,0x1d,0x64,0x1d,0x65,0x18,0x66,0x52},
							{0x40,0x05,0x41,0x3f,0x42,0x15,0x43,0x1e,0x44,0x20,0x45,0x1a,0x46,0x41,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x20,0x55,0x1a,0x56,0x42,0x60,0x05,0x61,0x3f,0x62,0x13,0x63,0x1c,0x64,0x1f,0x65,0x17,0x66,0x54},
							{0x40,0x05,0x41,0x3f,0x42,0x15,0x43,0x1e,0x44,0x1f,0x45,0x1a,0x46,0x43,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x1f,0x55,0x1a,0x56,0x44,0x60,0x05,0x61,0x3f,0x62,0x0f,0x63,0x1d,0x64,0x1d,0x65,0x17,0x66,0x56},
							{0x40,0x05,0x41,0x3f,0x42,0x16,0x43,0x1d,0x44,0x1f,0x45,0x19,0x46,0x46,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x1f,0x55,0x19,0x56,0x46,0x60,0x05,0x61,0x3f,0x62,0x10,0x63,0x1c,0x64,0x1d,0x65,0x16,0x66,0x5b}};

static struct msm_panel_common_pdata * lcdc_oled_pdata;
static void gpio_lcd_emuspi_write_one_index(unsigned short addr);
static void gpio_lcd_emuspi_write_one_data(unsigned short data);
static void gpio_lcd_emuspi_write_more(unsigned int num,unsigned int level);

static void lcdc_oled_sleep(void);
static void lcdc_oled_init(void);
static void lcdc_set_bl(struct msm_fb_data_type *mfd);

static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);

static int lcdc_panel_on(struct platform_device *pdev)
{

	spi_init();
	if(!is_firsttime)
	{
		lcdc_oled_init();
		
	}
	else
	{
		is_firsttime = false;
	}
	return 0;
}

static void lcdc_oled_sleep(void)
{
	gpio_lcd_emuspi_write_one_index(0x14);
	gpio_lcd_emuspi_write_one_data(0x00);

	gpio_lcd_emuspi_write_one_index(0x1D);
	gpio_lcd_emuspi_write_one_data(0xA1);
	
	mdelay(20);

}

static void lcdc_oled_init(void)
{
	gpio_direction_output(oled_reset, 1);
	mdelay(10);
	gpio_direction_output(oled_reset, 0);
	mdelay(20);
	gpio_direction_output(oled_reset, 1);
	mdelay(10);

	gpio_lcd_emuspi_write_one_index(0x31);
	gpio_lcd_emuspi_write_one_data(0x08);
	
	gpio_lcd_emuspi_write_one_index(0x32);
	gpio_lcd_emuspi_write_one_data(0x14);

	gpio_lcd_emuspi_write_one_index(0x30);
	gpio_lcd_emuspi_write_one_data(0x02);

	gpio_lcd_emuspi_write_one_index(0x27);
	gpio_lcd_emuspi_write_one_data(0x01);

	gpio_lcd_emuspi_write_one_index(0x12);
	gpio_lcd_emuspi_write_one_data(0x08);

	gpio_lcd_emuspi_write_one_index(0x13);
	gpio_lcd_emuspi_write_one_data(0x08);

	gpio_lcd_emuspi_write_one_index(0x15);
	gpio_lcd_emuspi_write_one_data(0x0f);

	gpio_lcd_emuspi_write_one_index(0x16);
	gpio_lcd_emuspi_write_one_data(0x00);

	gpio_lcd_emuspi_write_one_index(0xef);
	gpio_lcd_emuspi_write_one_data(0xd0);
	gpio_lcd_emuspi_write_one_data(0xe8);

	gpio_lcd_emuspi_write_one_index(0x39);
	gpio_lcd_emuspi_write_one_data(0x44);

	gpio_lcd_emuspi_write_one_index(0x40);
	gpio_lcd_emuspi_write_one_data(0x05);

	gpio_lcd_emuspi_write_one_index(0x41);
	gpio_lcd_emuspi_write_one_data(0x3F);

	gpio_lcd_emuspi_write_one_index(0x42);
	gpio_lcd_emuspi_write_one_data(0x16);

	gpio_lcd_emuspi_write_one_index(0x43);
	gpio_lcd_emuspi_write_one_data(0x1D);

	gpio_lcd_emuspi_write_one_index(0x44);
	gpio_lcd_emuspi_write_one_data(0x1F);

	gpio_lcd_emuspi_write_one_index(0x45);
	gpio_lcd_emuspi_write_one_data(0x19);

	gpio_lcd_emuspi_write_one_index(0x46);
	gpio_lcd_emuspi_write_one_data(0x46);

	gpio_lcd_emuspi_write_one_index(0x50);
	gpio_lcd_emuspi_write_one_data(0x05);

	gpio_lcd_emuspi_write_one_index(0x51);
	gpio_lcd_emuspi_write_one_data(0x00);

	gpio_lcd_emuspi_write_one_index(0x52);
	gpio_lcd_emuspi_write_one_data(0x00);

	gpio_lcd_emuspi_write_one_index(0x53);
	gpio_lcd_emuspi_write_one_data(0x1E);

	gpio_lcd_emuspi_write_one_index(0x54);
	gpio_lcd_emuspi_write_one_data(0x1F);

	gpio_lcd_emuspi_write_one_index(0x55);
	gpio_lcd_emuspi_write_one_data(0x19);

	gpio_lcd_emuspi_write_one_index(0x56);
	gpio_lcd_emuspi_write_one_data(0x46);

	gpio_lcd_emuspi_write_one_index(0x60);
	gpio_lcd_emuspi_write_one_data(0x05);

	gpio_lcd_emuspi_write_one_index(0x61);
	gpio_lcd_emuspi_write_one_data(0x3F);

	gpio_lcd_emuspi_write_one_index(0x62);
	gpio_lcd_emuspi_write_one_data(0x10);

	gpio_lcd_emuspi_write_one_index(0x63);
	gpio_lcd_emuspi_write_one_data(0x1C);

	gpio_lcd_emuspi_write_one_index(0x64);
	gpio_lcd_emuspi_write_one_data(0x1D);

	gpio_lcd_emuspi_write_one_index(0x65);
	gpio_lcd_emuspi_write_one_data(0x16);

	gpio_lcd_emuspi_write_one_index(0x66);
	gpio_lcd_emuspi_write_one_data(0x5B);

	gpio_lcd_emuspi_write_one_index(0x17);
	gpio_lcd_emuspi_write_one_data(0x22);

	gpio_lcd_emuspi_write_one_index(0x18);
	gpio_lcd_emuspi_write_one_data(0x33);

	gpio_lcd_emuspi_write_one_index(0x19);
	gpio_lcd_emuspi_write_one_data(0x03);

	gpio_lcd_emuspi_write_one_index(0x1a);
	gpio_lcd_emuspi_write_one_data(0x01);

	gpio_lcd_emuspi_write_one_index(0x22);
	gpio_lcd_emuspi_write_one_data(0xa4);

	gpio_lcd_emuspi_write_one_index(0x23);
	gpio_lcd_emuspi_write_one_data(0x00);

	gpio_lcd_emuspi_write_one_index(0x26);
	gpio_lcd_emuspi_write_one_data(0xa0);

	gpio_lcd_emuspi_write_one_index(0x1d);
	gpio_lcd_emuspi_write_one_data(0xa0);
	mdelay(200);
	
	gpio_lcd_emuspi_write_one_index(0x14);
	gpio_lcd_emuspi_write_one_data(0x03);


	printk("lcd module oled init exit\n!");

}


static void lcdc_set_bl(struct msm_fb_data_type *mfd)
{

	int current_lel = mfd->bl_level;
	unsigned int num = (sizeof(array[current_lel-1])/sizeof(array[current_lel-1][0]))/2;
	pr_info( "[ZYF] lcdc_set_bl level=%d, %d\n", current_lel , mfd->panel_power_on);
	
    if(!mfd->panel_power_on)			
  	  return;

	if(current_lel < 1)
	{
		current_lel = 1;
	}
	if(current_lel > LCD_BL_LEVEL)
	{
		current_lel = LCD_BL_LEVEL;
	}
	if(isOdd)			
	{
		gpio_lcd_emuspi_write_one_index(0x39);
		gpio_lcd_emuspi_write_one_data(0x43);

	}
	else
	{
		gpio_lcd_emuspi_write_one_index(0x39);
		gpio_lcd_emuspi_write_one_data(0x34);

	}
	gpio_lcd_emuspi_write_more(num,current_lel);
	if(isOdd)				
	{
		gpio_lcd_emuspi_write_one_index(0x39);
		gpio_lcd_emuspi_write_one_data(0x34);
		isOdd = false;
	}
	else
	{
		gpio_lcd_emuspi_write_one_index(0x39);
		gpio_lcd_emuspi_write_one_data(0x43);
		isOdd = true;
	}
	

#if 0
    int current_lel = mfd->bl_level;
    uint8_t cnt = 0;
    unsigned long flags;


    pr_info(HP_TAG "[ZYF] lcdc_set_bl level=%d, %d\n", 
		   current_lel , mfd->panel_power_on);


    if(!mfd->panel_power_on)
	{
    	    gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);		
	    return;
    	}

    if(current_lel < 1)
    {
        current_lel = 0;
    }
    if(current_lel > 32)
    {
        current_lel = 32;
    }

    local_irq_save(flags);
    if(current_lel==0)
	    gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
    else {
	    for(cnt = 0;cnt < 33-current_lel;cnt++) 
	    { 
		    gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
		    udelay(20);
		    gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
		    udelay(5);
	    }     
	    gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	    mdelay(8);

    }
    local_irq_restore(flags);
#endif	
}

static void spi_init(void)
{
	spi_sclk = *(lcdc_oled_pdata->gpio_num);
	spi_cs   = *(lcdc_oled_pdata->gpio_num + 1);
	spi_sdi  = *(lcdc_oled_pdata->gpio_num + 2);
	spi_sdo  = *(lcdc_oled_pdata->gpio_num + 3);
	oled_reset = *(lcdc_oled_pdata->gpio_num + 4);
	printk("spi_init\n!");

	gpio_set_value(spi_sclk, 1);
	gpio_set_value(spi_sdo, 1);
	gpio_set_value(spi_cs, 1);

}
static int lcdc_panel_off(struct platform_device *pdev)
{

	lcdc_oled_sleep();

	gpio_direction_output(oled_reset, 0);

	return 0;
}


static void gpio_lcd_emuspi_write_one_index(unsigned short addr)
{
	unsigned int i;
	int j;

	i = addr | 0x7000;
	gpio_direction_output(spi_cs, 0);

	for (j = 0; j < 16; j++) {

		if (i & 0x8000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}

	gpio_direction_output(spi_cs, 1);
}

static void gpio_lcd_emuspi_write_one_data(unsigned short data)
{
	unsigned int i;
	int j;

	i = data | 0x7200;
	gpio_direction_output(spi_cs, 0);

	for (j = 0; j < 16; j++) {

		if (i & 0x8000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}

	gpio_direction_output(spi_cs, 1);
}

static void gpio_lcd_emuspi_write_more(unsigned int num,unsigned int level)
{
	unsigned int i;
	for(i = 0; i < num;i++)
	{
		gpio_lcd_emuspi_write_one_index(array[level-1][2*i]);
		gpio_lcd_emuspi_write_one_data(array[level-1][2*i+1]);
	}
}
static struct msm_fb_panel_data lcdc_oled_panel_data = {
       .panel_info = {.bl_max = 12},
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_oled_panel_data,
	}
};

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) {
		lcdc_oled_pdata = pdev->dev.platform_data;
		lcdc_oled_pdata->panel_config_gpio(1);

		pinfo = &lcdc_oled_panel_data.panel_info;
		pinfo->xres = 480;
		pinfo->yres = 800;
		pinfo->type = LCDC_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 18;
		pinfo->fb_num = 2;

		pinfo->clk_rate = 24576000;
		
		pinfo->lcdc.h_back_porch = 8;
		pinfo->lcdc.h_front_porch = 8;
		pinfo->lcdc.h_pulse_width = 1;
		pinfo->lcdc.v_back_porch = 8;
		pinfo->lcdc.v_front_porch = 8;
		pinfo->lcdc.v_pulse_width = 1;
		pinfo->lcdc.border_clr = 0;	
		pinfo->lcdc.underflow_clr = 0xffff;	
		pinfo->lcdc.hsync_skew = 0;

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



static int __init lcdc_oled_panel_init(void)
{
	int ret;

	ret = platform_driver_register(&this_driver);

	return ret;
}

module_init(lcdc_oled_panel_init);


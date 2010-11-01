


#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <asm/mach-types.h>


static unsigned int keypad_row_gpios[] = {28, 33, 34}; 

static unsigned int keypad_col_gpios[] = {41, 40, 37, 36}; 


#define KEYMAP_INDEX(row, col) ((row)*ARRAY_SIZE(keypad_col_gpios) + (col))
static const unsigned short keypad_keymap_joe[ARRAY_SIZE(keypad_col_gpios)
                                              * ARRAY_SIZE(keypad_row_gpios)] = {
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEDOWN,     
	[KEYMAP_INDEX(0, 1)] = KEY_CAMERA,    /
	[KEYMAP_INDEX(0, 2)] = KEY_VOLUMEUP,    
	[KEYMAP_INDEX(0, 3)] = KEY_OK,      
	[KEYMAP_INDEX(1, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(1, 1)] = KEY_HOME,    
	[KEYMAP_INDEX(1, 2)] = KEY_MENU,   
	[KEYMAP_INDEX(1, 3)] = KEY_BACK, 
	[KEYMAP_INDEX(2, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(2, 1)] = KEY_SEARCH,    
	[KEYMAP_INDEX(2, 2)] = KEY_RESERVED,  
	[KEYMAP_INDEX(2, 3)] = KEY_RESERVED, 
};
static struct gpio_event_matrix_info joe_keypad_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		    = keypad_keymap_joe,
	.output_gpios	= keypad_row_gpios,
	.input_gpios	= keypad_col_gpios,
	.noutputs	    = ARRAY_SIZE(keypad_row_gpios),
	.ninputs	    = ARRAY_SIZE(keypad_col_gpios),
	.settle_time.tv.nsec    = 0,
	.poll_time.tv.nsec      = 20 * NSEC_PER_MSEC,
#if 0 
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE | GPIOKPF_PRINT_UNMAPPED_KEYS
#else
	.flags          = GPIOKPF_LEVEL_TRIGGERED_IRQ
	                    | GPIOKPF_DRIVE_INACTIVE
	                    | GPIOKPF_ACTIVE_HIGH
	                    | GPIOKPF_PRINT_UNMAPPED_KEYS
#endif
};

static struct gpio_event_info *joe_keypad_info[] = {
	&joe_keypad_matrix_info.info
};

static struct gpio_event_platform_data joe_keypad_data = {
	.name		= "joe_keypad",
	.info		= joe_keypad_info,
	.info_count	= ARRAY_SIZE(joe_keypad_info)
};

struct platform_device keypad_device_joe = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev	= {
		.platform_data	= &joe_keypad_data,
	},
};
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
extern int zte_get_ftm_flag(void);
#endif
static int __init joe_init_keypad(void)
{
	#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	int ftm_flag;
	ftm_flag = zte_get_ftm_flag();
	if (1 ==ftm_flag)return 0;
	#endif
	joe_keypad_matrix_info.keymap = keypad_keymap_joe;
	return platform_device_register(&keypad_device_joe);
}

device_initcall(joe_init_keypad);


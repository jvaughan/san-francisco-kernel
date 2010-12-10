

#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <asm/mach-types.h>

static unsigned int keypad_row_gpios[] = {28, 31, 33}; // {31, 32, 33, 34, 35, 41}

static unsigned int keypad_col_gpios[] = {37, 41}; // { 36, 37, 38, 39, 40 }

#define KEYMAP_INDEX(row, col) ((row)*ARRAY_SIZE(keypad_col_gpios) + (col))


static const unsigned short keypad_keymap_raise[ARRAY_SIZE(keypad_col_gpios) *
					      ARRAY_SIZE(keypad_row_gpios)] = {
	/*                       row, col   */
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEUP, //upper
	[KEYMAP_INDEX(0, 1)] = KEY_VOLUMEDOWN, //lower

	[KEYMAP_INDEX(1, 0)] = KEY_REPLY, //middle key
	[KEYMAP_INDEX(1, 1)] = KEY_SEND,   //left key

	[KEYMAP_INDEX(2, 0)] = KEY_END,   //right key
	[KEYMAP_INDEX(2, 1)] = KEY_CAMERA, //undefined
};

static const unsigned short rasise_keypad_virtual_keys[] = {
	KEY_END,
	KEY_POWER
};

static struct gpio_event_matrix_info raise_keypad_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keypad_keymap_raise,
	.output_gpios	= keypad_row_gpios,
	.input_gpios	= keypad_col_gpios,
	.noutputs	= ARRAY_SIZE(keypad_row_gpios),
	.ninputs	= ARRAY_SIZE(keypad_col_gpios),
	.settle_time.tv.nsec = 0,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
#if 0 
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS
#else
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE | GPIOKPF_ACTIVE_HIGH | GPIOKPF_PRINT_UNMAPPED_KEYS /*| GPIOKPF_PRINT_MAPPED_KEYS*/
#endif
};

static struct gpio_event_info *raise_keypad_info[] = {
	&raise_keypad_matrix_info.info
};

static struct gpio_event_platform_data raise_keypad_data = {
	.name		= "raise_keypad",
	.info		= raise_keypad_info,
	.info_count	= ARRAY_SIZE(raise_keypad_info)
};

struct platform_device keypad_device_raise = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &raise_keypad_data,
	},
};
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
extern int zte_get_ftm_flag(void);
#endif
static int __init raise_init_keypad(void)
{
	#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	int ftm_flag;
	ftm_flag = zte_get_ftm_flag();
	if (1 ==ftm_flag)return 0;
	#endif
	raise_keypad_matrix_info.keymap = keypad_keymap_raise;
	return platform_device_register(&keypad_device_raise);
}

device_initcall(raise_init_keypad);


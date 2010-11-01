
#include <mach/msm_hsusb.h>
#include <linux/usb/mass_storage_function.h>

#ifdef CONFIG_USB_FUNCTION
struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
	.nluns          = 0x01,            
	.buf_size       = 16384,
	.vendor         = "ZTE",
	.product        = "Mass Storage",
	.release        = 0xffff,
};

struct usb_function_map usb_functions_map[] = {
	{"diag", 0},
	{"modem", 1},
	{"nmea", 2},
	{"mass_storage", 3},
	{"adb", 4},
        //{"ethernet", 5},
};

struct usb_composition usb_func_composition[] = {
#if defined(CONFIG_ZTE_PLATFORM)	
	{
        .vendor_id      = 0x19D2,
		.product_id     = 0x0112,
		.functions	    = 0x01, /* 000001 */
	},

	{
        .vendor_id      = 0x19D2,
		.product_id     = 0x0111,
		.functions	    = 0x07, /* 000111 */
	},
	
	{
        .vendor_id      = 0x19d2,
		.product_id     = 0x1355, 
		.functions	    = 0x0A, /* 001010 */
	},

	{
        .vendor_id      = 0x19d2,
		.product_id     = 0x1354, 
		.functions	    = 0x1A, /* 011010 */
	},

	{
        .vendor_id      = 0x19D2,
		.product_id     = 0x1353,
		.functions	    = 0x08, /* 001000 */
	},

	{
        .vendor_id      = 0x19d2,
		.product_id     = 0x1352,
		.functions	    = 0x10, /* 010000 */
	},

	{
        .vendor_id      = 0x19d2,
		.product_id     = 0x1351,
		.functions	    = 0x18, /* 011000 */
	},

	{
        .vendor_id      = 0x19d2,
		.product_id     = 0x1350,
		.functions	    = 0x1F, /* 011111 */
	},
#else
	{
        .vendor_id      = 0x19D2,
		.product_id     = 0x0112,
		.functions	    = 0x01, /* 000001 */
	},

	{
        .vendor_id      = 0x19D2,
		.product_id     = 0x0111,
		.functions	    = 0x07, /* 000111 */
	},

	{
        .vendor_id      = 0x045E,
		.product_id     = 0xFFFF,
		.functions	    = 0x08, /* 001000 */
	},

	{
        .vendor_id      = 0x18d1,
		.product_id     = 0xCFFE,
		.functions	    = 0x10, /* 010000 */
	},

	{
        .vendor_id      = 0x18d1,
		.product_id     = 0xD006,
		.functions	    = 0x18, /* 011000 */
	},

	{
        .vendor_id      = 0x18d1,
		.product_id     = 0xD00D,
		.functions	    = 0x1F, /* 011111 */
	},
#endif	
};
#endif

struct msm_hsusb_platform_data msm_hsusb_pdata = {
#ifdef CONFIG_USB_FUNCTION
	.version	= 0x0100,
	.phy_info	= (USB_PHY_INTEGRATED | USB_PHY_MODEL_65NM),
	.product_name       = "ZTE HSUSB Device",
	.manufacturer_name  = "ZTE Incorporated",
#if defined (CONFIG_ZTE_PLATFORM)
	.vendor_id          = 0x19d2,
	#if defined CONFIG_MACH_MOONCAKE
	.serial_number      = "P726N",
	#else
	.serial_number      = "ZTE-HSUSB",
	#endif	
#else	
	.vendor_id          = 0x18d1,
	.serial_number      = "1234567890ABCDEF",
#endif
	.compositions	= usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
	.function_map   = usb_functions_map,
	.num_functions	= ARRAY_SIZE(usb_functions_map),
	.config_gpio    = NULL,
#endif
};

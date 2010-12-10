
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/gpio.h>
#include <linux/clk.h>
#include <mach/avago_ofn.h>
#include <linux/input.h>
#include <linux/timer.h>

#include <linux/earlysuspend.h>


MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.2");
MODULE_AUTHOR("Neil Leeder QUALCOMM-QCT");
MODULE_DESCRIPTION("QUALCOMM Avago OFN driver");

struct avago_ofn_data {
       struct input_dev  *ofn_input_dev;
	int               delta_x, delta_y;
	int gpio_irq;
	int gpio_shutdown;
	int gpio_reset;
	struct i2c_client *clientp;
	struct early_suspend early_suspend;
};

static struct avago_ofn_data *ofn_data;

static int rocker_delta_x = 0;
static int rocker_delta_y = 0;

#ifdef USE_XYQ_MODE
#define Step_Thresh_sum  (3)
#else
#define Step_Thresh_sum  (20)
#define Step_Thresh_X  (1) 
#define Step_Thresh_Y  (1)
#endif



static void        ofn_work_f(struct work_struct *work);
static irqreturn_t ofn_interrupt(int irq, void *dev_id);
static int avago_ofn_probe(struct i2c_client *client, const struct i2c_device_id *id);

#if defined(CONFIG_MACH_RAISE)
extern struct input_dev *raise_keypad_get_input_dev(void);
#elif defined(CONFIG_MACH_MOONCAKE)
extern struct input_dev *mooncake_keypad_get_input_dev(void);
#endif

#define QUALCOMM_VENDORID              0x5413

#define GPIO_OFN_RESET  34
#define GPIO_OFN_SHUTDOWN  36
#define GPIO_OFN_MOTION  35
#define GPIO_OFN_MOTION_INT  MSM_GPIO_TO_INT(35)

#define AVAGO_OFN_I2C_ADDR  (0x33 << 1)

#define OFN_REG_PRODUCT_ID            0x00
#define OFN_REG_MOTION            0x02
#define OFN_REG_DELTA_X            0x03
#define OFN_REG_DELTA_Y            0x04
#define OFN_REG_AI_BIT           0x80


static	DECLARE_WORK(ofn_work, ofn_work_f);


#if 0
static int32_t ofn_i2c_read(uint8_t saddr, uint16_t reg, uint8_t *rbdata, uint8_t len_n)
{
    int32_t rc;

    rc = msm_rpc_i2c_read_b(saddr, reg, rbdata, len_n);

    return rc;
}

static int ofn_i2c_write(uint8_t saddr, uint16_t reg, uint8_t wbdata)
{
    int32_t rc;

    rc = msm_rpc_i2c_write_b(saddr, reg, wbdata);

    return rc;
}

static int ofn_config(void)
{
	uint8_t            buf[2];
	int           rc;
    uint8_t delta_xy_buf[2] = {23, 23}; 
    uint8_t motion = 23;

    printk("chenjun: ofn_config: isp = %d\n", gpio_get_value(32));

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x3a, 0x5A);
    mdelay(23); 

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x60, 0xE4);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x62, 0x12);
    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x63, 0x0E);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x64, 0x08);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x65, 0x06);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x66, 0x40);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x67, 0x08);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x68, 0x48);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x69, 0x0a);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x6a, 0x50);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x6b, 0x48);


    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x6e, 0x34);

// write 0x6f with 0x3c
    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x6f, 0x3c);

// write 0x70 with 0x18
    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x70, 0x18);

// write 0x71 with 0x20
    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x71, 0x20);


    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x75, 0x50);


    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x73, 0x99);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x74, 0x02);


    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x1C, 0x10);

        rc = ofn_i2c_read(AVAGO_OFN_I2C_ADDR, 0x02, &motion, 1);
        rc = ofn_i2c_read(AVAGO_OFN_I2C_ADDR, OFN_REG_DELTA_X, &delta_xy_buf[0], 1);
        rc = ofn_i2c_read(AVAGO_OFN_I2C_ADDR, OFN_REG_DELTA_Y, &delta_xy_buf[1], 1);

    ofn_i2c_write(AVAGO_OFN_I2C_ADDR, 0x1a, 0x08); 
    rc = ofn_i2c_read(AVAGO_OFN_I2C_ADDR, OFN_REG_PRODUCT_ID, buf, 1);
    if (rc)
	goto ofn_conf_err_exit;

    printk("chenjun: ofn_config: PRODUCT_ID = %x\n", buf[0]); 
    printk("chenjun: ofn_config: motion = %d\n", motion);
    printk("chenjun: ofn_config: delta_x = %d, delta_y = %d\n", delta_xy_buf[0], delta_xy_buf[1]);

    return 0;

ofn_conf_err_exit:
	return -1;
}

static void ofn_work_f(struct work_struct *work)
{
    uint8_t delta_xy_buf[2] = {23, 23}; 
    uint8_t motion = 23;
    int           rc;

    printk("chenjun: ofn_work_f: isp = %d\n", gpio_get_value(32));
	printk("chenjun: ofn_work_f: SHUTDOWN = %d\n", gpio_get_value(GPIO_OFN_SHUTDOWN));
	printk("chenjun: ofn_work_f: RESET = %d\n", gpio_get_value(GPIO_OFN_RESET));


#if 0
    rc = ofn_i2c_read(AVAGO_OFN_I2C_ADDR, (OFN_REG_AI_BIT |OFN_REG_DELTA_X), delta_xy_buf, 2);
#else
        rc = ofn_i2c_read(AVAGO_OFN_I2C_ADDR, 0x02, &motion, 1);
        rc = ofn_i2c_read(AVAGO_OFN_I2C_ADDR, OFN_REG_DELTA_X, &delta_xy_buf[0], 1);
        rc = ofn_i2c_read(AVAGO_OFN_I2C_ADDR, OFN_REG_DELTA_Y, &delta_xy_buf[1], 1);
#endif



    if (rc)
	goto ofn_work_err_exit;

    printk("chenjun: ofn_work_f: motion = %d\n", motion);
    printk("chenjun: ofn_work_f: delta_x = %d, delta_y = %d\n", delta_xy_buf[0], delta_xy_buf[1]);

ofn_work_err_exit:
	return;
}

static irqreturn_t ofn_interrupt(int irq, void *dev_id)
{
	schedule_work(&ofn_work);
	return IRQ_HANDLED;
}

static int avago_ofn_probe(struct platform_device *pdev)
{
	int rc;
	struct vreg *vreg_ofn;

#if 0
	struct clk *clk;
	clk = clk_get(NULL, "i2c_clk");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Could not get clock\n");
		rc = PTR_ERR(clk);
		goto fail_ofn_config;
	}
	clk_enable(clk);
#endif

	printk("chenjun: avago_ofn_probe: SHUTDOWN = %d\n", gpio_get_value(GPIO_OFN_SHUTDOWN));
	printk("chenjun: avago_ofn_probe: RESET = %d\n", gpio_get_value(GPIO_OFN_RESET));
	printk("chenjun: avago_ofn_probe: SHUTDOWN = %d\n", gpio_get_value(GPIO_OFN_SHUTDOWN));
	gpio_direction_output(GPIO_OFN_SHUTDOWN, 1);
	gpio_direction_output(GPIO_OFN_RESET, 1);
	printk("chenjun: avago_ofn_probe: SHUTDOWN = %d\n", gpio_get_value(GPIO_OFN_SHUTDOWN));
	printk("chenjun: avago_ofn_probe: RESET = %d\n", gpio_get_value(GPIO_OFN_RESET));
       mdelay(503); 


#if 1
	vreg_ofn = vreg_get(NULL, "msmp");
	if (IS_ERR(vreg_ofn)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
				__func__, PTR_ERR(vreg_ofn));
		return -EIO;
	}

	rc = vreg_set_level(vreg_ofn, 2800);
	if (rc) {
		printk(KERN_ERR "%s: vreg set level failed (%d)\n",
			       __func__, rc);
		return -EIO;
	}
	rc = vreg_enable(vreg_ofn);
	if (rc) {
		printk(KERN_ERR "%s: vreg enable failed (%d)\n",
			       __func__, rc);
		return -EIO;
	}

       mdelay(1000);
#endif
	gpio_direction_output(32, 0); 
	printk("chenjun: avago_ofn_probe: isp = %d\n", gpio_get_value(32));


	rc = gpio_direction_input(GPIO_OFN_MOTION);
	if (rc) {
        printk("avago_ofn_probe FAILED: gpio_direction_input(%d), rc=%d\n", GPIO_OFN_MOTION, rc);
		goto fail_irq;
	}
	rc = request_irq(GPIO_OFN_MOTION_INT, &ofn_interrupt,
		                   IRQF_TRIGGER_LOW, "ofn", 0); 
	if (rc) {
		printk("avago_ofn_probe FAILED: request_irq rc=%d\n", rc);
		goto fail_irq;
	}
#if 1
	gpio_direction_output(GPIO_OFN_SHUTDOWN, 0);
	gpio_direction_output(GPIO_OFN_RESET, 0);
       udelay(29); // Minimum: 20us
	gpio_direction_output(GPIO_OFN_RESET, 1);
	printk("avago_ofn_probe: SHUTDOWN = %d\n", gpio_get_value(GPIO_OFN_SHUTDOWN));
	printk("avago_ofn_probe: RESET = %d\n", gpio_get_value(GPIO_OFN_RESET));

       mdelay(103); // Minimum: 100ms
#endif
	rc = ofn_config();
	if (rc) {
		printk("avago_ofn_probe FAILED: ofn_config: rc=%d\n", rc);
		goto fail_ofn_config;
	}
	return 0;

fail_ofn_config:
fail_irq:
	return rc;
}

static int avago_ofn_remove(struct platform_device *pdev)
{
    printk("avago_ofn_remove\n");
    free_irq(GPIO_OFN_MOTION_INT, NULL);
    gpio_direction_output(GPIO_OFN_SHUTDOWN, 1);

    return 0;
}

static struct platform_driver avago_ofn_driver = {
	.probe = avago_ofn_probe,
	.remove = avago_ofn_remove,
	.driver = {
		.name = "avago_ofn",
		.owner = THIS_MODULE,
	},
};

static int __init ofn_init(void)
{
	return platform_driver_register(&avago_ofn_driver);
}

static void __exit ofn_exit(void)
{
	platform_driver_unregister(&avago_ofn_driver);
}

module_init(ofn_init);
module_exit(ofn_exit);

#elif 0 

static int ofn_i2c_read(struct i2c_client *client,
			     int reg, u8 *buf, int count)
{
	int rc;
	int ret = 0;

       if  (1 == count)
       {
	    buf[0] = reg;
       }
	else if (count >= 2)
	{
	    buf[0] = OFN_REG_AI_BIT | reg;
	}
	rc = i2c_master_send(client, buf, 1);
	if (rc != 1) {
		dev_err(&client->dev,
		       "ofn_i2c_read FAILED: read of register %d\n", reg);
		ret = -1;
		goto ofn_i2c_rd_exit;
	}
	rc = i2c_master_recv(client, buf, count);
	if (rc != count) {
		dev_err(&client->dev,
		       "ofn_i2c_read FAILED: read %d bytes from reg %d\n",
		       count, reg);
		ret = -1;
	}

ofn_i2c_rd_exit:
	return ret;
}

static int ofn_i2c_write(struct i2c_client *client, int reg, u8 data)
{
	u8            buf[2];
	int           rc;
	int           ret = 0;

	buf[0] = reg;
	buf[1] = data;
	rc = i2c_master_send(client, buf, 2);
	if (rc != 2) {
		dev_err(&client->dev,
		       "ofn_i2c_write FAILED: writing to reg %d\n", reg);
		ret = -1;
	}
	return ret;
}

static void ofn_next_key_timer_callback(unsigned long data)
{
     ofn_next_key = 1;
}

static void ofn_work_f(struct work_struct *work)
{
    struct avago_ofn_data   *dd = ofn_data;
    uint8_t delta_xy_buf[2] = {23, 23}; 
    uint8_t motion = 23;
    int rc = 0;
    int ii = 0, jj = 0;
    int x_zero = 0, y_zero = 0;
    int up = 0, down = 0, left = 0, right = 0;
    struct input_dev *ofn_input_dev = NULL;
    unsigned short keycode = 0;



#if 1
    rc = ofn_i2c_read(dd->clientp, OFN_REG_MOTION, &motion, 1);
    rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_X, delta_xy_buf, 2);
#else
    rc = ofn_i2c_read(dd->clientp, OFN_REG_MOTION, &motion, 1);
    rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_X, &delta_xy_buf[0], 1);
    rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_Y, &delta_xy_buf[1], 1);
#endif


    if (rc)
	goto ofn_work_err_exit;



if (((0 == delta_xy_buf[0]) && (0 == delta_xy_buf[1])) || (ofn_int_num > 4))
{
    ofn_int_num = 0;
    goto ofn_work_err_exit;
}
if (ofn_int_num < 4)
{
    g_delta_x_buf[ofn_int_num] = delta_xy_buf[0];
    g_delta_y_buf[ofn_int_num] = delta_xy_buf[1];
    ofn_int_num++;
}
if (4 == ofn_int_num)
{
    for (ii = 0; ii < 4; ii++)
    {
        if (0 == g_delta_x_buf[ii])
        {
            x_zero++;
        }
	 else if (g_delta_x_buf[ii] > 128)
	 {
	     down++;
	 }
	 else if (g_delta_x_buf[ii] < 127)
	 {
	     up++;
	 }
    }

    for (jj = 0; jj < 4; jj++)
    {
        if (0 == g_delta_y_buf[jj])
        {
            y_zero++;
        }
	 else if (g_delta_y_buf[jj] > 128)
	 {
	     right++;
	 }
	 else if (g_delta_y_buf[jj] < 127)
	 {
	     left++;
	 }
    }

    
#if defined(CONFIG_MACH_RAISE)
    ofn_input_dev = raise_keypad_get_input_dev();
#elif defined(CONFIG_MACH_MOONCAKE)
    ofn_input_dev = mooncake_keypad_get_input_dev();
#endif

    if ((x_zero >= 1) && (right >= 4) && (1 == ofn_next_key))
    {
        keycode = KEY_RIGHT;
        input_report_key(ofn_input_dev, keycode, 1);
    }
    else if ((x_zero >= 1) && (left >= 4) && (1 == ofn_next_key))
    {
        keycode = KEY_LEFT;
        input_report_key(ofn_input_dev, keycode, 1);
    }
    else if ((y_zero >= 1) && (down >= 4) && (1 == ofn_next_key))
    {
        keycode = KEY_DOWN;
        input_report_key(ofn_input_dev, keycode, 1);
    }
    else if ((y_zero >= 1) && (up >= 4) && (1 == ofn_next_key))
    {
        keycode = KEY_UP;
        input_report_key(ofn_input_dev, keycode, 1);
    }

if ((KEY_RIGHT == keycode) || (KEY_LEFT == keycode)
    || (KEY_DOWN == keycode) || (KEY_UP == keycode))
{
    input_report_key(ofn_input_dev, keycode, 0);

    ofn_next_key = 0;
    mod_timer(&ofn_next_key_timer, jiffies + 23); // here 23 is 230ms
}

    ofn_int_num = 0;

}

ofn_work_err_exit:
	return;
}


static irqreturn_t ofn_interrupt(int irq, void *dev_id)
{
	schedule_work(&ofn_work);
	return IRQ_HANDLED;
}

static int ofn_config(struct avago_ofn_data *dd)
{
    uint8_t            buf[2];
    int           rc;
    uint8_t delta_xy_buf[2] = {23, 23}; 
    uint8_t motion = 23;


    ofn_i2c_write(dd->clientp, 0x3a, 0x5A);
    mdelay(23); 

    ofn_i2c_write(dd->clientp, 0x60, 0xE4);

    ofn_i2c_write(dd->clientp, 0x62, 0x12);
    ofn_i2c_write(dd->clientp, 0x63, 0x0E);

    ofn_i2c_write(dd->clientp, 0x64, 0x08);

    ofn_i2c_write(dd->clientp, 0x65, 0x06);

    ofn_i2c_write(dd->clientp, 0x66, 0x40);

    ofn_i2c_write(dd->clientp, 0x67, 0x08);

    ofn_i2c_write(dd->clientp, 0x68, 0x48);

// 0x69 with 0x0a
    ofn_i2c_write(dd->clientp, 0x69, 0x0a);

// 0x6a with 0x50
    ofn_i2c_write(dd->clientp, 0x6a, 0x50);

// 0x6b with 0x48
    ofn_i2c_write(dd->clientp, 0x6b, 0x48);


    ofn_i2c_write(dd->clientp, 0x6e, 0x34);

// write 0x6f with 0x3c
    ofn_i2c_write(dd->clientp, 0x6f, 0x3c);

// write 0x70 with 0x18
    ofn_i2c_write(dd->clientp, 0x70, 0x18);

// write 0x71 with 0x20
    ofn_i2c_write(dd->clientp, 0x71, 0x20);


    ofn_i2c_write(dd->clientp, 0x75, 0x50);


    ofn_i2c_write(dd->clientp, 0x73, 0x99);

// write 0x74 with 0x02
    ofn_i2c_write(dd->clientp, 0x74, 0x02);


    ofn_i2c_write(dd->clientp, 0x1C, 0x10);

        rc = ofn_i2c_read(dd->clientp, 0x02, &motion, 1);
        rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_X, &delta_xy_buf[0], 1);
        rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_Y, &delta_xy_buf[1], 1);

    ofn_i2c_write(dd->clientp, 0x1a, 0x08); 
    rc = ofn_i2c_read(dd->clientp, OFN_REG_PRODUCT_ID, buf, 1);
    if (rc)
	goto ofn_conf_err_exit;



    return 0;

ofn_conf_err_exit:
	return -1;
}

static int __exit avago_ofn_remove(struct i2c_client *client)
{
	struct avago_ofn_data              *dd;

	dd = i2c_get_clientdata(client);
	kfree(dd);
	i2c_set_clientdata(client, NULL);
	ofn_data = NULL;

    free_irq(GPIO_OFN_MOTION_INT, NULL);
    gpio_direction_output(GPIO_OFN_SHUTDOWN, 1);
    gpio_direction_output(GPIO_OFN_RESET, 1);
    mdelay(503); // Minimum: 500ms
    gpio_direction_output(GPIO_OFN_RESET, 1);
	return 0;
}

static const struct i2c_device_id avago_ofn_id[] = {
	{ "avago_ofn", 0},
	{ },
};

static struct i2c_driver ofn_driver = {
	.id_table = avago_ofn_id,
	.driver = {
		.name   = "avago_ofn",
		.owner  = THIS_MODULE,
	},
	.probe   = avago_ofn_probe,
	.remove  =  __exit_p(avago_ofn_remove),

};

static int avago_ofn_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int                                rc;
	struct avago_ofn_platform_data *pd;
	struct avago_ofn_data              *dd;

	if (ofn_data) {
		dev_err(&client->dev,
			"ofn_probe: only a single ofn supported\n");
		rc = -EPERM;
		goto probe_exit;
	}

	dd = kzalloc(sizeof *dd, GFP_KERNEL);
	if (!dd) {
		rc = -ENOMEM;
		goto probe_exit;
	}
	ofn_data = dd;
	i2c_set_clientdata(client, dd);

	dd->clientp = client;
	client->driver = &ofn_driver;
	pd = client->dev.platform_data;
	if (!pd) {
		dev_err(&client->dev,
			"ofn_probe: platform data not set\n");
		rc = -EFAULT;
		goto probe_free_exit;
	}

	dd->gpio_irq = pd->gpio_irq;
	dd->gpio_shutdown = pd->gpio_shutdown;
	dd->gpio_reset= pd->gpio_reset;

	gpio_direction_output(GPIO_OFN_SHUTDOWN, 1);
	gpio_direction_output(GPIO_OFN_RESET, 1);
       mdelay(803); // Minimum: 500ms

       gpio_direction_output(GPIO_OFN_RESET, 1);



	rc = gpio_direction_input(GPIO_OFN_MOTION);
	if (rc) {
		goto probe_free_exit;
	}
	rc = request_irq(GPIO_OFN_MOTION_INT, &ofn_interrupt,
		                   (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_LOW), "ofn", 0); 
	if (rc) {
		goto probe_free_exit;
	}
#if 1
	gpio_direction_output(GPIO_OFN_SHUTDOWN, 0);
	gpio_direction_output(GPIO_OFN_RESET, 0);
       udelay(39); // Minimum: 20us
	gpio_direction_output(GPIO_OFN_RESET, 1);

       mdelay(203); // Minimum: 100ms
#endif

	init_timer(&ofn_next_key_timer);
	ofn_next_key_timer.function = ofn_next_key_timer_callback;
	ofn_next_key_timer.data = 0;

	rc = ofn_config(dd);
	if (rc) {
		goto probe_fail_ofn_config;
	}
	return 0;


probe_fail_ofn_config:
probe_free_exit:
probe_exit:
	return rc;
}

static int __init ofn_init(void)
{
	int rc;


	rc = i2c_add_driver(&ofn_driver);
	if (rc) {
		goto init_exit;
	}

	return 0;

init_exit:
	return rc;
}

static void __exit ofn_exit(void)
{
	i2c_del_driver(&ofn_driver);
}

module_init(ofn_init);
module_exit(ofn_exit);

#else 

static int ofn_i2c_read(struct i2c_client *client,
			     int reg, u8 *buf, int count)
{
	int rc;
	int ret = 0;

       if  (1 == count)
       {
	    buf[0] = reg;
       }
	else if (count >= 2)
	{
	    buf[0] = OFN_REG_AI_BIT | reg;
	}
	rc = i2c_master_send(client, buf, 1);
	if (rc != 1) {
		dev_err(&client->dev,
		       "ofn_i2c_read FAILED: read of register %d\n", reg);
		ret = -1;
		goto ofn_i2c_rd_exit;
	}
	rc = i2c_master_recv(client, buf, count);
	if (rc != count) {
		dev_err(&client->dev,
		       "ofn_i2c_read FAILED: read %d bytes from reg %d\n",
		       count, reg);
		ret = -1;
	}

ofn_i2c_rd_exit:
	return ret;
}

static int ofn_i2c_write(struct i2c_client *client, int reg, u8 data)
{
	u8            buf[2];
	int           rc;
	int           ret = 0;

	buf[0] = reg;
	buf[1] = data;
	rc = i2c_master_send(client, buf, 2);
	if (rc != 2) {
		dev_err(&client->dev,
		       "ofn_i2c_write FAILED: writing to reg %d\n", reg);
		ret = -1;
	}
	return ret;
}

#if 0 // for U700 project
static int ofn_fpd_check(void) 
{	
 	unsigned char i2c_buffer[7];

		
#if 1
			i2c_buffer[0] = ADBM_A320_SQUAL_ADDR|0x80; 
			i2c_onfa320_read_register(onfa320_client.adapter, i2c_buffer,6);	
			squal  	=	 i2c_buffer[1];
			shutter_h  =	 i2c_buffer[2];
			shutter_l  =	 i2c_buffer[3];
			pix_max  =	 i2c_buffer[4];
			pix_avg   =	 i2c_buffer[5];
			pix_min  =	 i2c_buffer[6];


			shutter = shutter_h;
			shutter =  shutter<<8 | shutter_l;
			

			if ( (abs(pix_max - pix_min) > 240) || (squal < 15) || (shutter < 60) || (shutter >700) )
			{	

				return  0;
			}
			else
			{
				return 1;
			}
#endif
}

#else

static int ofn_fpd_check(void)
{	
 	unsigned char pix_min, pix_max, pix_avg;
 	unsigned char squal, shutter_h, shutter_l;
 	unsigned int shutter;
 	struct avago_ofn_data   *dd = ofn_data;
 	uint8_t i2c_buffer;


 	ofn_i2c_read(dd->clientp, 0x05, &i2c_buffer, 1); // SQUAL
 	squal  	=	 i2c_buffer;

 	ofn_i2c_read(dd->clientp, 0x06, &i2c_buffer, 1); // Shutter_Upper
 	shutter_h  =	 i2c_buffer;

 	ofn_i2c_read(dd->clientp, 0x07, &i2c_buffer, 1); // Shutter_Lower
 	shutter_l  =	 i2c_buffer;

 	ofn_i2c_read(dd->clientp, 0x08, &i2c_buffer, 1); // Maximum_Pixel
 	pix_max  =	 i2c_buffer;

 	ofn_i2c_read(dd->clientp, 0x09, &i2c_buffer, 1); // Pixel_Sum
 	pix_avg   =	 i2c_buffer;

 	ofn_i2c_read(dd->clientp, 0x0A, &i2c_buffer, 1); // Minimum_Pixel
 	pix_min  =	 i2c_buffer;

 	shutter = shutter_h;
 	shutter =  shutter<<8 | shutter_l;


 	ofn_i2c_read(dd->clientp, 0x63, &i2c_buffer, 1); // OFN_Speed_Control

 	if ( (abs(pix_max - pix_min) > 240) || (shutter < 0x23)) // 0x23 is 35
 	{	
 	    return  0;
 	}
 	else
 	{
 	    return 1;
 	}
}
#endif

#if 0
static void ofn_report_rel(void)
{
    struct avago_ofn_data   *dd = ofn_data;
    struct input_dev *ofn_input_dev = dd->ofn_input_dev;
    int loop = 0;

    if ((abs(rocker_delta_x) > Step_Thresh_X) || (abs(rocker_delta_y) > Step_Thresh_Y))
    {
    	if (abs(rocker_delta_x) > abs(rocker_delta_y))
    	{
    	       dd->delta_x = 0;
		if (rocker_delta_x > Step_Thresh_X)  dd->delta_y = -1; // = UP;
		else if (rocker_delta_x < -Step_Thresh_X) dd->delta_y = 1; // = DOWN;
		else dd->delta_y = 0;
    	}
	else
	{
    	       dd->delta_y = 0;
		if (rocker_delta_y > Step_Thresh_Y)  dd->delta_x = -1; // = LEFT;
		else if (rocker_delta_y < -Step_Thresh_Y) dd->delta_x = 1; // = RIGHT;
		else dd->delta_x = 0;
	}
    }
    else
    {
    	if((rocker_delta_x < 0) && (g_delta_x > 0))
    	{
    	    rocker_delta_x = 0;
    	}
    	else if((rocker_delta_x > 0) && (g_delta_x < 0))
    	{
    	    rocker_delta_x = 0;
    	}

    	if((rocker_delta_y < 0) && (g_delta_y > 0))
    	{
    	    rocker_delta_y = 0;
    	}
    	else if((rocker_delta_y > 0) && (g_delta_y < 0))
    	{
    	    rocker_delta_y = 0;
    	}
        return;
    }

    printk("chenjun: ofn_report_rel: dd : delta_x = %d, delta_y = %d\n", dd->delta_x, dd->delta_y);

    if (((0 != dd->delta_x) || (0 != dd->delta_y)) && (1 == ofn_fpd_check()))
    {
        if (0 != dd->delta_x)
        {
            printk("chenjun:input_report_rel:REL_X\n");
            for (loop = 0; loop < 3; loop++)
            {
                input_report_rel(ofn_input_dev, REL_X, dd->delta_x);
                input_sync(ofn_input_dev);
            }
        }
        else if (0 != dd->delta_y)
        {
            printk("chenjun:input_report_rel:REL_Y\n");
            for (loop = 0; loop < 3; loop++)
            {
                input_report_rel(ofn_input_dev, REL_Y, dd->delta_y);
                input_sync(ofn_input_dev);
            }
        }
        dd->delta_x = 0;
        dd->delta_y = 0;
        rocker_delta_x = 0;
        rocker_delta_y = 0;
    }
}
#endif

#if 0
static void ofn_next_key_timer_callback(unsigned long data)
{

}
#endif

static void ofn_work_f(struct work_struct *work)
{
    struct avago_ofn_data   *dd = ofn_data;
    int8_t delta_xy_buf[2] = {23, 23}; 
    uint8_t motion = 23;
    int rc = 0;

    struct input_dev *ofn_input_dev = dd->ofn_input_dev;

#if 1
    rc = ofn_i2c_read(dd->clientp, OFN_REG_MOTION, &motion, 1);
    rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_X, delta_xy_buf, 2);
#else
    rc = ofn_i2c_read(dd->clientp, OFN_REG_MOTION, &motion, 1);
    rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_X, &delta_xy_buf[0], 1);
    rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_Y, &delta_xy_buf[1], 1);
#endif


    if (rc)
	goto ofn_work_err_exit;

 

#if 0
if (((0 == delta_xy_buf[0]) && (0 == delta_xy_buf[1])) || (ofn_int_num > 4))
{
    ofn_int_num = 0;
    goto ofn_work_err_exit;
}
if (ofn_int_num < 4)
{
    g_delta_x_buf[ofn_int_num] = delta_xy_buf[0];
    g_delta_y_buf[ofn_int_num] = delta_xy_buf[1];
    ofn_int_num++;
}
if (4 == ofn_int_num)
{
    for (ii = 0; ii < 4; ii++)
    {
        if (0 == g_delta_x_buf[ii])
        {
            x_zero++;
        }
	 else if (g_delta_x_buf[ii] > 128)
	 {
	     down++;
	 }
	 else if (g_delta_x_buf[ii] < 127)
	 {
	     up++;
	 }
    }

    for (jj = 0; jj < 4; jj++)
    {
        if (0 == g_delta_y_buf[jj])
        {
            y_zero++;
        }
	 else if (g_delta_y_buf[jj] > 128)
	 {
	     right++;
	 }
	 else if (g_delta_y_buf[jj] < 127)
	 {
	     left++;
	 }
    }

    

#if defined(CONFIG_MACH_RAISE)
    ofn_input_dev = raise_keypad_get_input_dev();
#elif defined(CONFIG_MACH_MOONCAKE)
    ofn_input_dev = mooncake_keypad_get_input_dev();
#endif

    if ((x_zero >= 1) && (right >= 4) && (1 == ofn_next_key))
    {
        keycode = KEY_RIGHT;
        input_report_key(ofn_input_dev, keycode, 1);
    }
    else if ((x_zero >= 1) && (left >= 4) && (1 == ofn_next_key))
    {
        keycode = KEY_LEFT;
        input_report_key(ofn_input_dev, keycode, 1);
    }
    else if ((y_zero >= 1) && (down >= 4) && (1 == ofn_next_key))
    {
        keycode = KEY_DOWN;
        input_report_key(ofn_input_dev, keycode, 1);
    }
    else if ((y_zero >= 1) && (up >= 4) && (1 == ofn_next_key))
    {
        keycode = KEY_UP;
        input_report_key(ofn_input_dev, keycode, 1);
    }

if ((KEY_RIGHT == keycode) || (KEY_LEFT == keycode)
    || (KEY_DOWN == keycode) || (KEY_UP == keycode))
{
    input_report_key(ofn_input_dev, keycode, 0);

    ofn_next_key = 0;
    mod_timer(&ofn_next_key_timer, jiffies + 23); // here 23 is 230ms
}

    ofn_int_num = 0;

}
#endif



    if (motion & 0x80) //check if the report data is valid
    {
#if 1

#if 0
    dd->delta_x = -delta_xy_buf[1];
    dd->delta_y = -delta_xy_buf[0];
#elif 0
    if (delta_xy_buf[1] > 0)
    {
        dd->delta_x = -1;
    }
    else if (delta_xy_buf[1] < 0)
    {
        dd->delta_x = 1;
    }
    else if (0 == delta_xy_buf[1])
    {
        dd->delta_x = 0;
    }

    if (delta_xy_buf[0] > 0)
    {
        dd->delta_y = -1;
    }
    else if (delta_xy_buf[0] < 0)
    {
        dd->delta_y = 1;
    }
    if (0 == delta_xy_buf[0])
    {
        dd->delta_y = 0;
    }
#else
#if 0
    if (ofn_next_key)
    {
        ofn_next_key = 0;
        mod_timer(&ofn_next_key_timer, jiffies + 5); // here 5 is 50ms
    }
#endif
    rocker_delta_x = rocker_delta_x + delta_xy_buf[0];
    rocker_delta_y = rocker_delta_y + delta_xy_buf[1];
    
    
#if 1 // use timer
#ifdef USE_XYQ_MODE
    if (1 || (abs(rocker_delta_x) > Step_Thresh_sum) || (abs(rocker_delta_y) > Step_Thresh_sum))
    {
    	if (abs(rocker_delta_x) > abs(rocker_delta_y))
    	{
    	       dd->delta_x = 0;
		if (rocker_delta_x > 0)  dd->delta_y = -1; // = UP;
		else if (rocker_delta_x < 0) dd->delta_y = 1; // = DOWN;
		else dd->delta_y = 0;
    	}
	else
	{
    	       dd->delta_y = 0;
		if (rocker_delta_y > 0)  dd->delta_x = -1; // = LEFT;
		else if (rocker_delta_y < 0) dd->delta_x = 1; // = RIGHT;
		else dd->delta_x = 0;
	}
    }
#else
    if (/* (0 == ofn_in_timer) && */((abs(rocker_delta_x) > Step_Thresh_X) || (abs(rocker_delta_y) > Step_Thresh_Y)))
    {
    	if (abs(rocker_delta_x) > abs(rocker_delta_y))
    	{
    	       dd->delta_x = 0;
		if (rocker_delta_x > Step_Thresh_X)  dd->delta_y = -1; // = UP;
		else if (rocker_delta_x < -Step_Thresh_X) dd->delta_y = 1; // = DOWN;
		else dd->delta_y = 0;
    	}
	else
	{
    	       dd->delta_y = 0;
		if (rocker_delta_y > Step_Thresh_Y)  dd->delta_x = -1; // = LEFT;
		else if (rocker_delta_y < -Step_Thresh_Y) dd->delta_x = 1; // = RIGHT;
		else dd->delta_x = 0;
	}
    }
#endif
    else
    {
#if 0
    	if((rocker_delta_x < 0) && (g_delta_x > 0))
    	{
    	    rocker_delta_x = 0;
    	}
    	else if((rocker_delta_x > 0) && (g_delta_x < 0))
    	{
    	    rocker_delta_x = 0;
    	}

    	if((rocker_delta_y < 0) && (g_delta_y > 0))
    	{
    	    rocker_delta_y = 0;
    	}
    	else if((rocker_delta_y > 0) && (g_delta_y < 0))
    	{
    	    rocker_delta_y = 0;
    	}
        goto no_reach_thresh;
#else
        return;
#endif
    }
#endif // use timer
#endif

#if 1 // use timer

#if 0
    if (((0 != dd->delta_x) || (0 != dd->delta_y)) && (1 == ofn_fpd_check()))
    {
        if (0 != dd->delta_x)
        {
            printk("chenjun:input_report_rel:REL_X\n");
            input_report_rel(ofn_input_dev, REL_X, dd->delta_x);
        }
        else if (0 != dd->delta_y)
        {
            printk("chenjun:input_report_rel:REL_Y\n");
            input_report_rel(ofn_input_dev, REL_Y, dd->delta_y);
        }
        input_sync(ofn_input_dev);
        dd->delta_x = 0;
        dd->delta_y = 0;
        rocker_delta_x = 0;
        rocker_delta_y = 0;
    }
#else
    if (((0 != dd->delta_x) || (0 != dd->delta_y)) && (1 == ofn_fpd_check()))
    {
        if (0 != dd->delta_x)
        {
            {
                input_report_rel(ofn_input_dev, REL_X, dd->delta_x);
            }
        }
        else if (0 != dd->delta_y)
        {
            {
                input_report_rel(ofn_input_dev, REL_Y, dd->delta_y);
            }
        }
        input_sync(ofn_input_dev);
        dd->delta_x = 0;
        dd->delta_y = 0;
        rocker_delta_x = 0;
        rocker_delta_y = 0;
    }
#endif

#endif // use timer
#else
if (1 == ofn_next_key)
{
    if (ofn_int_num < 4)
    {
        input_report_rel(ofn_input_dev, REL_Y, 1);
        input_sync(ofn_input_dev);
        ofn_int_num++;
    }
    else
    {
        ofn_int_num = 0;
        ofn_next_key = 0;
        mod_timer(&ofn_next_key_timer, jiffies + 100); // here 100 is 1000ms
    }
}
#endif
    }
    else
    {
    }

#if 0
no_reach_thresh:
    if (ofn_next_key)
    {
        ofn_next_key = 0;
        mod_timer(&ofn_next_key_timer, jiffies + 5); // here 5 is 50ms
        ofn_in_timer = 1;
    }
#endif

ofn_work_err_exit:
	return;
}


static irqreturn_t ofn_interrupt(int irq, void *dev_id)
{
	schedule_work(&ofn_work);
	return IRQ_HANDLED;
}


static int ofn_config(struct avago_ofn_data *dd)
{

     int           rc;

   rc= ofn_i2c_write(dd->clientp, 0x3a, 0x5A);
    mdelay(20); 

#ifdef USE_XYQ_MODE
    rc= ofn_i2c_write(dd->clientp, 0x60, 0xB4);
#else
   rc=  ofn_i2c_write(dd->clientp, 0x60, 0xE4);
#endif

#ifdef USE_XYQ_MODE
   rc=  ofn_i2c_write(dd->clientp, 0x62, 0x09); 
#else
   rc=  ofn_i2c_write(dd->clientp, 0x62, 0x12);
#endif

#ifndef USE_XYQ_MODE
  rc=   ofn_i2c_write(dd->clientp, 0x63, 0x04); 


    rc= ofn_i2c_write(dd->clientp, 0x64, 0x08);

   rc=  ofn_i2c_write(dd->clientp, 0x65, 0x06);

   rc=  ofn_i2c_write(dd->clientp, 0x66, 0x40);

   rc=  ofn_i2c_write(dd->clientp, 0x67, 0x08);

   rc=  ofn_i2c_write(dd->clientp, 0x68, 0x48);

   rc=  ofn_i2c_write(dd->clientp, 0x69, 0x0a);

   rc=  ofn_i2c_write(dd->clientp, 0x6a, 0x50);

   rc=  ofn_i2c_write(dd->clientp, 0x6b, 0x48);
#endif


   rc=  ofn_i2c_write(dd->clientp, 0x6d, 0xc4); // 0xc4(higher cpi)->0xc2(low cpi)

//

   rc=  ofn_i2c_write(dd->clientp, 0x6e, 0x2e); // 0x34->0x2e
// write 0x6f with 0x3c
   rc=  ofn_i2c_write(dd->clientp, 0x6f, 0x3b);
// write 0x70 with 0x18
   rc=  ofn_i2c_write(dd->clientp, 0x70, 0x2e);
// write 0x71 with 0x20
   rc=  ofn_i2c_write(dd->clientp, 0x71, 0x3b);

   rc=  ofn_i2c_write(dd->clientp, 0x75, 0x46);

//

//
#ifdef USE_XYQ_MODE

   rc=  ofn_i2c_write(dd->clientp, 0x73, 0x99);

   rc=  ofn_i2c_write(dd->clientp, 0x74, 0x07);
#endif

   rc=  ofn_i2c_write(dd->clientp, 0x21, 0x0f);
   rc=  ofn_i2c_write(dd->clientp, 0x22, 0x0f);
   rc=  ofn_i2c_write(dd->clientp, 0x29, 0xc1);
   rc=  ofn_i2c_write(dd->clientp, 0x2b, 0x18);
   rc=  ofn_i2c_write(dd->clientp, 0x2c, 0x09);

#if 0 
//
        rc = ofn_i2c_read(dd->clientp, 0x02, &motion, 1);
        rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_X, &delta_xy_buf[0], 1);
        rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_Y, &delta_xy_buf[1], 1);
//

    rc = ofn_i2c_read(dd->clientp, OFN_REG_PRODUCT_ID, buf, 1);
    if (rc)
	goto ofn_conf_err_exit;

 
#endif
    return 0;


	 return rc;
}

static int __exit avago_ofn_remove(struct i2c_client *client)
{
	struct avago_ofn_data              *dd;

	dd = i2c_get_clientdata(client);
	input_unregister_device(dd->ofn_input_dev);
	kfree(dd);
	i2c_set_clientdata(client, NULL);
	ofn_data = NULL;

//
    free_irq(GPIO_OFN_MOTION_INT, NULL);
    gpio_direction_output(GPIO_OFN_SHUTDOWN, 1);
    gpio_direction_output(GPIO_OFN_RESET, 1);
    mdelay(150); // Minimum: 150ms // 500ms
//
	return 0;
}

static int avago_ofn_suspend(struct i2c_client *client,
			    pm_message_t state)
{


	printk("chenjun: ofn:SHUTDOWN = %d\n", gpio_get_value(GPIO_OFN_SHUTDOWN));
	printk("chenjun: ofn:RESET = %d\n", gpio_get_value(GPIO_OFN_RESET));
	printk("chenjun: ofn: MOTION = %d\n", gpio_get_value(GPIO_OFN_MOTION));

	disable_irq(GPIO_OFN_MOTION_INT);

	gpio_direction_output(GPIO_OFN_SHUTDOWN, 1);
       mdelay(151); // Minimum: 150ms // 500ms
       
	cancel_work_sync(&ofn_work);
    
	return 0;
}

static int avago_ofn_resume(struct i2c_client *client)
{
	struct avago_ofn_data *dd;
	uint8_t delta_xy_buf[2] = {23, 23}; 
	uint8_t motion = 23;


	printk("chenjun: ofn:SHUTDOWN = %d\n", gpio_get_value(GPIO_OFN_SHUTDOWN));
	printk("chenjun: ofn:RESET = %d\n", gpio_get_value(GPIO_OFN_RESET));
	printk("chenjun: ofn: MOTION = %d\n", gpio_get_value(GPIO_OFN_MOTION));


	dd = i2c_get_clientdata(client);

	gpio_direction_output(GPIO_OFN_SHUTDOWN, 0);
       mdelay(101); // Minimum: 100ms

	enable_irq(GPIO_OFN_MOTION_INT);

       ofn_i2c_read(dd->clientp, 0x02, &motion, 1);
       ofn_i2c_read(dd->clientp, OFN_REG_DELTA_X, &delta_xy_buf[0], 1);
       ofn_i2c_read(dd->clientp, OFN_REG_DELTA_Y, &delta_xy_buf[1], 1);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ofn_early_suspend(struct early_suspend *h)
{
	avago_ofn_suspend(ofn_data->clientp, PMSG_SUSPEND);
}

static void ofn_late_resume(struct early_suspend *h)
{
	avago_ofn_resume(ofn_data->clientp);
}
#endif

static const struct i2c_device_id avago_ofn_id[] = {
	{ "avago_ofn", 0},
	{ },
};

static struct i2c_driver ofn_driver = {
	.id_table = avago_ofn_id,
	.driver = {
		.name   = "avago_ofn",
		.owner  = THIS_MODULE,
	},
	.probe   = avago_ofn_probe,
	.remove  =  __exit_p(avago_ofn_remove),
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = avago_ofn_suspend,
	.resume  = avago_ofn_resume,
#endif
};

static int ofn_input_dev_open(struct input_dev *dev)
{
	int  rc = 0;
	struct avago_ofn_data  *dd = input_get_drvdata(dev);
	uint8_t            buf[2];
	uint8_t delta_xy_buf[2] = {23, 23}; 
	uint8_t motion = 23;
       static int open_time = 0;

       open_time++;
       if (open_time > 1)
       {
           return 0; // only open one time
       }
       
	if (!dd) {
		rc = -ENOMEM;
		goto dev_open_exit;
	}
    
#if 1
	printk("chenjun: ofn_input_dev_open: SHUTDOWN = %d\n", gpio_get_value(GPIO_OFN_SHUTDOWN));
	printk("chenjun: ofn_input_dev_open: RESET = %d\n", gpio_get_value(GPIO_OFN_RESET));

#if 0	
	gpio_direction_output(GPIO_OFN_SHUTDOWN, 1);
	gpio_direction_output(GPIO_OFN_RESET, 1);
       mdelay(803); // Minimum: 500ms

       gpio_direction_output(GPIO_OFN_RESET, 1);
#endif

	
	
#if 0
	rc = gpio_direction_input(GPIO_OFN_MOTION);
	if (rc) {
		goto dev_open_exit;
	}
	rc = request_irq(GPIO_OFN_MOTION_INT, &ofn_interrupt,
		                   IRQF_TRIGGER_FALLING, "ofn", 0); 
	if (rc) {

		goto dev_open_exit;
	}
#endif
#if 1
	gpio_direction_output(GPIO_OFN_SHUTDOWN, 0);
	gpio_direction_output(GPIO_OFN_RESET, 0);
       udelay(23); // 39 // Minimum: 20us
	gpio_direction_output(GPIO_OFN_RESET, 1);

       mdelay(100); // 203 // Minimum: 100ms
#endif

#if 0
	init_timer(&ofn_next_key_timer);
	ofn_next_key_timer.function = ofn_next_key_timer_callback;
	ofn_next_key_timer.data = 0;
#endif
	rc = ofn_config(dd);
     if (rc!=0)
	 goto	ofn_config_fail;

#if 1
	rc = gpio_direction_input(GPIO_OFN_MOTION);
	if (rc) {
		goto dev_open_exit;
	}
	rc = request_irq(GPIO_OFN_MOTION_INT, &ofn_interrupt,
		                   IRQF_TRIGGER_FALLING, "ofn", 0); 
	if (rc) {
		goto dev_open_exit;
	}
#endif
#if 1
//
        rc = ofn_i2c_read(dd->clientp, 0x02, &motion, 1);
        rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_X, &delta_xy_buf[0], 1);
        rc = ofn_i2c_read(dd->clientp, OFN_REG_DELTA_Y, &delta_xy_buf[1], 1);
//

    rc = ofn_i2c_read(dd->clientp, OFN_REG_PRODUCT_ID, buf, 1);
    if (rc)
	goto ofn_config_fail;

#endif

#ifdef CONFIG_HAS_EARLYSUSPEND

	dd->early_suspend.level = 52;
	dd->early_suspend.suspend = ofn_early_suspend;
	dd->early_suspend.resume = ofn_late_resume;
	register_early_suspend(&dd->early_suspend);
#endif

	return 0;


ofn_config_fail:
dev_open_exit:
	return rc;
#endif
}

#if 0 
static void ofn_input_dev_close(struct input_dev *dev)
{
	free_irq(GPIO_OFN_MOTION_INT, NULL);
	cancel_work_sync(&ofn_work);
}
#endif

static int avago_ofn_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int                                rc;
	struct avago_ofn_platform_data *pd;
	struct avago_ofn_data              *dd;

	if (ofn_data) {
		dev_err(&client->dev,
			"ofn_probe: only a single ofn supported\n");
		rc = -EPERM;
		goto probe_exit;
	}

	dd = kzalloc(sizeof *dd, GFP_KERNEL);
	if (!dd) {
		rc = -ENOMEM;
		goto probe_exit;
	}
	ofn_data = dd;
	i2c_set_clientdata(client, dd);

	dd->clientp = client;
	client->driver = &ofn_driver;
	pd = client->dev.platform_data;
	if (!pd) {
		dev_err(&client->dev,
			"ofn_probe: platform data not set\n");
		rc = -EFAULT;
		goto probe_free_exit;
	}

	dd->gpio_irq = pd->gpio_irq;
	dd->gpio_shutdown = pd->gpio_shutdown;
	dd->gpio_reset= pd->gpio_reset;

	dd->ofn_input_dev= input_allocate_device();
	if (!dd->ofn_input_dev) {
		rc = -ENOMEM;
		goto probe_free_exit;
	}

	input_set_drvdata(dd->ofn_input_dev, dd);
	dd->ofn_input_dev->open       = ofn_input_dev_open;
	dd->ofn_input_dev->name       = "avago_ofn";
	dd->ofn_input_dev->id.bustype = BUS_I2C;
	dd->ofn_input_dev->id.vendor  = QUALCOMM_VENDORID;
	dd->ofn_input_dev->id.product = 1;
	dd->ofn_input_dev->id.version = 1;
	__set_bit(EV_REL,    dd->ofn_input_dev->evbit);
	__set_bit(REL_X,     dd->ofn_input_dev->relbit);
	__set_bit(REL_Y,     dd->ofn_input_dev->relbit);
	__set_bit(BTN_MOUSE, dd->ofn_input_dev->keybit);

	rc = input_register_device(dd->ofn_input_dev);
	if (rc) {
		dev_err(&client->dev,
			"ofn_probe: input_register_device rc=%d\n",
		       rc);
		goto probe_free_exit;
	}

	printk("avago_ofn_probe: SHUTDOWN = %d\n", gpio_get_value(GPIO_OFN_SHUTDOWN));
	printk("avago_ofn_probe: RESET = %d\n", gpio_get_value(GPIO_OFN_RESET));

#if 0 
	printk("avago_ofn_probe: SHUTDOWN = %d\n", gpio_get_value(GPIO_OFN_SHUTDOWN));
	printk("avago_ofn_probe: RESET = %d\n", gpio_get_value(GPIO_OFN_RESET));

#if 0	
	gpio_direction_output(GPIO_OFN_SHUTDOWN, 1);
	gpio_direction_output(GPIO_OFN_RESET, 1);
       mdelay(803); // Minimum: 500ms

       gpio_direction_output(GPIO_OFN_RESET, 1);
#endif

	
       gpio_direction_output(GPIO_OFN_RESET, 1);

	rc = gpio_direction_input(GPIO_OFN_MOTION);
	if (rc) {
		goto probe_free_exit;
	}
	rc = request_irq(GPIO_OFN_MOTION_INT, &ofn_interrupt,
		                   IRQF_TRIGGER_FALLING, "ofn", 0);
	if (rc) {
		goto probe_free_exit;
	}
#if 1
	gpio_direction_output(GPIO_OFN_SHUTDOWN, 0);
	gpio_direction_output(GPIO_OFN_RESET, 0);
       udelay(20); // 39 // Minimum: 20us
	gpio_direction_output(GPIO_OFN_RESET, 1);

       mdelay(100); // 203 // Minimum: 100ms
#endif

	init_timer(&ofn_next_key_timer);
	ofn_next_key_timer.function = ofn_next_key_timer_callback;
	ofn_next_key_timer.data = 0;

	rc = ofn_config(dd);
	if (rc) {
		goto probe_fail_ofn_config;
	}
	return 0;

probe_fail_ofn_config:
	free_irq(GPIO_OFN_MOTION_INT, NULL);
#endif

	return 0;

probe_free_exit:
       kfree(dd);
	ofn_data = NULL;
	i2c_set_clientdata(client, NULL);
probe_exit:
	return rc;
}

static int __init ofn_init(void)
{
	int rc;


	rc = i2c_add_driver(&ofn_driver);
	if (rc) {
		goto init_exit;
	}

	return 0;

init_exit:
	return rc;
}

static void __exit ofn_exit(void)
{
	i2c_del_driver(&ofn_driver);
}

module_init(ofn_init);
module_exit(ofn_exit);

#endif

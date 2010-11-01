
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <linux/proc_fs.h>

static struct workqueue_struct *cypress_wq;
static struct i2c_driver cypress_ts_driver;

#if defined(CONFIG_MACH_BLADE)
#define GPIO_TOUCH_EN_OUT  31
#elif defined(CONFIG_MACH_R750)
#define GPIO_TOUCH_EN_OUT  33
#else
#define GPIO_TOUCH_EN_OUT  31
#endif

struct cypress_ts_data
{
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	struct hrtimer timer;
	struct work_struct  work;
	uint16_t max[2];
	struct early_suspend early_suspend;
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cypress_ts_early_suspend(struct early_suspend *h);
static void cypress_ts_late_resume(struct early_suspend *h);
#endif

static int cypress_i2c_read(struct i2c_client *client, int reg, u8 * buf, int count)
{
    int rc;
    int ret = 0;

    buf[0] = reg;
    rc = i2c_master_send(client, buf, 1);
    if (rc != 1)
    {
        dev_err(&client->dev, "cypress_i2c_read FAILED: read of register %d\n", reg);
        ret = -1;
        goto tp_i2c_rd_exit;
    }
    rc = i2c_master_recv(client, buf, count);
    if (rc != count)
    {
        dev_err(&client->dev, "cypress_i2c_read FAILED: read %d bytes from reg %d\n", count, reg);
        ret = -1;
    }

  tp_i2c_rd_exit:
    return ret;
}
static int cypress_i2c_write(struct i2c_client *client, int reg, u8 data)
{
    u8 buf[2];
    int rc;
    int ret = 0;

    buf[0] = reg;
    buf[1] = data;
    rc = i2c_master_send(client, buf, 2);
    if (rc != 2)
    {
        dev_err(&client->dev, "cypress_i2c_write FAILED: writing to reg %d\n", reg);
        ret = -1;
    }
    return ret;
}

static void cypress_ts_work_func(struct work_struct *work)
{
    int ret;
    int x1 = 0, y1 = 0, pressure = 0,x2 = 0, y2 = 0;
    uint8_t buf[15];
    struct cypress_ts_data *ts = container_of(work, struct cypress_ts_data, work);
    ret = cypress_i2c_read(ts->client, 0x00, buf, 15);
	if(buf[0] == 0x80)cypress_i2c_write(ts->client, 0x0b, 0x80);
	else  pressure = 255;

    x1=buf[2] | (uint16_t)(buf[1] & 0x03) << 8; 
	y1=buf[4] | (uint16_t)(buf[3] & 0x03) << 8; 
    x2=buf[6] | (uint16_t)(buf[5] & 0x03) << 8; 
    y2=buf[8] | (uint16_t)(buf[7] & 0x03) << 8;

	if (((buf[0] == 0x81)&&(x1 == x2)&&(y1 == y2))||(buf[0] == 0x82))
	{
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 255);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x1);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y1);
		input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 10);
		input_mt_sync(ts->input_dev);
	}
	if(buf[0] == 0x82)
	{
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 255);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x2);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y2);
		input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 10);
		input_mt_sync(ts->input_dev);
	}
	input_sync(ts->input_dev);
    input_report_key(ts->input_dev, BTN_TOUCH, !!pressure);
    input_sync(ts->input_dev);
}

static enum hrtimer_restart cypress_ts_timer_func(struct hrtimer *timer)
{
	struct cypress_ts_data *ts = container_of(timer, struct cypress_ts_data, timer);
	queue_work(cypress_wq, &ts->work);

	hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

static irqreturn_t cypress_ts_irq_handler(int irq, void *dev_id)
{
	struct cypress_ts_data *ts = dev_id;

	disable_irq_nosync(ts->client->irq);
	queue_work(cypress_wq, &ts->work);
	enable_irq(ts->client->irq);
	return IRQ_HANDLED;
}

static int cypress_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct cypress_ts_data *ts;
     uint8_t buf1[8];
     int ret = 0;
     uint16_t max_x, max_y;

    gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
    msleep(20);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        printk(KERN_ERR "cypress_ts_probe: need I2C_FUNC_I2C\n");
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }
    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }

    INIT_WORK(&ts->work, cypress_ts_work_func);
    ts->client = client;
    i2c_set_clientdata(client, ts);
    client->driver = &cypress_ts_driver;
    printk("wly: %s\n", __FUNCTION__);
    {
        int retry = 3;
		
        while (retry-- > 0)
        {

            ret = cypress_i2c_read(ts->client, 0x00, buf1, 8);
	     if (ret >= 0)
    	     break;	
    	       msleep(10);
        }
	 if (retry < 0)
	 {
	     ret = -1;
	     goto err_detect_failed;
	 }
    }
    ts->input_dev = input_allocate_device();

    if (ts->input_dev == NULL)
    {
        ret = -ENOMEM;
        printk(KERN_ERR "cypress_ts_probe: Failed to allocate input device\n");
        goto err_input_dev_alloc_failed;
    }
    ts->input_dev->name = "cypress_touch";
    ts->input_dev->phys = "cypress_touch/input0";

    set_bit(EV_SYN, ts->input_dev->evbit);
    set_bit(EV_KEY, ts->input_dev->evbit);
    set_bit(BTN_TOUCH, ts->input_dev->keybit);
    set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts->input_dev->absbit);
	
#if defined(CONFIG_MACH_BLADE)
	max_x=479;
    max_y=799;
#elif defined(CONFIG_MACH_R750)
    max_x=319;
    max_y=479;
#else
    max_x=319;
    max_y=479;
#endif

	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, max_x, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        printk(KERN_ERR "cypress_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
        goto err_input_register_device_failed;
    }
    if (client->irq)
    {
        ret = request_irq(ts->client->irq, cypress_ts_irq_handler, IRQF_TRIGGER_LOW, "cypress_touch", ts);
        if (ret == 0) ts->use_irq = 1;
    }
    if (!ts->use_irq)
    {
        hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        ts->timer.function = cypress_ts_timer_func;
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }
#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = cypress_ts_early_suspend;
    ts->early_suspend.resume = cypress_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif

    printk(KERN_INFO "cypress_ts_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name,
           ts->use_irq ? "interrupt" : "polling");

    return 0;
err_input_register_device_failed:
    input_unregister_device(ts->input_dev);

err_input_dev_alloc_failed:
	input_free_device(ts->input_dev);
err_detect_failed:
    kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
    return ret;
}

static int  cypress_ts_remove(struct i2c_client *client)
{
	struct cypress_ts_data *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(ts->client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	 gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
	return 0;
}

static int cypress_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct cypress_ts_data *ts = i2c_get_clientdata(client);
	if (ts->use_irq)
		disable_irq(ts->client->irq);
	else
		hrtimer_cancel(&ts->timer);
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq) 
		enable_irq(ts->client->irq);
    ret = cypress_i2c_write(ts->client, 0x0a, 0x08);    
	if (ret < 0)
        printk(KERN_ERR "cypress_ts_suspend: cypress_i2c_write failed\n");
	return 0;
}
static int cypress_ts_resume(struct i2c_client *client)
{
    struct cypress_ts_data *ts = i2c_get_clientdata(client);
	gpio_direction_output(29, 0);
	gpio_direction_input(29);
	if (ts->use_irq)
		enable_irq(ts->client->irq);
    if (!ts->use_irq)
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
   

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cypress_ts_early_suspend(struct early_suspend *h)
{
	struct cypress_ts_data *ts;
	ts = container_of(h, struct cypress_ts_data, early_suspend);
	cypress_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void cypress_ts_late_resume(struct early_suspend *h)
{
	struct cypress_ts_data *ts;
	ts = container_of(h, struct cypress_ts_data, early_suspend);
	cypress_ts_resume(ts->client);
}
#endif
static const struct i2c_device_id cypress_touch_id[] = {
	{ "cypress_touch", 0},
	{ },
};
static struct i2c_driver cypress_ts_driver = {

	   .probe		= cypress_ts_probe,
       .remove      = cypress_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	   .suspend	    = cypress_ts_suspend,
	   .resume		= cypress_ts_resume,
#endif
       .id_table    = cypress_touch_id,
	   .driver      = {
                     	.name   = "cypress_touch",
	             		.owner  = THIS_MODULE,
					},
};
static int __devinit cypress_ts_init(void)
{
	cypress_wq = create_rt_workqueue("cypress_wq");
	if (!cypress_wq)
		return -ENOMEM;
	return i2c_add_driver(&cypress_ts_driver);
}

static void __exit cypress_ts_exit(void)
{
	i2c_del_driver(&cypress_ts_driver);
        if (cypress_wq)
		destroy_workqueue(cypress_wq);
}

module_init(cypress_ts_init);
module_exit(cypress_ts_exit);

MODULE_DESCRIPTION("Cypress Touchscreen Driver");
MODULE_LICENSE("GPL");

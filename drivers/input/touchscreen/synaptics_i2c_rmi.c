/* drivers/input/keyboard/synaptics_i2c_rmi.c
 *
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/synaptics_i2c_rmi.h>
#if 1 
#include <mach/gpio.h>
#endif
unsigned long polling_time = 30000000;
#if defined(CONFIG_MACH_BLADE)//P729B touchscreen enable
#define GPIO_TOUCH_EN_OUT  31
#elif defined(CONFIG_MACH_R750)//R750 touchscreen enable
#define GPIO_TOUCH_EN_OUT  33
#else//other projects
#define GPIO_TOUCH_EN_OUT  31
#endif
static struct workqueue_struct *synaptics_wq;
static struct i2c_driver synaptics_ts_driver;
#define POLL_IN_INT

struct synaptics_ts_data
{
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	struct hrtimer timer;
	struct hrtimer resume_timer;
	struct work_struct  work;
	uint16_t max[2];
	struct early_suspend early_suspend;
};
#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h);
static void synaptics_ts_late_resume(struct early_suspend *h);
#endif
static int synaptics_i2c_read(struct i2c_client *client, int reg, u8 * buf, int count)
{
    int rc;
    int ret = 0;

    buf[0] = 0xff;
	buf[1] = reg >> 8;
    rc = i2c_master_send(client, buf, 2);
    if (rc != 2)
{
        dev_err(&client->dev, "synaptics_i2c_read FAILED: failed of page select %d\n", rc);
        ret = -1;
        goto tp_i2c_rd_exit;
    }
	buf[0] = 0xff & reg;
	rc = i2c_master_send(client, buf, 1);
    if (rc != 1)
    {
        dev_err(&client->dev, "synaptics_i2c_read FAILED: read of register %d\n", reg);
        ret = -1;
        goto tp_i2c_rd_exit;
    }
    rc = i2c_master_recv(client, buf, count);
    if (rc != count)
    {
        dev_err(&client->dev, "synaptics_i2c_read FAILED: read %d bytes from reg %d\n", count, reg);
        ret = -1;
    }

  tp_i2c_rd_exit:
    return ret;
	}
static int synaptics_i2c_write(struct i2c_client *client, int reg, u8 data)
{
    u8 buf[2];
    int rc;
    int ret = 0;

    buf[0] = 0xff;
    buf[1] = reg >> 8;
    rc = i2c_master_send(client, buf, 2);
    if (rc != 2)
    {
        dev_err(&client->dev, "synaptics_i2c_write FAILED: writing to reg %d\n", reg);
        ret = -1;
    }
	buf[0] = 0xff & reg;
    buf[1] = data;
    rc = i2c_master_send(client, buf, 2);
    if (rc != 2)
    {
        dev_err(&client->dev, "synaptics_i2c_write FAILED: writing to reg %d\n", reg);
        ret = -1;
    }
	return ret;
}
static int proc_read_val(char *page, char **start,
           off_t off, int count, int *eof, void *data)
{
        int len;
        len = sprintf(page, "%lu\n", polling_time);
        return len;
}

static int proc_write_val(struct file *file, const char *buffer,
           unsigned long count, void *data)
{
		unsigned long val;
		sscanf(buffer, "%lu", &val);
		if (val >= 0) {
			polling_time= val;
			return count;
		}
		return -EINVAL;
}
static void synaptics_ts_work_func(struct work_struct *work)
{
	int ret, x, y, z, finger, w, x2, y2,w2,z2,finger2,pressure,pressure2;
	uint8_t buf[16];
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data, work);
	finger=0;
	ret = synaptics_i2c_read(ts->client, 0x0014, buf, 16);

    if (ret < 0)
    {
		printk(KERN_ERR "synaptics_ts_work_func: i2c_transfer failed\n");
    }
    else
    {

		x = (uint16_t) buf[2] << 4| (buf[4] & 0x0f) ; 
		y = (uint16_t) buf[3] << 4| ((buf[4] & 0xf0) >> 4); 
		pressure = buf[6];
		w = buf[5] >> 4;
		z = buf[5]&0x0f;
		finger = buf[1] & 0x3;

		x2 = (uint16_t) buf[7] << 4| (buf[9] & 0x0f) ;  
		y2 = (uint16_t) buf[8] << 4| ((buf[9] & 0xf0) >> 4); 
		pressure2 = buf[11]; 
		w2 = buf[10] >> 4; 
		z2 = buf[10] & 0x0f;
		finger2 = buf[1] & 0xc; 
		#ifdef CONFIG_MACH_JOE//ZTE_TS_ZT_20100520_001
		y = 2787 - y;
		y2 = 2787 - y2;
		#endif	
		if(finger)
        {   
            input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, pressure);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
            input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
			input_mt_sync(ts->input_dev);
		}
		if(finger2)

		{
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, pressure2);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x2);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y2);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w2);
			input_mt_sync(ts->input_dev);
		}
		input_sync(ts->input_dev);
		input_report_key(ts->input_dev, BTN_TOUCH, !!finger);
		input_sync(ts->input_dev);

	}
	#ifdef POLL_IN_INT
	if(finger)
		{
			hrtimer_start(&ts->timer, ktime_set(0, polling_time), HRTIMER_MODE_REL);
		}
	else
		{
			hrtimer_cancel(&ts->timer);
			enable_irq(ts->client->irq);
	}
	#else
	if (ts->use_irq)
		enable_irq(ts->client->irq);
	#endif
}

static enum hrtimer_restart synaptics_ts_timer_func(struct hrtimer *timer)
{
	struct synaptics_ts_data *ts = container_of(timer, struct synaptics_ts_data, timer);

	/* printk("synaptics_ts_timer_func\n"); */

	queue_work(synaptics_wq, &ts->work);
	#ifndef POLL_IN_INT
	hrtimer_start(&ts->timer, ktime_set(0, polling_time), HRTIMER_MODE_REL);
	#endif
	return HRTIMER_NORESTART;
}
static enum hrtimer_restart synaptics_ts_resume_func(struct hrtimer *timer)
{
	
	#if 0
	struct synaptics_ts_data *ts = container_of(timer, struct synaptics_ts_data, resume_timer);
    if (ts->use_irq)
		enable_irq(ts->client->irq);
     synaptics_i2c_write(ts->client, 0x0026, 0x07);    /* enable abs int */
	 synaptics_i2c_write(ts->client, 0x0031, 0x7F); 
	 #else
	 printk("synaptics_ts_resume_func\n");
	 #endif

	return HRTIMER_NORESTART;
}

static irqreturn_t synaptics_ts_irq_handler(int irq, void *dev_id)
{
	struct synaptics_ts_data *ts = dev_id;

	/* printk("synaptics_ts_irq_handler\n"); */
	disable_irq_nosync(ts->client->irq);
	#ifdef POLL_IN_INT
	hrtimer_start(&ts->timer, ktime_set(0, 0), HRTIMER_MODE_REL);
	#else
	queue_work(synaptics_wq, &ts->work);
	#endif
	return IRQ_HANDLED;
}

static int synaptics_ts_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	struct synaptics_ts_data *ts;
	uint8_t buf1[9];
	//struct i2c_msg msg[2];
	int ret = 0;
	uint16_t max_x, max_y;
	struct proc_dir_entry *refresh;
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
	msleep(250);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
		printk(KERN_ERR "synaptics_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	INIT_WORK(&ts->work, synaptics_ts_work_func);
	ts->client = client;
	i2c_set_clientdata(client, ts);
	client->driver = &synaptics_ts_driver;
	{
		int retry = 3;
        while (retry-- > 0)
        {

            ret = synaptics_i2c_read(ts->client, 0x0078, buf1, 9);
			printk("wly: synaptics_i2c_read, %c, %d,%d,%d,%d,%d,%d,%d,%d\n",
				buf1[0],buf1[1],buf1[2],buf1[3],buf1[4],buf1[5],buf1[6],buf1[7],buf1[8]);
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
    ret = synaptics_i2c_write(ts->client, 0x0025, 0x00); 
    ret = synaptics_i2c_read(ts->client, 0x002D, buf1, 2);
    if (ret < 0)
    {
        printk(KERN_ERR "synaptics_i2c_read failed\n");
		goto err_detect_failed;
	}
    ts->max[0] = max_x = buf1[0] | ((buf1[1] & 0x0f) << 8);
    ret = synaptics_i2c_read(ts->client, 0x002F, buf1, 2);
    if (ret < 0)
    {
        printk(KERN_ERR "synaptics_i2c_read failed\n");
		goto err_detect_failed;
	}
    ts->max[1] = max_y = buf1[0] | ((buf1[1] & 0x0f) << 8);
	printk("wly: synaptics_ts_probe,max_x=%d, max_y=%d\n", max_x, max_y);
#if defined(CONFIG_MACH_R750)
	max_y = 2739;
#endif
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "synaptics_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "synaptics-rmi-touchscreen";
	ts->input_dev->phys = "synaptics-rmi-touchscreen/input0";

	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts->input_dev->absbit);
	
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, max_x, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	ret = input_register_device(ts->input_dev);
    if (ret)
    {
		printk(KERN_ERR "synaptics_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}
   	hrtimer_init(&ts->resume_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ts->resume_timer.function = synaptics_ts_resume_func;
	#ifdef POLL_IN_INT
	hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ts->timer.function = synaptics_ts_timer_func;
	ret = request_irq(client->irq, synaptics_ts_irq_handler, IRQF_TRIGGER_LOW, "synaptics_touch", ts);
	if(ret == 0)
		{
		ret = synaptics_i2c_write(ts->client, 0x0026, 0x07);  
		if (ret)
		free_irq(client->irq, ts);
		}
	if(ret == 0)
		ts->use_irq = 1;
	else
		dev_err(&client->dev, "request_irq failed\n");
	#else
   if (0)
    {
        ret = request_irq(client->irq, synaptics_ts_irq_handler, IRQF_TRIGGER_LOW, "synaptics_touch", ts);
		if (ret == 0) {
             ret = synaptics_i2c_write(ts->client, 0x0026, 0x07);  /* enable abs int */
			if (ret)
				free_irq(client->irq, ts);
		}
		if (ret == 0)
			ts->use_irq = 1;
		else
			dev_err(&client->dev, "request_irq failed\n");
	}
	ts->use_irq = 0;
    if (!ts->use_irq)
    {
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
	#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_ts_early_suspend;
	ts->early_suspend.resume = synaptics_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
	refresh = create_proc_entry("ts_poll_freq", 0644, NULL);
	if (refresh) {
		refresh->data		= NULL;
		refresh->read_proc  = proc_read_val;
		refresh->write_proc = proc_write_val;
	}
	printk(KERN_INFO "synaptics_ts_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_detect_failed:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static int synaptics_ts_remove(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
	return 0;
}

static int synaptics_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);

	if (ts->use_irq)
		disable_irq(client->irq);
	else
		hrtimer_cancel(&ts->timer);
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq) /* if work was pending disable-count is now 2 */
		enable_irq(client->irq);
    ret = synaptics_i2c_write(ts->client, 0x0026, 0);     /* disable interrupt */
	if (ret < 0)
        printk(KERN_ERR "synaptics_ts_suspend: synaptics_i2c_write failed\n");

    ret = synaptics_i2c_write(client, 0x0025, 0x01);      /* deep sleep */
	if (ret < 0)
        printk(KERN_ERR "synaptics_ts_suspend: synaptics_i2c_write failed\n");

	return 0;
}

static int synaptics_ts_resume(struct i2c_client *client)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
    ret = synaptics_i2c_write(ts->client, 0x0025, 0x00); 
	hrtimer_start(&ts->resume_timer, ktime_set(0, 5000000), HRTIMER_MODE_REL);
	#if 1 
	if (ts->use_irq)
		enable_irq(client->irq);

	if (!ts->use_irq)
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	else
		{
        synaptics_i2c_write(ts->client, 0x0026, 0x07);    
	    	synaptics_i2c_write(ts->client, 0x0031, 0x7F); 
		}
	#endif
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_ts_late_resume(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id synaptics_ts_id[] = {
	{ SYNAPTICS_I2C_RMI_NAME, 0 },
	{ }
};

static struct i2c_driver synaptics_ts_driver = {
	.probe		= synaptics_ts_probe,
	.remove		= synaptics_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= synaptics_ts_suspend,
	.resume		= synaptics_ts_resume,
#endif
	.id_table	= synaptics_ts_id,
	.driver = {
		.name	= SYNAPTICS_I2C_RMI_NAME,
	},
};

static int __devinit synaptics_ts_init(void)
{
	synaptics_wq = create_singlethread_workqueue("synaptics_wq");
	if (!synaptics_wq)
		return -ENOMEM;
	return i2c_add_driver(&synaptics_ts_driver);
}

static void __exit synaptics_ts_exit(void)
{
	i2c_del_driver(&synaptics_ts_driver);
	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);
}

module_init(synaptics_ts_init);
module_exit(synaptics_ts_exit);

MODULE_DESCRIPTION("Synaptics Touchscreen Driver");
MODULE_LICENSE("GPL");

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
/* ========================================================================================
when         who        what, where, why                         comment tag
--------     ----       -------------------------------------    --------------------------
2010-07-29   wly         disable polling mode,only use irq       ZTE_WLY_CRDB00533288
2010-06-22   wly         config 8 bit adress                      ZTE_WLY_CRDB00512790
2010-06-10   wly         touchscreen firmware information         ZTE_WLY_CRDB00509514
2010-05-24   wly            change pressure value                     ZTE_PRESS_WLY_0524
2010-05-20   zt  	    modified the y axis for P727A1					ZTE_TS_ZT_20100520_001
2010-05-13	 zt		    modified the ts configuration for R750		ZTE_TS_ZT_20100513_002
2010-05-18   wly         config set bit                                 ZTE_SET_BIT_WLY_0518
2010-3-18    wly        add gesture and resume timer             	  ZTE_WLY_RESUME_001
2010-2-27    wly        add for limo                                  ZTE_WLY_LOCK_001
2010-02-04	 chj		protect two timer booming at the same time    ZTE_TOUCH_CHJ_010
2010-02-03	 chj		moving polling process into the interrpt      ZTE_TOUCH_CHJ_009
2010-01-19   wly        add proc interface                            ZTE_TOUCH_WLY_008
2010-01-19   wly        down cpu use                                  ZTE_TOUCH_WLY_007
2010-01-06   wly        add synaptics gesture                         ZTE_TOUCH_WLY_006
2009-12-19   wly        change synaptics driver                       ZTE_TOUCH_WLY_005
========================================================================================*/
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
#if 1 //wly
#include <mach/gpio.h>
#endif

//ZTE_TS_ZT_20100513_002 begin
#if defined(CONFIG_MACH_BLADE)//P729B touchscreen enable
#define GPIO_TOUCH_EN_OUT  31
#elif defined(CONFIG_MACH_R750)//R750 touchscreen enable
#define GPIO_TOUCH_EN_OUT  33
#else//other projects
#define GPIO_TOUCH_EN_OUT  31
#endif
//ZTE_TS_ZT_20100513_002 end

#if defined(CONFIG_MACH_R750)//ZTE_TS_ZT_20100513_002
#define TS_KEY_REPORT 
#endif

/*ZTE_TOUCH_WLY_006,@2010-01-06,begin*/
#define ABS_SINGLE_TAP	0x21	
#define ABS_TAP_HOLD	0x22	
#define ABS_DOUBLE_TAP	0x23	
#define ABS_EARLY_TAP	0x24	
#define ABS_FLICK	0x25	
#define ABS_PRESS	0x26	
#define ABS_PINCH 	0x27	
#define sigle_tap  (1 << 0)
#define tap_hold   (1 << 1)
#define double_tap (1 << 2)
#define early_tap  (1 << 3)
#define flick      (1 << 4)
#define press      (1 << 5)
#define pinch      (1 << 6)
/*ZTE_TOUCH_WLY_006,@2010-01-06,end*/
/*ZTE_TOUCH_WLY_007,@2010-01-19,begin*/
unsigned long polling_time = 30000000;
/*ZTE_TOUCH_WLY_007,@2010-01-19,end*/

static struct workqueue_struct *synaptics_wq;
static struct i2c_driver synaptics_ts_driver;
//#define swap(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)
//ZTE_TOUCH_CHJ_009,moving polling process into the interrpt,@2010-02-03,begin
#define POLL_IN_INT   
#if defined (POLL_IN_INT)
#undef POLL_IN_INT   //ZTE_WLY_CRDB00533288
#endif
//ZTE_TOUCH_CHJ_009,moving polling process into the interrpt,@2010-02-03,end

struct synaptics_ts_data
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
static void synaptics_ts_early_suspend(struct early_suspend *h);
static void synaptics_ts_late_resume(struct early_suspend *h);
#endif

#ifdef TS_KEY_REPORT//ZTE_TS_ZT_20100513_002
const char ts_keys_size_synaptics[] = "0x01:102:51:503:102:1007:0x01:139:158:503:102:1007:0x01:158:266:503:102:1007";
struct attribute ts_key_report_attr_synaptics = {
        .name = "virtualkeys.synaptics-rmi-touchscreen",
        .mode = S_IRWXUGO,
};
 
static struct attribute *def_attrs_synaptics[] = {
        &ts_key_report_attr_synaptics,
        NULL,
};
 
void ts_key_report_synaptics_release(struct kobject *kobject)
{
        return;
}
 
ssize_t ts_key_report_synaptics_show(struct kobject *kobject, struct attribute *attr,char *buf)
{
        sprintf(buf,"%s\n",ts_keys_size_synaptics);
        return strlen(ts_keys_size_synaptics)+2;
}
 
ssize_t ts_key_report_synaptics_store(struct kobject *kobject,struct attribute *attr,const char *buf, size_t count)
{
        return count;
}
 
struct sysfs_ops ts_key_report_sysops_synaptics =
{
        .show = ts_key_report_synaptics_show,
        .store = ts_key_report_synaptics_store,
};
 
struct kobj_type ktype_synaptics = 
{
        .release = ts_key_report_synaptics_release,
        .sysfs_ops=&ts_key_report_sysops_synaptics,
        .default_attrs=def_attrs_synaptics,
};
 
struct kobject kobj_synaptics;
static void ts_key_report_synaptics_init(void)
{
	int ret = 0;
        ret = kobject_init_and_add(&kobj_synaptics,&ktype_synaptics,NULL,"board_properties");
	if(ret)
		printk(KERN_ERR "ts_key_report_init: Unable to init and add the kobject\n");
}
#endif
#if 1 //ZTE_WLY_CRDB00512790,BEGIN
static int synaptics_i2c_read(struct i2c_client *client, int reg, u8 * buf, int count)
{
    int rc;
    int ret = 0;

    buf[0] = reg;
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

    buf[0] = reg;
    buf[1] = data;
    rc = i2c_master_send(client, buf, 2);
    if (rc != 2)
    {
        dev_err(&client->dev, "synaptics_i2c_write FAILED: writing to reg %d\n", reg);
        ret = -1;
    }
    return ret;
}
#else

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
#endif  ////ZTE_WLY_CRDB00512790,END

static int
proc_read_val(char *page, char **start, off_t off, int count, int *eof,
	  void *data)
{
	int len = 0;
	len += sprintf(page + len, "%s\n", "touchscreen module");
	len += sprintf(page + len, "name     : %s\n", "synaptics");
	#if defined(CONFIG_MACH_R750)
	len += sprintf(page + len, "i2c address  : %x\n", 0x23);
	#else
	len += sprintf(page + len, "i2c address  : 0x%x\n", 0x22);
	#endif
	len += sprintf(page + len, "IC type    : %s\n", "2000 series");
	#if defined(CONFIG_MACH_R750)
	len += sprintf(page + len, "firmware version    : %s\n", "TM1551");
	#elif  defined(CONFIG_MACH_JOE)
	len += sprintf(page + len, "firmware version    : %s\n", "TM1419-001");
	#elif  defined(CONFIG_MACH_BLADE)
	len += sprintf(page + len, "firmware version    : %s\n", "TM1541");
	#endif
	len += sprintf(page + len, "module : %s\n", "synaptics + TPK");
	if (off + count >= len)
		*eof = 1;
	if (len < off)
		return 0;
	*start = page + off;
	return ((count < len - off) ? count : len - off);
}
//ZTE_WLY_CRDB00509514,END

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
/*ZTE_TOUCH_WLY_008,@2010-01-19,end*/
static void synaptics_ts_work_func(struct work_struct *work)
{
  /*ZTE_TOUCH_WLY_007,@2010-01-19,begin*/
	int ret, x, y, z, finger, w, x2, y2,w2,z2,finger2,pressure,pressure2;
  /*ZTE_TOUCH_WLY_006,@2010-01-06,begin*/
	__s8  gesture, flick_y, flick_x, direction = 0;  
	uint8_t buf[16];
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data, work);
	finger=0;//initializing the status
	ret = synaptics_i2c_read(ts->client, 0x14, buf, 16);  //ZTE_WLY_CRDB00512790
	if (ret < 0){
   	printk(KERN_ERR "synaptics_ts_work_func: synaptics_i2c_write failed, go to poweroff.\n");
    gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
    msleep(200);
    gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
    msleep(200);
  }
  else
  {
			/*printk(KERN_WARNING "synaptics_ts_work_func:"
			"%x %x %x %x %x %x %x %x %x"
					       " %x %x %x %x %x %x, ret %d\n",
					       buf[0], buf[1], buf[2], buf[3],
					       buf[4], buf[5], buf[6], buf[7],
	        buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], ret);*/
			x = (uint16_t) buf[2] << 4| (buf[4] & 0x0f) ; 
			y = (uint16_t) buf[3] << 4| ((buf[4] & 0xf0) >> 4); 
			pressure = buf[6];
			w = buf[5] >> 4;
			z = buf[5]&0x0f;
			finger = buf[1] & 0x3;
	
			x2 = (uint16_t) buf[7] << 4| (buf[9] & 0x0f) ;  
			y2 = (uint16_t) buf[8] << 4| ((buf[9] & 0xf0) >> 4); 

	#ifdef CONFIG_MACH_JOE//ZTE_TS_ZT_20100520_001
			y = 2787 - y;
			y2 = 2787 - y2;
	#endif		
	
			pressure2 = buf[11]; 
			w2 = buf[10] >> 4; 
			z2 = buf[10] & 0x0f;
	        /*ZTE_TOUCH_WLY_006,@2010-01-06,begin*/
			finger2 = buf[1] & 0xc; 
			//printk("wly: finger=%d, finger2=%d, buf[1]=%d\n", finger, finger2, buf[1]);
			gesture = buf[12];
	
			flick_x = buf[14];
			flick_y = buf[15];
			//printk("wly: gesture=%d,flick_x=%d,flick_y=%d\n",gesture,flick_x,flick_y);
			if((16==gesture)||(flick_x)||(flick_y))
			{
				if ((flick_x >0 )&& (abs(flick_x) > abs(flick_y))) 
				direction = 1;
				else if((flick_x <0 )&& (abs(flick_x) > abs(flick_y)))  
				direction = 2;
				else if ((flick_y >0 )&& (abs(flick_x) < abs(flick_y))) 
				direction = 3;
	
				else if ((flick_y <0 )&& (abs(flick_x) < abs(flick_y))) 
				direction = 4;

			}
			if(finger)
      {   
	      input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 255);//ZTE_PRESS_WLY_0524
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
	      input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 10);//ZTE_PRESS_WLY_0524
				input_mt_sync(ts->input_dev);

			}
			if(finger2)
			{
				input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 255);//ZTE_PRESS_WLY_0524
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x2);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y2);
				input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 10);//ZTE_PRESS_WLY_0524
				input_mt_sync(ts->input_dev);
			}
		input_sync(ts->input_dev);
		input_report_abs(ts->input_dev, ABS_PINCH, flick_x);
		input_report_key(ts->input_dev, BTN_TOUCH, !!finger);
		input_sync(ts->input_dev);
	}
		/*ZTE_WLY_LOCK_001,2009-12-07 END*/
	
		//ZTE_TOUCH_CHJ_010,protect two timer booming in the same time,@2010-02-04,begin
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
		//ZTE_TOUCH_CHJ_010,protect two timer booming in the same time,@2010-02-04,end
}

static enum hrtimer_restart synaptics_ts_timer_func(struct hrtimer *timer)
{
	struct synaptics_ts_data *ts = container_of(timer, struct synaptics_ts_data, timer);

	/* printk("synaptics_ts_timer_func\n"); */

	queue_work(synaptics_wq, &ts->work);
	//ZTE_TOUCH_CHJ_010,protect two timer booming in the same time,@2010-02-04,begin
	#ifndef POLL_IN_INT
	/*ZTE_TOUCH_WLY_007,@2010-01-19,begin*/
	hrtimer_start(&ts->timer, ktime_set(0, polling_time), HRTIMER_MODE_REL);
	/*ZTE_TOUCH_WLY_007,@2010-01-19,end*/
	#endif
	//ZTE_TOUCH_CHJ_010,protect two timer booming in the same time,@2010-02-04,end
	return HRTIMER_NORESTART;
}


static irqreturn_t synaptics_ts_irq_handler(int irq, void *dev_id)
{
	struct synaptics_ts_data *ts = dev_id;

	/* printk("synaptics_ts_irq_handler\n"); */
	disable_irq_nosync(ts->client->irq);
	//ZTE_TOUCH_CHJ_009,moving polling process into the interrpt,@2010-02-03,begin
	#ifdef POLL_IN_INT
	hrtimer_start(&ts->timer, ktime_set(0, 0), HRTIMER_MODE_REL);
	#else
	queue_work(synaptics_wq, &ts->work);
	#endif
	//ZTE_TOUCH_CHJ_009,moving polling process into the interrpt,@2010-02-03,end
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
	/*ZTE_TOUCH_WLY_008,@2010-01-19,begin*/
	struct proc_dir_entry *dir, *refresh;//ZTE_WLY_CRDB00509514
   /*ZTE_TOUCH_WLY_008,@2010-01-19,end*/
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
	//pdata = client->dev.platform_data;
    //printk("wly:%s, ts->client->addr=%x\n", __FUNCTION__, ts->client->addr);
	{
		int retry = 3;
        while (retry-- > 0)
        {

            ret = synaptics_i2c_read(ts->client, 0x78, buf1, 9);//ZTE_WLY_CRDB00512790,BEGIN
			printk("wly: synaptics_i2c_read, %c, %d,%d,%d,%d,%d,%d,%d,%d\n",
				buf1[0],buf1[1],buf1[2],buf1[3],buf1[4],buf1[5],buf1[6],buf1[7],buf1[8]);
		//ZTE_TOUCH_WLY_009,2010-05-10, BEGIN
			if (ret >= 0)
				break;
			msleep(10);
//ZTE_TOUCH_WLY_009,2010-05-10, END

	}
		/*ZTE_TOUCH_WLY_005,@2009-12-19,begin*/
		if (retry < 0)
			{
			ret = -1;
		goto err_detect_failed;
	}
		/*ZTE_TOUCH_WLY_005,@2009-12-19,begin*/
	}
//ZTE_WLY_CRDB00512790,BEGIN
    ret = synaptics_i2c_write(ts->client, 0x25, 0x00); /*wly set nomal operation*/
    ret = synaptics_i2c_read(ts->client, 0x2D, buf1, 2);
//ZTE_WLY_CRDB00512790,END
    if (ret < 0)
    {
        printk(KERN_ERR "synaptics_i2c_read failed\n");
		goto err_detect_failed;
	}
    ts->max[0] = max_x = buf1[0] | ((buf1[1] & 0x0f) << 8);
    ret = synaptics_i2c_read(ts->client, 0x2F, buf1, 2); //ZTE_WLY_CRDB00512790
    if (ret < 0)
    {
        printk(KERN_ERR "synaptics_i2c_read failed\n");
		goto err_detect_failed;
	}
    ts->max[1] = max_y = buf1[0] | ((buf1[1] & 0x0f) << 8);
	printk("wly: synaptics_ts_probe,max_x=%d, max_y=%d\n", max_x, max_y);
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
	//ZTE_SET_BIT_WLY_0518,BEGIN
	set_bit(ABS_SINGLE_TAP, ts->input_dev->absbit);
	set_bit(ABS_TAP_HOLD, ts->input_dev->absbit);
	set_bit(ABS_EARLY_TAP, ts->input_dev->absbit);
	set_bit(ABS_FLICK, ts->input_dev->absbit);
	set_bit(ABS_PRESS, ts->input_dev->absbit);
	set_bit(ABS_DOUBLE_TAP, ts->input_dev->absbit);
	set_bit(ABS_PINCH, ts->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts->input_dev->absbit);

#ifdef TS_KEY_REPORT//ZTE_TS_ZT_20100513_002
	max_y = 2739;
#endif
	
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, max_x, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_SINGLE_TAP, 0, 5, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_TAP_HOLD, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_EARLY_TAP, 0, 5, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_FLICK, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESS, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_DOUBLE_TAP, 0, 5, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_PINCH, -255, 255, 0, 0);
	ret = input_register_device(ts->input_dev);
    if (ret)
    {
		printk(KERN_ERR "synaptics_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}
	#ifdef POLL_IN_INT
	hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ts->timer.function = synaptics_ts_timer_func;
	ret = request_irq(client->irq, synaptics_ts_irq_handler, IRQF_TRIGGER_FALLING, "synaptics_touch", ts);
	if(ret == 0)
		{
		ret = synaptics_i2c_write(ts->client, 0x26, 0x07);  /* enable abs int ZTE_WLY_CRDB00512790*/
		if (ret)
		free_irq(client->irq, ts);
		}
	if(ret == 0)
		ts->use_irq = 1;
	else
		dev_err(&client->dev, "request_irq failed\n");
	#else
	//ZTE_TOUCH_CHJ_009,moving polling process into the interrpt,@2010-02-03,end
    /*ZTE_TOUCH_WLY_007,@2010-01-19,begin*/
   if (client->irq)
    {
        ret = request_irq(client->irq, synaptics_ts_irq_handler, IRQF_TRIGGER_FALLING, "synaptics_touch", ts);
		if (ret == 0) {
    ret = synaptics_i2c_write(ts->client, 0x26, 0x07);  /* enable abs int,ZTE_WLY_CRDB00512790 */
			if (ret)
				free_irq(client->irq, ts);
		}
		if (ret == 0)
			ts->use_irq = 1;
		else
			dev_err(&client->dev, "request_irq failed\n");
	}
	/*ZTE_TOUCH_WLY_007,@2010-01-19,end*/
    if (!ts->use_irq)
    {
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
	//ZTE_TOUCH_CHJ_009,moving polling process into the interrpt,@2010-02-03,begin
	#endif
	//ZTE_TOUCH_CHJ_009,moving polling process into the interrpt,@2010-02-03,end
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_ts_early_suspend;
	ts->early_suspend.resume = synaptics_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
/*ZTE_TOUCH_WLY_008,@2010-01-19,begin*/
//ZTE_WLY_CRDB00509514,BEGIN
  dir = proc_mkdir("touchscreen", NULL);
	refresh = create_proc_entry("ts_information", 0644, dir);
//ZTE_WLY_CRDB00509514,END
	if (refresh) {
		refresh->data		= NULL;
		refresh->read_proc  = proc_read_val;
		refresh->write_proc = proc_write_val;
	}
/*ZTE_TOUCH_WLY_008,@2010-01-19,end*/
	printk(KERN_INFO "synaptics_ts_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

#ifdef TS_KEY_REPORT//ZTE_TS_ZT_20100513_002
	ts_key_report_synaptics_init();
#endif

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_detect_failed:
//err_power_failed:
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
	/*ZTE_WLY_RESUME_001,2010-3-18 START*/
	//hrtimer_cancel(&ts->timer);  //wly
	//hrtimer_cancel(&ts->resume_timer);
	/*ZTE_WLY_RESUME_001,2010-3-18 END*/
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq) /* if work was pending disable-count is now 2 */
		enable_irq(client->irq);
    ret = synaptics_i2c_write(ts->client, 0x26, 0);     /* disable interrupt,ZTE_WLY_CRDB00512790 */
	if (ret < 0)
        printk(KERN_ERR "synaptics_ts_suspend: synaptics_i2c_write failed\n");

    ret = synaptics_i2c_write(client, 0x25, 0x01);      /* deep sleep *//*wly value need change, ZTE_WLY_CRDB00512790*/
	if (ret < 0)
        printk(KERN_ERR "synaptics_ts_suspend: synaptics_i2c_write failed\n");
	//gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);

	return 0;
}

static int synaptics_ts_resume(struct i2c_client *client)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
    ret = synaptics_i2c_write(ts->client, 0x25, 0x00); /*wly set nomal operation,ZTE_WLY_CRDB00512790*/
	if (ts->use_irq)
		enable_irq(client->irq);

	if (!ts->use_irq)
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	else
		{
        synaptics_i2c_write(ts->client, 0x26, 0x07);    /* enable abs int,ZTE_WLY_CRDB00512790 */
	    	synaptics_i2c_write(ts->client, 0x31, 0x7F); /*wly set 2D gesture enable,ZTE_WLY_CRDB00512790*/
		}
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
	synaptics_wq = create_rt_workqueue("synaptics_wq");
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

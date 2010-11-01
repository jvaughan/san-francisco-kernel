
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
#include <mach/jogball_key.h>


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Android Jogball KEY driver");

struct jogball_driver_data {
       struct input_dev  *jogball_input_dev;
	int gpio_irq_up;
	int gpio_irq_down;
	int gpio_irq_left;
	int gpio_irq_right;
};

#define GPIO_JOGBALL_UP_INT  MSM_GPIO_TO_INT(30)
#define GPIO_JOGBALL_DOWN_INT  MSM_GPIO_TO_INT(38)
#define GPIO_JOGBALL_LEFT_INT  MSM_GPIO_TO_INT(76)
#define GPIO_JOGBALL_RIGHT_INT  MSM_GPIO_TO_INT(88)


#define MAX_NUM 6

static irqreturn_t jogball_key_interrupt(int irq, void *dev_id)
{      
       static int num_up_irq = 0;
	static int num_down_irq = 0;
	static int num_left_irq = 0;
	static int num_right_irq = 0;

	bool report_key = 0; 
       
	struct jogball_driver_data *dd_jogball = dev_id;

	struct input_dev *jogball_input_dev = dd_jogball->jogball_input_dev;

	switch (irq) {
	case GPIO_JOGBALL_UP_INT:
		num_up_irq+=1;
		break;
	case GPIO_JOGBALL_DOWN_INT:
		num_down_irq+=1;
		break;
	case GPIO_JOGBALL_LEFT_INT:
		num_left_irq+=1;
		break;
	case GPIO_JOGBALL_RIGHT_INT:
		num_right_irq+=1;
		break;
	default:
		break;
	}

	
	if((MAX_NUM == num_up_irq)&&(0 == report_key))
	{    
	      report_key = 1;
        input_report_key(jogball_input_dev, KEY_UP, 1);
	      input_report_key(jogball_input_dev, KEY_UP, 0);
		  
	}
	else if((MAX_NUM == num_down_irq)&&(0 == report_key))
	{    
	      report_key = 1;
        input_report_key(jogball_input_dev, KEY_DOWN, 1);
	      input_report_key(jogball_input_dev, KEY_DOWN, 0);
   
	}
	else if((MAX_NUM == num_left_irq)&&(0 == report_key))
	{    
	      report_key = 1;
        input_report_key(jogball_input_dev, KEY_LEFT, 1);
	      input_report_key(jogball_input_dev, KEY_LEFT, 0);
		  
	}
	else if((MAX_NUM == num_right_irq)&&(0 == report_key))
	{   
	     report_key = 1;
       input_report_key(jogball_input_dev, KEY_RIGHT, 1);
	     input_report_key(jogball_input_dev, KEY_RIGHT, 0);
		 
	}

	if(report_key) 
	{
	   num_up_irq = 0;
	   num_down_irq = 0;
	   num_left_irq = 0;
	   num_right_irq = 0;
	   report_key = 0;
	}


	return IRQ_HANDLED;
	 
}


static int __devinit jogball_key_probe(struct platform_device *pdev)
{
  int result;
	struct input_dev *input_dev;
	struct jogball_driver_data *dd_jogball;
	struct jogball_key_platform_data *pd_jogball = pdev->dev.platform_data;
  dd_jogball = kzalloc(sizeof(struct jogball_driver_data), GFP_KERNEL);
 
	input_dev = input_allocate_device();

	if (!input_dev || !dd_jogball) {
	      result = -ENOMEM;
	      goto fail_alloc_mem;
	}
	
      platform_set_drvdata(pdev,dd_jogball);

      dd_jogball->gpio_irq_up = pd_jogball->gpio_irq_up;
      dd_jogball->gpio_irq_down = pd_jogball->gpio_irq_down;
      dd_jogball->gpio_irq_left = pd_jogball->gpio_irq_left;
      dd_jogball->gpio_irq_right = pd_jogball->gpio_irq_right;

      input_dev->name = "jogball_key";

      input_set_capability(input_dev,EV_KEY,KEY_UP);
      input_set_capability(input_dev,EV_KEY,KEY_DOWN);
      input_set_capability(input_dev,EV_KEY,KEY_LEFT);
      input_set_capability(input_dev,EV_KEY,KEY_RIGHT);

      result = input_register_device(input_dev);
		
	if (result) {
             goto fail_ip_reg;
	}

	dd_jogball->jogball_input_dev = input_dev;

       result = request_irq(GPIO_JOGBALL_UP_INT, jogball_key_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"jogball_key", dd_jogball); 
       if (result)
		goto fail_req_irq;
       
       result = request_irq(GPIO_JOGBALL_DOWN_INT, jogball_key_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"jogball_key", dd_jogball); 
       if (result)
		goto fail_req_irq;
       
       result = request_irq(GPIO_JOGBALL_LEFT_INT, jogball_key_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"jogball_key", dd_jogball); 
       if (result)
		goto fail_req_irq;
       
       result = request_irq(GPIO_JOGBALL_RIGHT_INT, jogball_key_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"jogball_key", dd_jogball); 
       if (result)
		goto fail_req_irq;


	return 0;
	
fail_req_irq:
	
fail_ip_reg:
	      input_unregister_device(input_dev);
	      input_dev = NULL;
fail_alloc_mem:
        input_free_device(input_dev);
	      kfree(dd_jogball);
	      return result;

}

static int __devexit jogball_key_remove(struct platform_device *pdev)
{
	struct jogball_driver_data *dd_jogball = platform_get_drvdata(pdev);

	free_irq(GPIO_JOGBALL_UP_INT, dd_jogball);
	free_irq(GPIO_JOGBALL_DOWN_INT, dd_jogball);
	free_irq(GPIO_JOGBALL_LEFT_INT, dd_jogball);
	free_irq(GPIO_JOGBALL_RIGHT_INT, dd_jogball);

	input_unregister_device(dd_jogball->jogball_input_dev);

	platform_set_drvdata(pdev, NULL);
	
	kfree(dd_jogball);

	return 0;
}

static struct platform_driver jogball_key_driver = {
	.probe		= jogball_key_probe,
	.remove		= __devexit_p(jogball_key_remove),
	.driver		= {
		.name = "jogball_key",
		.owner = THIS_MODULE,
	},
};

static int __init jogball_key_init(void)
{      
       return platform_driver_register(&jogball_key_driver);
}

static void __exit jogball_key_exit(void) 
{
	platform_driver_unregister(&jogball_key_driver);
}

module_init(jogball_key_init);
module_exit(jogball_key_exit);


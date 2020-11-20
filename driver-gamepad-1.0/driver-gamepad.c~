/*
 * This is a demo Linux kernel module.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/signal.h>

#include <stdbool.h>
#include "efm32gg.h"

#define irq_even 	17
#define irq_odd  	18

#define CLASS_NAME "gamepad"

dev_t DEV_ID;
char DEV_NAME[] = "gamepad";

struct class *cl;	
struct fasync_struct* async;
struct cdev my_cdev;
void  __iomem *mem_reg_GPIO_PORT_C, *mem_reg_GPIO_INT;
char button_status_cache = 0, *button_cache_ptr;

int Setup_GPIO(void){
	printk(KERN_INFO "Start GPIO_setup");
	if(request_mem_region(GPIO_PC_BASE, 0x0020, DEV_NAME) == NULL)
		return -1; //error
		
	if(request_mem_region(GPIO_INTERRUPT_BASE, 0x0020, DEV_NAME) == NULL)
		return -2; //error

	mem_reg_GPIO_PORT_C = ioremap_nocache(GPIO_PC_BASE, 0x0020);
	if(mem_reg_GPIO_PORT_C == 0){
		return -3;} // fail to remap memory
		
	mem_reg_GPIO_INT = ioremap_nocache(GPIO_INTERRUPT_BASE, 0x1c);
	if(mem_reg_GPIO_INT == 0){
		return -4;} // failed to remap memory
	
	/*********configure buttons***************/
	iowrite32(0x33333333, mem_reg_GPIO_PORT_C + MODEL);//Set pins 0-7 to input
	iowrite32(0xff , mem_reg_GPIO_PORT_C + DOUT);
	
	/*********configure interrupt*************/
	iowrite32(0x22222222, mem_reg_GPIO_INT + 0);	  //GPIO_EXTIPSELL
	iowrite32(0xff,		  mem_reg_GPIO_INT+ 0x0c);//GPIO_EXTIFALL
	iowrite32(0xff,		  mem_reg_GPIO_INT+ 0x10);//GPIO_IEN
	//printk(KERN_INFO "it has gone through setup");

	
	
return 0;

}


static irq_handler_t Button_handler(int irq, void *dev_id, struct pt_regs *regs) {
	
	iowrite8(0xff,mem_reg_GPIO_INT + 0x1c); // Clear the interrupt flag
	if(async){
		button_status_cache = ~ioread8(mem_reg_GPIO_PORT_C + DIN);
		button_cache_ptr = &button_status_cache;
		kill_fasync(&async, SIGIO, POLL_IN);
		}
	printk(KERN_INFO "button %d pushed! from driver! \n",button_status_cache);
	return (irq_handler_t) IRQ_HANDLED;
}


/*
 * template_init - function to insert this module into kernel space
 *
 * This is the first of two exported functions to handle inserting this
 * code into a running kernel
 *
 * Returns 0 if successfull, otherwise -1
 */
static int my_open(struct inode *inode, struct file *filp){
	nonseekable_open(inode,filp);
	return 0;
}

static int my_release(struct inode *inode, struct file *filp){
	return 0;
}


static ssize_t my_read(struct file *filp, char __user *buff, size_t count, loff_t *offp){
	uint8_t button_status;
	button_status = ~ioread8(mem_reg_GPIO_PORT_C + DIN);
	if(button_cache_ptr != NULL){
		button_status = button_status_cache;
		button_cache_ptr = NULL;
	}	
	put_user((char)button_status, buff);
	
	
	
	return 0;
}

static ssize_t my_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp){
	
	
	return 0;
}

static int fasync(int inode_num, struct file *filp, int mode){
	return fasync_helper(inode_num, filp, mode, &async);	
}

static struct file_operations my_fops = {
.owner 		= THIS_MODULE,
.read 		= my_read,
.write 		= my_write,
.open 		= my_open,
.release 	= my_release,
.fasync 	= fasync
};

static int __init template_init(void)
{
	printk("Hello World, here is your module speaking\n");	
	
	if(alloc_chrdev_region(&DEV_ID, 0, 1, DEV_NAME) < 0) goto fail_alloc;
	
	cdev_init(&my_cdev,&my_fops);
	
	cl = class_create(THIS_MODULE, CLASS_NAME);
	device_create(cl, NULL, DEV_ID, NULL, CLASS_NAME);
		
		if(request_irq(irq_odd, (irq_handler_t) Button_handler, 0, DEV_NAME,NULL) != 0) goto Fail_irq;
		if(request_irq(irq_even, (irq_handler_t) Button_handler, 0, DEV_NAME,NULL) != 0) goto Fail_irq;
		
		
	switch(Setup_GPIO()){
		case -1:
			printk("can't get access to GPIO_PC register");
			goto fail_GPIO_alloc;
		case -2:
			printk("can't get access to GPIO_IFC register");
			goto fail_GPIO_alloc;
		case -3:
			goto fail_GPIO_alloc;
		case -4:
			goto fail_GPIO_alloc;
		default:
			printk(KERN_INFO "Ended up in default case! \n");
			printk(KERN_INFO "GPIO setup successfull! \n");
		}
	
	printk("this is awsome!!!!!!!");
	
	if(cdev_add(&my_cdev, DEV_ID, 1) < 0) goto fail_cdev;
	
	
	return 0;
	//fail hadling:
	fail_GPIO_alloc:
		printk(KERN_ERR "failed to allocate GPIO");
		release_mem_region(GPIO_PC_BASE, 0x020);
		release_mem_region(GPIO_INTERRUPT_BASE, 0x20);
		iounmap(mem_reg_GPIO_PORT_C);
		iounmap(mem_reg_GPIO_INT);
		
	
	fail_alloc:
		printk(KERN_ERR "something failed");
		
	fail_cdev:
		printk(KERN_ERR "Something went wrong with CDEV");
	Fail_irq:
		free_irq(irq_even, NULL);
	 	free_irq(irq_odd, NULL);
	 	printk("IRQ failure");
		
	return -1;
}

/***********************************************************************************/




/*
 * template_cleanup - function to cleanup this module from kernel space
 *
 * This is the second of two exported functions to handle cleanup this
 * code from a running kernel
 */

static void __exit template_cleanup(void)
{
	 printk("Short life for a small module...\n");
	 //closes all opend files etc.
	 cdev_del(&my_cdev);
	 /****GPIO_CLEANUP*****/
	 release_mem_region(GPIO_PC_BASE, 0x020);
	 release_mem_region(GPIO_INTERRUPT_BASE, 0x1c);
	 iounmap(mem_reg_GPIO_PORT_C);
	 iounmap(mem_reg_GPIO_INT);
	 free_irq(irq_even, NULL);
	 free_irq(irq_odd, NULL);
	 
	 class_destroy(cl);
	 
	 
}

module_init(template_init);
module_exit(template_cleanup);

MODULE_DESCRIPTION("Small module, demo only, not very useful.");
MODULE_LICENSE("GPL");


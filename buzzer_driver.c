/*comp3438_buzzer

 *

 * This program is free software; you can redistribute it and/or modify

 * it under the terms of the GNU General Public License version 2 as

 * published by the Free Software Foundation.

 */

#include <linux/fs.h>

#include <linux/types.h>

#include <linux/moduleparam.h>

#include <linux/ioctl.h>

#include <linux/cdev.h>


#include <asm/uaccess.h>


#include <linux/module.h>

#include <linux/kernel.h>

#include <linux/init.h>

#include <linux/platform_device.h>

#include <linux/fb.h>

#include <linux/backlight.h>

#include <linux/err.h>

#include <linux/pwm.h>

#include <linux/slab.h>

#include <linux/miscdevice.h>

#include <linux/delay.h>


#include <mach/gpio.h>

#include <mach/regs-gpio.h>

#include <plat/gpio-cfg.h>

#include <plat/regs-timer.h>

#include <linux/clk.h>


#
define DEVICE_NAME "buzzer"

# define N_D 1 /*Number of Devices*/

# define S_N 1 /*The start minor number*/

# define GPD0_CON_ADDR 0xE02000A0

# define GPD0_CON_DATA 0Xe02000A4

static void * ZL_GPD0_CON_ADDR;

static void * ZL_GPD0_CON_DATA;

//PWM

#
define PWM_TCFG0 0xE2500000

# define PWM_TCFG1 0xE2500004

# define PWM_TCNTB0 0xE250000C

# define PWM_TCMPB0 0xE2500010

# define PWM_CON 0xE2500008

static void * ZL_PWM_TCFG0;

static void * ZL_PWM_TCFG1;

static void * ZL_PWM_TCNTB0;

static void * ZL_PWM_TCMPB0;

static void * ZL_PWM_CON;

//static struct semaphore lock;

static int major;

static dev_t devno;

static struct cdev dev_buzzer;

void buzzer_start(void) //start the buzzer  

{

    unsigned int data;

    data = readl(ZL_GPD0_CON_DATA); //  

    data |= 0x01; // set data as high   

    writel(data, ZL_GPD0_CON_DATA);
    printk("ZL_GPD0_CON_DATA: %i\n", data);

}

void buzzer_stop(void) //stop the buzzer   

{
    unsigned int data;
    data = readl(ZL_GPD0_CON_DATA); //  
    data &= ~0x01; // set data as low
    writel(data, ZL_GPD0_CON_DATA);
    printk("ZL_GPD0_CON_DATA: %i\n", data);

}

void pwm_stop() {
    int data;
    data = readl(ZL_PWM_CON);
    data &= ~(1 << 0);
    writel(data, ZL_PWM_CON);
    printk("ZL_PWM_CON: %i\n", data);
}

void pwm_start() {
    int data;
    data = readl(ZL_PWM_CON);
    data |= (1 << 0);
    writel(data, ZL_PWM_CON);
    printk("ZL_PWM_CON: %i\n", data);

}

//printk("\n");

void set_pwm(int input) {
    int data1;
    int data2;
    pwm_stop();
    data1 = readl(ZL_PWM_TCNTB0);
    data1 = input;
    writel(data1, ZL_PWM_TCNTB0);

    data2 = readl(ZL_PWM_TCMPB0);

    data2 = data1 / 2;

    writel(data2, ZL_PWM_TCMPB0);

    printk("ZL_PWM_TCNTB0: %i, ZL_PWM_TCMPB0: %i\n", data1, data2);
    pwm_start();

}

void pwm_init(void) {

    unsigned int data;

    unsigned long pclk;

    unsigned long pre_div;

    unsigned long timer_cnt;

    struct clk * clk_p;

    clk_p = clk_get(NULL, "pclk");

    pclk = clk_get_rate(clk_p);

    //printk("PCLK: %lu\n", pclk);

    //PRE/DIV Feq = PCLK / (PRESCALER value + 1) / (DIVIDER value)

    pre_div = pclk / (65 + 1) / 16;

    //printk("pre_div: %lu\n", pre_div);

    timer_cnt = (1 / 265) / (1 / pre_div);

    //printk("timer_cnt: %lu\n", timer_cnt);

    ZL_PWM_TCFG0 = ioremap(PWM_TCFG0, 0x00000006);

    ZL_PWM_TCFG1 = ioremap(PWM_TCFG1, 0x00000007);

    ZL_PWM_TCNTB0 = ioremap(PWM_TCNTB0, 0x00000008);

    ZL_PWM_TCMPB0 = ioremap(PWM_TCMPB0, 0x00000009);

    ZL_PWM_CON = ioremap(PWM_CON, 0x00000010);

    data = readl(ZL_PWM_TCFG0);
    data |= 65;
    writel(data, ZL_PWM_TCFG0);
    printk("ZL_PWM_TCFG0: %i\n", data);

    data = readl(ZL_PWM_TCFG1);
    data |= (1 << 2);
    writel(data, ZL_PWM_TCFG1);
    printk("ZL_PWM_TCFG1: %i\n", data);

    data = readl(ZL_PWM_CON);

    data &= ~(1 << 4);

    data |= (1 << 3);

    data &= ~(1 << 2);

    data &= ~(1 << 1);

    data &= ~(1 << 0);

    writel(data, ZL_PWM_CON);
    printk("ZL_PWM_CON: %i\n", data);

}

static int zili_demo_char_buzzer_open(struct inode * inode, struct file * file) {

    unsigned int data;

    //map the IO physical memory address to virtual address

    ZL_GPD0_CON_ADDR = ioremap(GPD0_CON_ADDR, 0x00000004);

    ZL_GPD0_CON_DATA = ioremap(GPD0_CON_DATA, 0x00000005);
    //configure the GPIO work as output, set the last four bits of the register as 0010
    data = readl(ZL_GPD0_CON_ADDR);
    data &= (~0x01 << 1);
    data &= (~0x01 << 2);
    data &= (~0x01 << 3);
    data |= (1 << 1);
    writel(data, ZL_GPD0_CON_ADDR);
    printk("ZL_GPD0_CON_ADDR: %i\n", data);

    pwm_init();

    printk("Device "
        DEVICE_NAME " open.\n");

    return 0;

}

static int zili_demo_char_buzzer_close(struct inode * inode, struct file * file) {

    return 0;

}

static ssize_t zili_demo_char_buzzer_write(struct file * fp,
    const char * buf, size_t count, loff_t * position)

{

    int buzzer_status;

    int ret;

    ret = copy_from_user( & buzzer_status, buf, sizeof(buzzer_status));

    if (ret)

    {

        printk("Fail to copy data from the user space to the kernel space!\n");

    }

    if (buzzer_status > 0)

    {

        set_pwm(buzzer_status);

    } else

    {

        pwm_stop();

    }

    return sizeof(buzzer_status);

}

static struct file_operations zili_mini210_pwm_ops = {

    .owner = THIS_MODULE,

    .open = zili_demo_char_buzzer_open,

    .release = zili_demo_char_buzzer_close,

    .write = zili_demo_char_buzzer_write,

};

static int __init zili_demo_char_buzzer_dev_init(void) {

    int ret;

    /*Register a major number*/

    ret = alloc_chrdev_region( & devno, S_N, N_D, DEVICE_NAME);

    if (ret < 0)

    {

        printk("Device "
            DEVICE_NAME " cannot get major number.\n");

        return ret;

    }

    major = MAJOR(devno);

    printk("Device "
        DEVICE_NAME " initialized (Major Number -- %d).\n", major);

    /*Register a char device*/

    cdev_init( & dev_buzzer, & zili_mini210_pwm_ops);

    dev_buzzer.owner = THIS_MODULE;

    dev_buzzer.ops = & zili_mini210_pwm_ops;

    ret = cdev_add( & dev_buzzer, devno, N_D);

    if (ret)

    {

        printk("Device "
            DEVICE_NAME " register fail.\n");

        return ret;

    }

    return 0;

}

static void __exit zili_demo_char_buzzer_dev_exit(void) {

    buzzer_stop();

    cdev_del( & dev_buzzer);

    unregister_chrdev_region(devno, N_D);

    printk("Device "
        DEVICE_NAME " unloaded.\n");

}

module_init(zili_demo_char_buzzer_dev_init);

module_exit(zili_demo_char_buzzer_dev_exit);

MODULE_LICENSE("GPL");

MODULE_AUTHOR("FriendlyARM Inc.");

MODULE_DESCRIPTION("S5PV210 Buzzer Driver");
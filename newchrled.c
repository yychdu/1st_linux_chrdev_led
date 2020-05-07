#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/cdev.h>

//physical addr
#define CCM_CCGR1_BASE 				(0X0204406C)
#define SW_MUX_GPIO1_IO03_BASE		(0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE		(0X020E02F4)
#define GPIO1_DR_BASE  				(0X0209C000)
#define GPIO1_GDIR_BASE				(0X0209C004)

//visual addr ptr
 static void __iomem *IMX6U_CCM_CCGR1;
 static void __iomem *SW_MUX_GPIO1_IO03;
 static void __iomem *SW_PAD_GPIO1_IO03;
 static void __iomem *GPIO1_DR;
 static void __iomem *GPIO1_GDIR;

#define LEDOFF  0
#define LEDON   1
#define NEWCHRLED_CNT			1		  	/* 设备号个数 */
#define NEWCHRLED_NAME "newchrled"

void led_switch(u8 sta)
{
	u32 val = 0;
	if(sta == LEDON) {
		val = readl(GPIO1_DR);
		val &= ~(1 << 3);	
		writel(val, GPIO1_DR);
	}else if(sta == LEDOFF) {
		val = readl(GPIO1_DR);
		val|= (1 << 3);	
		writel(val, GPIO1_DR);
	}	
}
struct newchrled_dev
{
	struct cdev cdev;
	struct class *class;
	struct device *device;
	dev_t devid;
	int major;
	int minor;

};

struct newchrled_dev newchrled;
static int newchrled_open(struct inode *inode, struct file *filp)
{
	flip->private=&newchrled;
	return 0;
}

static int newchrled_release(struct inode *inode, struct file *filp)
{
	struct newchrled_dev *dev=(struct newchrled_dev*)filp->private_data;
	return 0;
}

static ssize_t newchrled_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	retvalue=copy_from_user(databuf,buf,cnt);
	if(retvalue<0)
	{
		printk("write ERR/r/n");
		return -1;
	}
	led_switch(databuf[0]);
	return 0;
}

static ssize_t newchrled_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

static const struct file_operations newchrled_fops=
{
	.owner=THIS_MODULE,
	.open = newchrled_open,
	.read = newchrled_read,
	.write = newchrled_write,
	.release = 	newchrled_release,
};

static int __init newchrled_init(void)
{
	int ret =0 ;
	u32 val = 0;

	/* 初始化LED */
	/* 1、寄存器地址映射 */
  	IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
	SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
  	SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
	GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
	GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

	/* 2、使能GPIO1时钟 */
	val = readl(IMX6U_CCM_CCGR1);
	val &= ~(3 << 26);	/* 清楚以前的设置 */
	val |= (3 << 26);	/* 设置新值 */
	writel(val, IMX6U_CCM_CCGR1);

	/* 3、设置GPIO1_IO03的复用功能，将其复用为
	 *    GPIO1_IO03，最后设置IO属性。
	 */
	writel(5, SW_MUX_GPIO1_IO03);
	
	/*寄存器SW_PAD_GPIO1_IO03设置IO属性
	 *bit 16:0 HYS关闭
	 *bit [15:14]: 00 默认下拉
     *bit [13]: 0 kepper功能
     *bit [12]: 1 pull/keeper使能
     *bit [11]: 0 关闭开路输出
     *bit [7:6]: 10 速度100Mhz
     *bit [5:3]: 110 R0/6驱动能力
     *bit [0]: 0 低转换率
	 */
	writel(0x10B0, SW_PAD_GPIO1_IO03);

	/* 4、设置GPIO1_IO03为输出功能 */
	val = readl(GPIO1_GDIR);
	val &= ~(1 << 3);	/* 清除以前的设置 */
	val |= (1 << 3);	/* 设置为输出 */
	writel(val, GPIO1_GDIR);

	/* 5、默认关闭LED */
	val = readl(GPIO1_DR);
	val |= (1 << 3);	
	writel(val, GPIO1_DR);

	printk("newchrled init \r\n");
	if(newchrled.major)
	{
		newchrled.devid=MKDEV(newchrled.major,0);
		ret=register_chrdev_region(newchrled.major,NEWCHRLED_CNT,NEWCHRLED_NAME);
	}
	else
	{
		ret=alloc_chrdev_region(&newchrled.devid,0, NEWCHRLED_CNT, NEWCHRLED_NAME);
		newchrled.major=MAJOR(newchrled.devid);
		newchrled.minor=MINOR(newchrled.devid);
	}
	if(ret<0)
	{
		printk("newchrled err\r\n");
		return -1;
	}
	printk("newchrled major=%d,minor=%d\r\n",newchrled.major,newchrled.minor);

	newchrled.cdev.owner=THIS_MODULE;
	cdev_init(&newchrled.cdev,&newchrled_fops);

	ret=cdev_add(&newchrled.cdev,newchrled.devid,NEWCHRLED_CNT);

	/* 4、创建类 */
	newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
	if (IS_ERR(newchrled.class)) {
		return PTR_ERR(newchrled.class);
	}

	newchrled.device = device_create(newchrled.class, NULL, newchrled.devid, NULL, NEWCHRLED_NAME);
	if(IS_ERR(newchrled.device))
	{
		return PTR_ERR(newchrled.device);
	}
	return 0;
}

static void __exit newchrled_exit(void)
{
	/* 取消映射 */
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);

	/* 注销字符设备驱动 */
	printk("newchrled_exit\r\n");
	cdev_del(&newchrled.cdev);
	unregister_chrdev_region(newchrled.devid,NEWCHRLED_CNT);

	device_destroy(newchrled.class,newchrled.devid);
	class_destroy(newchrled.class);
}

module_init(newchrled_init);
module_exit(newchrled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("yyc");
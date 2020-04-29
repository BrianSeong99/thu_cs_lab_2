#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mm.h>

#define DEV_MODULENAME "expdev"
#define DEV_CLASSNAME  "expdevclass"
static int majorNumber;
static struct class *devClass = NULL;
static struct device *devDevice = NULL;

char *memaddr; // 开辟的空间的地址

int fops_mmap (struct file *filp, struct vm_area_struct *vma) {
	unsigned long page;
    page = virt_to_phys((void *)memaddr) >> PAGE_SHIFT;

    if(remap_pfn_range(vma, vma->vm_start, page, (vma->vm_end - vma->vm_start), vma->vm_page_prot)) {
        printk("remap failed...");
        return -1;
    }
    vma->vm_flags |= (VM_DONTDUMP|VM_DONTEXPAND);

    return 0;
}

int fops_close (struct inode *inode, struct file *filp) {
	return 0;
}

int fops_open (struct inode *inode, struct file *p_file)
{
	// TODO 此处打印共享内存的地址（共享内存在内核中的虚拟地址，即Demo中的shared_memory_virt）
	printk(KERN_INFO "  > ->data:          0x%16p\n", memaddr);
	return 0;
}

static const struct file_operations dev_fops = { .open = fops_open, .release = fops_close, .mmap =
    fops_mmap, };

static int __init _module_init (void)
{
	int ret = 0;
	int i;
	uint8_t val;

	// Try to dynamically allocate a major number for the device
	majorNumber = register_chrdev(0, DEV_MODULENAME, &dev_fops);
	if (majorNumber < 0)
	{
		printk(KERN_ALERT "Failed to register a major number.\n");
		return -EIO; // I/O error
	}

	// Register the device class
	devClass = class_create(THIS_MODULE, DEV_CLASSNAME);
	// Check for error and clean up if there is
	if (IS_ERR(devClass))
	{
		printk(KERN_ALERT "Failed to register device class.\n");
		ret = PTR_ERR(devClass);
		goto goto_unregister_chrdev;
	}

	// Create and register the device
	devDevice = device_create(devClass, NULL, MKDEV(majorNumber, 0), NULL, DEV_MODULENAME);

	// Clean up if there is an error
	if (IS_ERR(devDevice))
	{
		printk(KERN_ALERT "Failed to create the device.\n");
		ret = PTR_ERR(devDevice);
		goto goto_class_destroy;
	}
	printk(KERN_INFO "Module registered.\n");

	// TODO 此处申请共享内存

	memaddr = kcalloc(0x10000, sizeof(char), GFP_KERNEL);
	if (data == NULL)
	{
		printk(KERN_ERR "insufficient memory\n");
		/* insufficient memory: you must handle this error! */
		return ENOMEM;
	}

  // TODO 此处对共享内存初始化

	for (i = 0; i < (MY_MMAP_LEN / PAGE_SIZE); i++)
	{
		memset(memaddr + (i * PAGE_SIZE), val, PAGE_SIZE);
	}

	return ret;

	// Error handling - using goto
	goto_class_destroy: class_destroy(devClass);
	goto_unregister_chrdev: unregister_chrdev(majorNumber, DEV_MODULENAME);

	return ret;
}

static void __exit _module_exit(void)
{
  device_destroy(devClass, MKDEV(majorNumber, 0));
  class_unregister(devClass);
  class_destroy(devClass);
  unregister_chrdev(majorNumber, DEV_MODULENAME);
  printk(KERN_INFO "Module unregistered.\n");
}

module_init(_module_init);
module_exit(_module_exit);
MODULE_LICENSE("GPL");

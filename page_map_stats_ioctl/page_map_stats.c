#include <linux/module.h>  // Needed by all modules
#include <linux/moduleparam.h> //For using module parameters
#include <linux/kernel.h>  // Needed for KERN_INFO
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>      // Needed by filp
#include <asm/page.h>
#include <asm/segment.h>
#include <asm/uaccess.h>   // Needed by segment descriptors
//#include <limux/kmod.h>
#include <linux/buffer_head.h>
#include <linux/slab.h> // Needed by kmalloc
#include <linux/namei.h>  //Needed for kern_path
#include <linux/slab.h>
#include <linux/pid.h>
#include <asm/io.h>

#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/pagemap.h>
#include <asm/pgtable.h>
#include <linux/rmap.h>
#include <linux/memcontrol.h>
#include <linux/mmu_notifier.h>
#include <asm/pgtable_64_types.h>
#include <linux/kprobes.h>
#include <linux/page-flags.h>
#include <linux/huge_mm.h>
#include <uapi/linux/kernel-page-flags.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>

#define LICENSE "GPL"
#define AUTHOR "Akash Panda"
#define DESC "Get information regarding a page"

MODULE_LICENSE(LICENSE);
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);

#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)
#define PAGE_MAP_STATS _IOWR('a' , 'b' , uint32_t*)

int32_t value = 0;
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops =
{
	.owner          = THIS_MODULE,
	.read           = etx_read,
	.write          = etx_write,
	.open           = etx_open,
	.unlocked_ioctl = etx_ioctl,
	.release        = etx_release,
};


static int etx_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "Device File Opened...!!!\n");
	return 0;
}

static int etx_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "Device File Closed...!!!\n");
	return 0;
}

static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	printk(KERN_INFO "Read Function\n");
	return 0;
}
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	printk(KERN_INFO "Write function\n");
	return 0;
}

struct anon_vma_chain * (*anon_vma_interval_tree_iter_first_p)(struct rb_root_cached *root, unsigned long start, unsigned long last);
struct anon_vma_chain * (*anon_vma_interval_tree_iter_next_p)(struct anon_vma_chain *node, unsigned long start, unsigned long last);

struct anon_vma * (*page_get_anon_vma_p)(struct page *);

struct file *file;

static bool is_stack(struct vm_area_struct *vma)
{
	/*
	 * We make no effort to guess what a given thread considers to be
	 * its "stack".  It's not even well-defined for programs written
	 * languages like Go.
	 */

	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
	//return vma->vm_start >= vma->vm_mm->start_stack &&
	//	vma->vm_end >= vma->vm_mm->start_stack;
}

static bool is_code(struct vm_area_struct *vma)
{
	return vma->vm_start >= vma->vm_mm->start_code &&
		vma->vm_end <= vma->vm_mm->end_code;
}

static bool is_data(struct vm_area_struct *vma)
{
	return vma->vm_start >= vma->vm_mm->start_data &&
		vma->vm_end <= vma->vm_mm->end_data;
}

static bool is_heap(struct vm_area_struct *vma_1)
{
	return vma_1->vm_start >= vma_1->vm_mm->start_brk &&
		vma_1->vm_end <= vma_1->vm_mm->brk;
}

static bool is_file_backed(struct vm_area_struct *vma_1) {
	return vma_1->vm_file != NULL;
}

struct page * get_page_from_pfn(unsigned long pfn) {
	return pfn_to_page(pfn);
}


int do_work(unsigned long pfn) {
	if(!pfn_valid(pfn)) {
		trace_printk("READINGS_PAGEMAP: INVALID_PFN %lx\n" , pfn);
		return;
	}
	struct page *page = get_page_from_pfn(pfn);
	if(page == NULL) {
		trace_printk("READINGS_PAGEMAP: NULL_PAGE %lx\n" , pfn);
		return;
	}
	void* virtual = NULL;
	virtual = page_address(page);
	bool free_page = false;
	bool anon_page = false;
	bool unevictable_page = false;
	bool unmovable_page = false;
	if(PageAnon(page)) {
		anon_page = true;
	}
	if(PageBuddy(page)) {
		free_page = true;
	}
	else if(PageCompound(page)) {

	}
	else if(!PageLRU(page)) {
		if(__PageMovable(page) && !PageIsolated(page)) {
			//This has to be a movable page.
		}
		else {
			//This has to be an unmovable page.
			unmovable_page = true;
		}
	}
	else{
	}
	if(!anon_page && unmovable_page) {
		trace_printk("READINGS_PAGEMAP: UNMOVEABLE_PAGE %lx\n" , pfn);
		return;
	} else if(anon_page){
		page_get_anon_vma_p = (void *) kallsyms_lookup_name("page_get_anon_vma");

		struct anon_vma *anon_vma = page_get_anon_vma_p(page);
		if(anon_vma == NULL) {
			trace_printk( "READINGS_PAGEMAP: UNKNOWN_PAGE_NO_ANON_VMA %lx\n" , pfn);
			return;
		}

		pgoff_t pgoff_start, pgoff_end;
		//   pgoff_start = page_to_pgoff(page);
		//    pgoff_end = pgoff_start + hpage_nr_pages(page) - 1;
		struct anon_vma_chain *avc;
		anon_vma_interval_tree_iter_first_p = (void *) kallsyms_lookup_name("anon_vma_interval_tree_iter_first");
		anon_vma_interval_tree_iter_next_p = (void *) kallsyms_lookup_name("anon_vma_interval_tree_iter_next");

		for (avc = anon_vma_interval_tree_iter_first_p(&anon_vma->rb_root, 0, ULONG_MAX); avc ; avc = anon_vma_interval_tree_iter_next_p(avc, 0 , ULONG_MAX)) {

			//    anon_vma_interval_tree_foreach(avc, &anon_vma->rb_root,
			//			pgoff_start, pgoff_end) {
			struct vm_area_struct *vma = avc->vma;
			pid_t pid_process  = vma->vm_mm->owner->pid;
			if(is_stack(vma)) {
				trace_printk( "READINGS_PAGEMAP: %s %lx %d %lx %lx [stack]\n" , (unmovable_page?"KERNEL_PAGE":"USER_PAGE"), pfn , pid_process , vma->vm_start , vma->vm_end);
			} else if (is_heap(vma)) {
				trace_printk( "READINGS_PAGEMAP: %s %lx %d %lx %lx [heap]\n" , (unmovable_page?"KERNEL_PAGE":"USER_PAGE"), pfn , pid_process , vma->vm_start , vma->vm_end);
			} else if (is_code(vma)) {
				trace_printk( "READINGS_PAGEMAP: %s %lx %d %lx %lx [code]\n" , (unmovable_page?"KERNEL_PAGE":"USER_PAGE"), pfn , pid_process , vma->vm_start , vma->vm_end);
			} else if(is_data(vma)) {
				trace_printk( "READINGS_PAGEMAP: %s %lx %d %lx %lx [data]\n" , (unmovable_page?"KERNEL_PAGE":"USER_PAGE"), pfn , pid_process , vma->vm_start , vma->vm_end);
			} else if (is_file_backed(vma)) {
				char path_file[1024];
				path_file[0] = '\0';
				char *returned_ptr;
				returned_ptr = d_path(&(vma->vm_file->f_path) , path_file , 1024);
				trace_printk( "READINGS_PAGEMAP: %s %lx %d %lx %lx [file_backed] %s\n" ,(unmovable_page?"KERNEL_PAGE":"USER_PAGE"), pfn , pid_process , vma->vm_start , vma->vm_end , returned_ptr);
			} else {
				trace_printk( "READINGS_PAGEMAP: %s %lx %d %lx %lx [pata_nai]\n" , (unmovable_page?"KERNEL_PAGE":"USER_PAGE"), pfn , pid_process , vma->vm_start , vma->vm_end);
			}

		}
		//                trace_printk( "READINGS: %lx %lx %d C\n" , vm_start + ((unsigned long)1 << 12)*num_my , pfn_value + ((unsigned long)1 << 12)*num_my , page_type);
		//}
	} else {
		trace_printk( "READINGS_PAGEMAP: UNKNOWN_PAGE %lx\n" , pfn);
	}
	return 0;
}


struct page_map_stats_struct {
	unsigned long pfn;
};

static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long pfn_value;
	switch(cmd) {
		case PAGE_MAP_STATS:
			copy_from_user(&pfn_value ,(unsigned long*) arg, sizeof(pfn_value));
			do_work(pfn_value);
			break;
		case RD_VALUE:
			copy_to_user((int32_t*) arg, &value, sizeof(value));
			break;
	}
	return 0;
}

static int __init etx_driver_init(void)
{
	/*Allocating Major number*/
	if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
		printk(KERN_INFO "Cannot allocate major number\n");
		return -1;
	}
	printk(KERN_INFO "Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

	/*Creating cdev structure*/
	cdev_init(&etx_cdev,&fops);

	/*Adding character device to the system*/
	if((cdev_add(&etx_cdev,dev,1)) < 0){
		printk(KERN_INFO "Cannot add the device to the system\n");
		goto r_class;
	}

	/*Creating struct class*/
	if((dev_class = class_create(THIS_MODULE,"etx_class")) == NULL){
		printk(KERN_INFO "Cannot create the struct class\n");
		goto r_class;
	}

	/*Creating device*/
	if((device_create(dev_class,NULL,dev,NULL,"page_map_stats_device")) == NULL){
		printk(KERN_INFO "Cannot create the Device 1\n");
		goto r_device;
	}
	printk(KERN_INFO "Device Driver Insert...Done!!!\n");
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev,1);
	return -1;
}

void __exit etx_driver_exit(void)
{
	device_destroy(dev_class,dev);
	class_destroy(dev_class);
	cdev_del(&etx_cdev);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "Device Driver Remove...Done!!!\n");
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);


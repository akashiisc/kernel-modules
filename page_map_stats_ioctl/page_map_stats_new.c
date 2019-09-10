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

struct anon_vma_chain * (*anon_vma_interval_tree_iter_first_p)(struct rb_root_cached *root, unsigned long start, unsigned long last);
struct anon_vma_chain * (*anon_vma_interval_tree_iter_next_p)(struct anon_vma_chain *node, unsigned long start, unsigned long last);


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

static inline struct address_space *page_mapping_cache(struct page *page)
{
	if (!page->mapping || PageAnon(page))
		return NULL;
	return page->mapping;
}

static bool page_is_page_cache(struct page *page) {
	return (page_mapping_cache(page) != NULL);
}

enum page_type_val {TYPE_NONE , TYPE_STACK , TYPE_HEAP , TYPE_CODE , TYPE_DATA , TYPE_FILE};

struct kernel_page_flags {
	uint64_t x;
	uint64_t r;
	uint64_t w;
};

struct page_flags_values {
	bool mmap;
	bool anon;
	bool ksm;
	bool buddy;
	bool page_cache;
	bool no_anon_vma;
	bool file_backed;
	char path_file[1024];
	char file_name[1024];
	char *path_returned_ptr;
	enum page_type_val page_type;
	int num_anon_vma;
	bool unmovable_page;
	pid_t pid;
	uint64_t virtual_address;
	struct kernel_page_flags kpf;
};

struct page_details {
	unsigned long pfn_value;
	struct page_flags_values pf;
};


void set_default_values(struct page_flags_values *pf) {
	pf->mmap = 0;
	pf->anon = 0;
	pf->ksm = 0;
	pf->buddy = 0;
	pf->page_cache = 0;
	pf->no_anon_vma = 0;
	pf->file_backed = 0;
	pf->path_returned_ptr = NULL;
	pf->page_type = TYPE_NONE;
	pf->num_anon_vma = 0;
	pf->unmovable_page = 0;
	pf->virtual_address = 0;
}

pteval_t get_pte_value(struct mm_struct *mm, unsigned long address) {
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd = NULL;
    pmd_t pmde;
    pte_t *ptep , pte;

    pgd = pgd_offset(mm, address);
    if (!pgd_present(*pgd))
        goto out;
    p4d = p4d_offset(pgd, address);
    if (!p4d_present(*p4d))
        goto out;

    pud = pud_offset(p4d, address);
    if (!pud_present(*pud))
        goto out;
    if(pud_large(*pud) || pud_trans_huge(*pud)) {
        return pud->pud;
    }
    pmd = pmd_offset(pud, address);
    /*
     * Some THP functions use the sequence pmdp_huge_clear_flush(), set_pmd_at()
     * without holding anon_vma lock for write.  So when looking for a
     * genuine pmde (in which to find pte), test present and !THP together.
     */
    pmde = *pmd;
    if(pmd_large(pmde) || pmd_trans_huge(pmde)) {
        return pmd->pmd;
    }
    if(pmd_present(pmde)) {
        ptep = pte_offset_kernel(pmd, address);
        if(pte_present(*ptep)) {
            pte = *ptep;
            return pte.pte;
        }
        return pte.pte;
    }
out:
    return 0;
}


struct kernel_page_flags get_kernel_page_flags(uint64_t address) {
	struct kernel_page_flags kpf;
	struct task_struct *ts = current;
	struct mm_struct *mm = ts->mm;
	//uint64_t virtual_address = 0xffff8e48b0ff5000;
	pteval_t pte  = get_pte_value(mm , address);
	pte_t ptett1;
	ptett1.pte = pte;
	kpf.w =  pte_write(ptett1);
	kpf.x = pte_exec(ptett1);
	int z = _AT( pteval_t, pte_flags(ptett1)) & (_PAGE_PRESENT | _PAGE_USER | _PAGE_ACCESSED | _PAGE_NX );
	int w = (_PAGE_PRESENT | _PAGE_USER | _PAGE_ACCESSED | _PAGE_NX);
	kpf.x = z==w;
	return kpf;
}

struct page_flags_values do_work(unsigned long pfn) {
	struct page *ppage;
	if(pfn_valid(pfn)) {
		ppage = pfn_to_page(pfn);
	} else {
		ppage = NULL;
	}
	if(ppage == NULL) {
		//printk(KERN_ALERT "No physical page\n")	;
	} else {
		//printk(KERN_ALERT "Physical page\n");
		unsigned long k = ppage->flags;
		struct page_flags_values pf;
		set_default_values(&pf);
		pf.mmap = !PageSlab(ppage) && page_mapped(ppage);

		pf.anon = PageAnon(ppage);
		pf.ksm = PageKsm(ppage);
		if(PageBuddy(ppage)) {
			pf.buddy = 1;
		}
		if(page_is_page_cache(ppage)) {
			pf.page_cache = 1;
			//printk(KERN_ALERT "READINGS_PAGEMAP: PAGE_CACHE %lx\n" , pfn);
		}
		//physical_page_details 	
		if(!PageLRU(ppage)) {
			pf.unmovable_page = 1;
			pf.virtual_address = (uint64_t)page_to_virt(ppage);
			if(pf.virtual_address != 0) {
				struct kernel_page_flags kpf = get_kernel_page_flags(pf.virtual_address);
				pf.kpf = kpf;
			}
			/*
			   if(__PageMovable(ppage) && !PageIsolated(ppage)) {
			//This has to be a movable page.
			}
			else {
			//This has to be an unmovable page.
			pf.unmovable_page = 1;
			}
			*/
		}
		page_get_anon_vma_p = (void *) kallsyms_lookup_name("page_get_anon_vma");
		struct anon_vma *avma = page_get_anon_vma_p(ppage);
		if(PageAnon(ppage) && avma != NULL) {
			//printk(KERN_ALERT "READINGS_PAGEMAP: UNKNOWN_PAGE_NO_ANON_VMA %lx\n" , pfn);
			pgoff_t pgoff_start, pgoff_end;
			//   pgoff_start = page_to_pgoff(page);
			//    pgoff_end = pgoff_start + hpage_nr_pages(page) - 1;
			struct anon_vma_chain *avc;
			anon_vma_interval_tree_iter_first_p = (void *) kallsyms_lookup_name("anon_vma_interval_tree_iter_first");
			anon_vma_interval_tree_iter_next_p = (void *) kallsyms_lookup_name("anon_vma_interval_tree_iter_next");

			for (avc = anon_vma_interval_tree_iter_first_p(&avma->rb_root, 0, ULONG_MAX); avc ; avc = anon_vma_interval_tree_iter_next_p(avc, 0 , ULONG_MAX)) {
				pf.num_anon_vma++;
				//    anon_vma_interval_tree_foreach(avc, &anon_vma->rb_root,
				//                      pgoff_start, pgoff_end) {
				struct vm_area_struct *vma = avc->vma;
				pid_t pid_process  = vma->vm_mm->owner->pid;
				pf.pid = pid_process;
				if(is_stack(vma)) {
					pf.page_type = TYPE_STACK;
				} else if (is_heap(vma)) {
					pf.page_type = TYPE_HEAP;
				} else if (is_code(vma)) {
					pf.page_type = TYPE_CODE;
				} else if(is_data(vma)) {
					pf.page_type = TYPE_DATA;
				} else if (is_file_backed(vma)) {
					pf.file_backed = 1;
					pf.page_type = TYPE_FILE;
					pf.path_file[0] = '\0';
					pf.path_returned_ptr = d_path(&(vma->vm_file->f_path) , pf.path_file , 1024);
					//printk(KERN_ALERT "FILE : %s" , pf.path_returned_ptr);
					//printk(KERN_ALERT "FILE1 : %s" , &pf.path_file[0]);
					char ch = *(pf.path_returned_ptr);
					int i = 0;
					while(ch != '\0') {
						pf.file_name[i] = ch;
						i++;
						ch = *(pf.path_returned_ptr+i); 
					}
					pf.file_name[i] = '\0';
				}

			}
			} else if (PageAnon(ppage) && avma == NULL) {
				pf.no_anon_vma = 1;
			}
			return pf;
		}
	}


	int evts_push_str(struct page_details *arg , struct page_details *kernel_a) {
		copy_to_user(arg->pf.file_name , kernel_a->pf.file_name, 1024 * sizeof(char));
	}

	static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
	{
		unsigned long pfn_value;
		struct page_details pd;
		switch(cmd) {
			case PAGE_MAP_STATS:
				copy_from_user(&pd ,(unsigned long*) arg, sizeof(struct page_details));
				struct page_flags_values pf = do_work(pd.pfn_value);
				pd.pf = pf;
				copy_to_user((struct page_details*) arg, &pd, sizeof(pd));
				evts_push_str((struct page_details*)arg , &pd);
				//copy_to_user((char *) ((struct page_details*)arg)->pf.path_file , pd.pf.path_file , sizeof(pd.pf.path_file));
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


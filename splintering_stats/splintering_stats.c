#include <linux/module.h>  // Needed by all modules
#include <linux/moduleparam.h> //For using module parameters
#include <linux/kernel.h>  // Needed for KERN_INFO
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
#include <linux/pagemap.h>
#include <asm/pgtable.h>
#include <linux/rmap.h>
#include <linux/memcontrol.h>
#include <linux/mmu_notifier.h>
#include <asm/pgtable_64_types.h>



#define LICENSE "GPL"
#define AUTHOR "Akash Panda"
#define DESC "Reading from a file"

MODULE_LICENSE(LICENSE);
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);

static int pid=0;
module_param(pid,int,0660);

static char *printstring = "Technicality Inside ";
module_param(printstring , charp, 0000);

struct file *file;


unsigned long get_pfn_value(struct mm_struct *mm, unsigned long address , int *page_type) {
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
    if(pud_trans_huge(*pud)) {
        *page_type = 1;
        return pud_pfn(*pud);
    }
    pmd = pmd_offset(pud, address);
    /*
     * Some THP functions use the sequence pmdp_huge_clear_flush(), set_pmd_at()
     * without holding anon_vma lock for write.  So when looking for a
     * genuine pmde (in which to find pte), test present and !THP together.
     */
    pmde = *pmd;
    if(pmd_trans_huge(pmde)) {
        *page_type = 2;
        return pmd_pfn(*pmd);
    }
    if(pmd_present(pmde)) {
        *page_type = 3;
        ptep = pte_offset_kernel(pmd, address);
        if(pte_present(*ptep)) {
            pte = *ptep;
            return pte_pfn(pte);
        }
        return pte_pfn(pte);
    }
out: 
    return 0;
}


pmd_t *mm_find_pmd_custom(struct mm_struct *mm, unsigned long address)
{
    pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd = NULL;
	pmd_t pmde;

	pgd = pgd_offset(mm, address);
	if (!pgd_present(*pgd))
		goto out;
	p4d = p4d_offset(pgd, address);
	if (!p4d_present(*p4d))
		goto out;

	pud = pud_offset(p4d, address);
	if (!pud_present(*pud))
		goto out;
    if(pud_trans_huge(*pud)) {
        printk(KERN_ALERT "%s : 2MB page" , printstring);
    }
	pmd = pmd_offset(pud, address);
	/*
	 * Some THP functions use the sequence pmdp_huge_clear_flush(), set_pmd_at()
	 * without holding anon_vma lock for write.  So when looking for a
	 * genuine pmde (in which to find pte), test present and !THP together.
	 */
	pmde = *pmd;
	barrier();
	if (!pmd_present(pmde) || pmd_trans_huge(pmde))
		pmd = NULL;
out:
	return pmd;
}



struct file *file_open(const char *path, int flags, int rights) 
{
    struct file *filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

void file_close(struct file *file) 
{
    filp_close(file, NULL);
}

int file_read(struct file *file, unsigned long long offset, unsigned char *data, unsigned long long size) 
{
    //return kernel_read(file , data , size , &offset);
        
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_read(file, data, size, &offset);

    set_fs(oldfs);
    return ret;

}   


struct mm_struct* pid_to_mmstruct(int pid)
{
    struct task_struct *task;
    struct mm_struct *mm;
    void *cr3_virt;
    unsigned long cr3_phys;

    task = pid_task(find_vpid(pid), PIDTYPE_PID);

    if (task == NULL)
        return NULL; // pid has no task_struct

    mm = task->mm;
    return mm;
}


unsigned long mm_struct_to_cr3(struct mm_struct *mm)
{
    void *cr3_virt;
    unsigned long cr3_phys;

    struct task_struct *task = mm->owner;
    // mm can be NULL in some rare cases (e.g. kthreads)
    // when this happens, we should check active_mm
    if (mm == NULL) {
        mm = task->active_mm;
    }

    if (mm == NULL)
        return 0; // this shouldn't happen, but just in case

    cr3_virt = (void *) mm->pgd;
    cr3_phys = virt_to_phys(cr3_virt);

    return cr3_phys;
}

static int __init technicalityinside_init(void) {
//    printk(KERN_ALERT "Technicality inside : PID : %lld" , pid);
//    printk(KERN_ALERT "Print String: %s" , printstring);
    struct mm_struct *mm = pid_to_mmstruct(pid);

    if(mm == NULL) {
        printk(KERN_ALERT "%s : Not able to get mm_struct" , printstring);
        return 0;
    }
    printk(KERN_ALERT "%s : Started",printstring);
    struct vm_area_struct *vmas = mm->mmap;
    int tries = 0;
    while (vmas != NULL) {
        unsigned long vm_start = vmas->vm_start;		
        unsigned long vm_end = vmas->vm_end;
        while(vm_start <= vm_end) {
            int page_type; 
            unsigned long pfn_value = get_pfn_value(mm , vm_start , &page_type);
            if(pfn_value == 0) {
                printk(KERN_ALERT "READINGS: %lx 0 0" , vm_start);
            } else {
                printk(KERN_ALERT "READINGS: %lx %lx %d" , vm_start , pfn_value , page_type);
            }
            if(page_type == 1) {
                vm_start = vm_start + ((unsigned long)1 << 30);
            } else if(page_type == 2) {
                vm_start = vm_start + ((unsigned long)1 << 21);
            } else {
                vm_start = vm_start + ((unsigned long)1 << 12);
            }

        }
        vmas = vmas->vm_next;
        tries++;
    }
    return 0;
}





static void __exit technicalityinside_exit(void) {
    printk(KERN_ALERT "Module Exit : Technicality inside\n");
//    file_close(file);
}

module_init(technicalityinside_init);
module_exit(technicalityinside_exit);



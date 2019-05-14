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

#define LICENSE "GPL"
#define AUTHOR "Akash Panda"
#define DESC "Get information regarding a page"

MODULE_LICENSE(LICENSE);
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);

struct anon_vma_chain * (*anon_vma_interval_tree_iter_first_p)(struct rb_root_cached *root, unsigned long start, unsigned long last);
struct anon_vma_chain * (*anon_vma_interval_tree_iter_next_p)(struct anon_vma_chain *node, unsigned long start, unsigned long last);

struct anon_vma * (*page_get_anon_vma_p)(struct page *);

static int pfn_address=0;
module_param(pfn_address,int,0660);

static char *printstring = "Technicality Inside ";
module_param(printstring , charp, 0000);

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

static bool is_heap(struct vm_area_struct *vma_1)
{
    return vma_1->vm_start <= vma_1->vm_mm->brk &&
                        vma_1->vm_end >= vma_1->vm_mm->start_brk;
}

static bool is_file_backed(struct vm_area_struct *vma_1) {
    return vma_1->vm_file != NULL;
}

struct page * get_page_from_pfn(unsigned long pfn) {
    return pfn_to_page(pfn);
}

static int __init technicalityinside_init(void) {
//    trace_printk(KERN_ALERT "Technicality inside : PID : %lld" , pid);
//    trace_printk(KERN_ALERT "Print String: %s" , printstring);
    unsigned long pfn = 0x279bf6;
    
    struct page *page = get_page_from_pfn(pfn);
    page_get_anon_vma_p = (void *) kallsyms_lookup_name("page_get_anon_vma");

    struct anon_vma *anon_vma = page_get_anon_vma_p(page);
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
        if(is_stack(vma)) {
            trace_printk( "READINGS:  %lx %lx [stack]\n" , vma->vm_start , vma->vm_end);        
        } else if (is_heap(vma)) {
            trace_printk( "READINGS:  %lx %lx [heap]\n" , vma->vm_start , vma->vm_end);
        } else if (is_file_backed(vma)) {
            trace_printk( "READINGS:  %lx %lx [file_backed]\n" , vma->vm_start , vma->vm_end);
        } else {
            trace_printk( "READINGS:  %lx %lx [pata nai]\n" , vma->vm_start , vma->vm_end);
        }
        
	}
    //                trace_printk( "READINGS: %lx %lx %d C\n" , vm_start + ((unsigned long)1 << 12)*num_my , pfn_value + ((unsigned long)1 << 12)*num_my , page_type);
    //}
    return 0;
}





static void __exit technicalityinside_exit(void) {
    printk(KERN_ALERT "Module Exit : Technicality inside\n");
//    file_close(file);
}

module_init(technicalityinside_init);
module_exit(technicalityinside_exit);



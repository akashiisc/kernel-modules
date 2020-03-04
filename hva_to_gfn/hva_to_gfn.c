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
#include <linux/kvm_host.h>



#define LICENSE "GPL"
#define AUTHOR "Akash Panda"
#define DESC "Reading from a file"

MODULE_LICENSE(LICENSE);
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);


static int pid=0;
module_param(pid,int,0660);

static unsigned long hva=0;
module_param(hva,ulong,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

extern struct kvm* get_kvm_ptr(void);
extern struct kvm* get_kvm_ptr_by_pid(pid_t pid);

gfn_t get_gfn_from_hva(long unsigned int hva , int pid) {
    struct kvm *kvm = get_kvm_ptr_by_pid(pid);
    struct kvm_memslots *slots;
    struct kvm_memory_slot *memslot;
    slots = kvm_memslots(kvm);
    kvm_for_each_memslot(memslot, slots) {
        unsigned long start = memslot->userspace_addr;
		unsigned long end;
		end = start + (memslot->npages << PAGE_SHIFT);
        if (hva >= start && hva < end) {
            //gfn_t gfn_offset = (hva - start) ;
            gfn_t gfn_offset = (hva - start) >> PAGE_SHIFT;
            gfn_t gfn = memslot->base_gfn + gfn_offset;
            return gfn;
        }
    }
    return 0;
}


static int __init technicalityinside_init(void) {
    gfn_t gfn = get_gfn_from_hva(hva , pid);
    //gpa_t gpa = gfn_to_gpa(gfn);
    trace_printk("KVM_SLOT_ADDR: %d-%llx\n" , pid , gfn);
    return 0;
}





static void __exit technicalityinside_exit(void) {
//    file_close(file);
}

module_init(technicalityinside_init);
module_exit(technicalityinside_exit);



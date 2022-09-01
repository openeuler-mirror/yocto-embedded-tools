/*
 *  A lite version of linux/drivers/char/mem.c
 *  Test with MMU for arm64 mcs functions
 */

#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/backing-dev.h>
#include <linux/shmem_fs.h>
#include <linux/splice.h>
#include <linux/pfn.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/uio.h>
#include <linux/uaccess.h>
#include <linux/security.h>
#include <linux/pseudo_fs.h>
#include <uapi/linux/magic.h>
#include <linux/mount.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#define DEVMEM_MINOR    1
#define START_INDEX     1
#define SIZE_INDEX  2
static u64 valid_start;
static u64 valid_end;

static ssize_t read_mem(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
    return -ENOSYS;
}

static ssize_t write_mem(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
    return -ENOSYS;
}

#ifdef CONFIG_STRICT_DEVMEM
static inline int range_is_allowed(unsigned long pfn, unsigned long size)
{
    u64 from = ((u64)pfn) << PAGE_SHIFT;
    u64 to = from + size;
    u64 cursor = from;

    while (cursor < to) {
        if (page_is_ram(pfn))
            return 0;
        cursor += PAGE_SIZE;
        pfn++;
    }
    return 1;
}
#else
static inline int range_is_allowed(unsigned long pfn, unsigned long size)
{
    return 1;
}
#endif

int mcs_phys_mem_access_prot_allowed(struct file *file,
	unsigned long pfn, unsigned long size, pgprot_t *vma_prot)
{
    u64 start, end;
    start = ((u64)pfn) << PAGE_SHIFT;
    end = start + size;

    if (valid_start == 0 && valid_end == 0) {
        if (!range_is_allowed(pfn, size))
            return 0;
        return 1;
    }

    if (start < valid_start || end > valid_end) {
        return 0;
    }

    return 1;
}

static pgprot_t mcs_phys_mem_access_prot(struct file *file, unsigned long pfn,
				     unsigned long size, pgprot_t vma_prot)
{
    return __pgprot_modify(vma_prot, PTE_ATTRINDX_MASK, PTE_ATTRINDX(MT_NORMAL_NC) | PTE_PXN | PTE_UXN);
}

static const struct vm_operations_struct mmap_mem_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys
#endif
};

static int mmap_mem(struct file *file, struct vm_area_struct *vma)
{
	size_t size = vma->vm_end - vma->vm_start;
	phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;

	/* Does it even fit in phys_addr_t? */
	if (offset >> PAGE_SHIFT != vma->vm_pgoff)
		return -EINVAL;

	/* It's illegal to wrap around the end of the physical address space. */
	if (offset + (phys_addr_t)size - 1 < offset)
		return -EINVAL;

	if (!mcs_phys_mem_access_prot_allowed(file, vma->vm_pgoff, size,
						&vma->vm_page_prot))
		return -EINVAL;

	vma->vm_page_prot = mcs_phys_mem_access_prot(file, vma->vm_pgoff,
						 size,
						 vma->vm_page_prot);

	vma->vm_ops = &mmap_mem_ops;

	/* Remap-pfn-range will mark the range VM_IO */
	if (remap_pfn_range(vma,
			    vma->vm_start,
			    vma->vm_pgoff,
			    size,
			    vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

static loff_t memory_lseek(struct file *file, loff_t offset, int orig)
{
    return -ENOSYS;
}

static int open_port(struct inode *inode, struct file *filp)
{
	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;
	return 0;
}

#define open_mem	open_port
static const struct file_operations __maybe_unused mem_fops = {
	.llseek		= memory_lseek,
	.read		= read_mem,
	.write		= write_mem,
	.mmap		= mmap_mem,
	.open		= open_mem,
};

static const struct memdev {
	const char *name;
	umode_t mode;
	const struct file_operations *fops;
	fmode_t fmode;
} devlist[] = {
	 [DEVMEM_MINOR] = { "mcsmem", 0, &mem_fops, FMODE_UNSIGNED_OFFSET },
};

static int memory_open(struct inode *inode, struct file *filp)
{
	int minor;
	const struct memdev *dev;

	minor = iminor(inode);
	if (minor >= ARRAY_SIZE(devlist))
		return -ENXIO;

	dev = &devlist[minor];
	if (!dev->fops)
		return -ENXIO;

	filp->f_op = dev->fops;
	filp->f_mode |= dev->fmode;

	if (dev->fops->open)
		return dev->fops->open(inode, filp);

	return 0;
}

static const struct file_operations memory_fops = {
	.open = memory_open,
	.llseek = noop_llseek,
};

static struct class *mem_class;

int mcsmem_major;

static int get_mcs_node_info(void)
{
    int ret = 0;
    struct device_node *nd = NULL;
    u8 datasize;
    u32 *val = NULL;

    nd = of_find_node_by_path("/reserved-memory/mcs@70000000");
    if(nd == NULL) {
        printk("invalid reserved-memory mcs node.\n");
        return -EINVAL;
    }

    datasize = of_property_count_elems_of_size(nd, "reg", sizeof(u32));
    if (datasize != 3) {
        printk("invalid reserved-memory mcs reg size.\n");
        return -EINVAL;
    }

    val = kmalloc(datasize * sizeof(u32), GFP_KERNEL);
    if (val == NULL)
        return -ENOMEM;

    ret = of_property_read_u32_array(nd, "reg", val, datasize);
    if (ret < 0)
        goto out;

    valid_start = (u64)(*(val + START_INDEX));
    valid_end = valid_start + (u64)(*(val + SIZE_INDEX));

out:
    kfree(val);
    return ret;
}

static int __init mcsmem_dev_init(void)
{
	int minor = 1;

    if (get_mcs_node_info() < 0)
		printk("cat't get mcsmem dts node info. Allow page isn't ram mmap.\n");
    else
        printk("valid mcsmem node decated.\n");

    mcsmem_major = register_chrdev(0, "mcsmem", &memory_fops);
	if (mcsmem_major < 0) {
		printk("unable to get major %d for memory devs\n", mcsmem_major);
        return -1;
    }

	mem_class = class_create(THIS_MODULE, "mcsmem");
	if (IS_ERR(mem_class))
		return PTR_ERR(mem_class);

	device_create(mem_class, NULL, MKDEV((unsigned int)mcsmem_major, minor),
            NULL, devlist[minor].name);

	return 0;
}

module_init(mcsmem_dev_init);
MODULE_DESCRIPTION("dev_mcsmem");
MODULE_LICENSE("GPL v2");

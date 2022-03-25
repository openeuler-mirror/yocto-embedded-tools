#include <linux/device.h>
#include <linux/file.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/arm-smccc.h>
#include <linux/irqnr.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/cpumask.h>

#define CPU_HANDLER_DEVICE  "cpu_handler"
#define CPU_ON_FUNCID       0xC4000003
#define IRQ_SENDTO_CLIENTOS  _IOW('A', 0, int)
#define OPENAMP_INIT_TRIGGER      0x0
#define OPENAMP_CLIENTOS_TRIGGER  0x1
#define OPENAMP_IRQ  8

static DEFINE_MUTEX(cpu_handler_mutex);
static struct class *cpu_handler_class;
static int cpu_handler_major;
static int cpu_state;

static ssize_t dev_cpuhandler_write(struct file *file, const char __user *buf,
		size_t datalen, loff_t *ppos)
{
	ssize_t result = -EINVAL;
	int cpu_id;
    phys_addr_t cpu_boot_addr;
    char *data = NULL, *startp = NULL, *endp = NULL;
    struct arm_smccc_res res;

	mutex_lock(&cpu_handler_mutex);

	if (cpu_state == 1) {
		pr_info("cpu_handler: clientos os already boot\n");
		goto out;
	}

    data = memdup_user_nul(buf, datalen);
    if (!data) {
        pr_err("cpu_handler: invalid input data\n");
        goto out;
    }

    startp = data;
    cpu_id = simple_strtol(startp, &endp, 0);
    if (*endp != '@') {
        pr_err("cpu_handler: invalid boot cpu format, str = %s\n", data);
        goto out;
    }
    startp = endp + 1;

#ifdef CONFIG_PHYS_ADDR_T_64BIT
    cpu_boot_addr = simple_strtoull(startp, &endp, 0);
#else
    cpu_boot_addr = simple_strtoul(startp, &endp, 0);
#endif

    if (startp == endp) {
        pr_err("cpu_handler: invalid boot cpu format, str = %s\n", data);
        goto out;
    }

	if (cpu_state == 0) {
		pr_info("cpu_handler: start booting clientos os on cpu(%d)\n", cpu_id);
		arm_smccc_hvc(CPU_ON_FUNCID, cpu_id, cpu_boot_addr, 0, 0, 0, 0, 0, &res);
		cpu_state = 1;
		result = datalen;
	}
out:
	mutex_unlock(&cpu_handler_mutex);
	return result;
}

static DECLARE_WAIT_QUEUE_HEAD(openamp_trigger_wait);

static int openamp_trigger;

static irqreturn_t handle_clientos_ipi(int irq, void *data)
{
    pr_info("cpu_handler: received ipi from client os\n");
    openamp_trigger = OPENAMP_CLIENTOS_TRIGGER;
    wake_up_interruptible(&openamp_trigger_wait);
    return IRQ_HANDLED;
}

static struct irq_desc *openamp_ipi_desc;

void set_openamp_ipi(void)
{
    int err;

    err = request_percpu_irq(OPENAMP_IRQ, handle_clientos_ipi, "IPI", &cpu_number);
    if (err) {
        pr_err("cpu_handler: request openamp irq failed(%d)\n", err);
        return;
    }

    openamp_ipi_desc = irq_to_desc(OPENAMP_IRQ);
    if (!openamp_ipi_desc) {
        pr_err("cpu_handler: generate openamp irq descriptor failed\n");
        return;
    }

    enable_percpu_irq(OPENAMP_IRQ, 0);

    return;
}

static void send_clientos_ipi(const struct cpumask *target)
{
    ipi_send_mask(OPENAMP_IRQ, target);
}

static ssize_t dev_cpuhandler_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	ssize_t len;
	char res[32];

	len = scnprintf(res, sizeof(res), "%d\n", cpu_state);
	return simple_read_from_buffer(buf, count, ppos, res, len);
}

static unsigned int dev_cpuhandler_poll(struct file *file, poll_table *wait)
{
    unsigned int mask;

    poll_wait(file, &openamp_trigger_wait, wait);
    mask = 0;
    if (openamp_trigger == OPENAMP_CLIENTOS_TRIGGER)
        mask |= POLLIN | POLLRDNORM;

    openamp_trigger = OPENAMP_INIT_TRIGGER;
    return mask;
}

static long dev_cpuhandler_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    int cpu_id = (int)arg;
    switch (cmd) {
    case IRQ_SENDTO_CLIENTOS:
        pr_info("cpu_handler: received ioctl cmd to send ipi to cpu(%d)\n", cpu_id);
        send_clientos_ipi(cpumask_of(cpu_id));
        return 0;
    default:
        return -EINVAL;
    }
}

static const struct file_operations dev_cpuhandler_fops = {
	.read = dev_cpuhandler_read,
	.write = dev_cpuhandler_write,
    .poll = dev_cpuhandler_poll,
    .unlocked_ioctl = dev_cpuhandler_ioctl,
	.llseek = generic_file_llseek,
};

static __init int cpu_handler_dev_init(void)
{
	struct device *class_dev = NULL;
	int ret = 0;

	cpu_handler_major = register_chrdev(0, CPU_HANDLER_DEVICE, &dev_cpuhandler_fops);
	if (cpu_handler_major < 0) {
		pr_err("cpu_handler: unable to get major %d for memory devs.\n", cpu_handler_major);
		return -1;
	}

	cpu_handler_class = class_create(THIS_MODULE, CPU_HANDLER_DEVICE);
	if (IS_ERR(cpu_handler_class)) {
		ret = PTR_ERR(cpu_handler_class);
		goto error_class_create;
	}

	class_dev = device_create(cpu_handler_class, NULL, MKDEV((unsigned int)cpu_handler_major, 1), 
			NULL, CPU_HANDLER_DEVICE);
	if (unlikely(IS_ERR(class_dev))) {
		ret = PTR_ERR(class_dev);
		goto error_device_create;
	}

    set_openamp_ipi();
	pr_info("cpu_handler: create major %d for cpu handler devs\n", cpu_handler_major);
	return 0;

error_device_create:
	class_destroy(cpu_handler_class);
error_class_create:
	unregister_chrdev(cpu_handler_major, CPU_HANDLER_DEVICE);
	return ret;
}
module_init(cpu_handler_dev_init);

static void __exit cpu_handler_dev_exit(void)
{
	device_destroy(cpu_handler_class, MKDEV((unsigned int)cpu_handler_major, 1));
	class_destroy(cpu_handler_class);
	unregister_chrdev(cpu_handler_major, CPU_HANDLER_DEVICE);
}
module_exit(cpu_handler_dev_exit);

MODULE_AUTHOR("OpenEuler Embedded");
MODULE_DESCRIPTION("cpu_handler device");
MODULE_LICENSE("Dual BSD/GPL");

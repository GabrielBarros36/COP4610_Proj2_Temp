#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Elevator scheduler kernel module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "elevator"
#define ENTRY_PERMS 0666
#define PARENT NULL
#define ELEVATOR_LIMIT 5        //max passengers in elevator
#define ELEVATOR_MAX_FLOOR 5    //max floor in building

static struct proc_dir_entry* elevator_entry;

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[10000];
    int len = 0;

    len = sprintf(buf, "Elevator state: \n");
    len += sprintf(buf + len, "Current floor: \n");
    len += sprintf(buf + len, "Current load: \n");
    len += sprintf(buf + len, "Elevator status: \n");
    // you can finish the rest.

    // not sure what else needs to be done here

    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};

static int __init elevator_init(void){
    //proc file operations
    elevator_entry = proc_create(ENTRY_NAME, ENTRY_PERMS, PARENT, &elevator_fops);
    if (elevator_entry == NULL){
        return -ENOMEM;
    }

    //insert rest of init operations here

    return 0;
}

static viud __exit elevator_exit(void){
    //proc file operation
    proc_remove(elevator_entry);

    //insert rest of exit operations here
}


module_init(elevator_init);
module_exit(elevator_exit);
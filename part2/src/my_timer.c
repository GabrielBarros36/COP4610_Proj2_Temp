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
MODULE_DESCRIPTION("A simple Linux kernel module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "timer"
#define PERMS 0666
#define PARENT NULL
#define BUF_LEN 100
#define PROC_PATH "/proc/timer"
static struct proc_dir_entry *timer_entry;
long long latestSec = 0;
long long latestNs = 0;

/*

void parseLastNumber(const char *filename, long long *x) {
    FILE *file;
    long long temp;
    int readCount;

    file = fopen(filename, "r");
    if (!file) {
        perror("Unable to open the file");
        exit(EXIT_FAILURE);
    }

    while (fscanf(file, "%lld", &temp) == 1) {
        *x = temp; 
    }

    fclose(file);
}

*/

/*
static long long read_last_number(const char *filepath) {
    struct file *file;
    long long last_number = 0;
    mm_segment_t oldfs;
    char buf[128]; // Assuming numbers are not longer than 128 bytes
    int bytes_read;

    // Open the file
    oldfs = get_fs();
    set_fs(get_ds());
    file = filp_open(filepath, O_RDONLY, 0);
    set_fs(oldfs);

    if (IS_ERR(file)) {
        printk(KERN_ERR "Error opening file: %s\n", filepath);
        return 0;
    }

    // Seek to the end of the file and read backwards (this is a simplified example)
    // Real implementation might involve more complex logic
    oldfs = get_fs();
    set_fs(get_ds());
    bytes_read = kernel_read(file, buf, sizeof(buf), &(file->f_pos));
    set_fs(oldfs);

    // Parse the last number from buf here
    // This is a simplified placeholder logic
    if (bytes_read > 0) {
        // Logic to parse the last number from the buffer
        // last_number = parse_number_from_buffer(buf);
    }

    // Close the file
    filp_close(file, NULL);

    return last_number;
}
*/

static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos){
	struct timespec64 ts_now; 
	char buf[BUF_LEN];
	int len = 0;
	long long secDiff;
	long long nanoDiff; 

	ktime_get_real_ts64(&ts_now);

	nanoDiff = ts_now.tv_nsec - latestNs;
	secDiff = ts_now.tv_sec - latestSec;

	if(nanoDiff < 0){
		--secDiff;	
	}

	if(!latestSec)
		len = snprintf(buf, sizeof(buf), "current time: %lld.%lld\n", (long long)ts_now.tv_sec, (long long)ts_now.tv_nsec);
	else
		
		len = snprintf(buf, sizeof(buf), "current time: %lld.%lld\nelapsed time: %lld.%lld\n", (long long)ts_now.tv_sec, (long long)(ts_now.tv_nsec) , secDiff, nanoDiff);

	latestSec = ts_now.tv_sec;
	latestNs = ts_now.tv_nsec;
	
	return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

static const struct proc_ops timer_fops = {
	.proc_read = timer_read,
};

static int __init timer_init(void){
	timer_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &timer_fops);
	if(!timer_entry){
		return -ENOMEM;
	}
	return 0;
}

static void __exit timer_exit(void){
	proc_remove(timer_entry);
}

module_init(timer_init);
module_exit(timer_exit);


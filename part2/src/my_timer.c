#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/kthread.h>

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



#include <stdio.h>
#include <stdlib.h>

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

static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos){
	struct timespec64 ts_now; 
	long long timeDiff;
	char buf[BUF_LEN];
	int len = 0;
	long long latestNum = 0;

	parseLastNumber(PROC_PATH, latestNum);

	ktime_get_real_ts64(&ts_now);
	len = snprintf(buf, sizeof(buf), "current time: %lld\nelapsed time:%lld\n", (long long)ts_now.tv_sec, (long long)(ts_now.tv_sec - latestNum));
	
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


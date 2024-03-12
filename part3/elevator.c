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
#define WEIGHT_LIMIT 7
#define ELEVATOR_MAX_FLOOR 5    //max floor in building

static struct proc_dir_entry* elevator_entry;

//Keeps track of the passengers INSIDE the elevator
struct {
    int total_cnt;
    int total_length;
    int total_weight_int;
    int total_weight_dec;
    struct list_head list;
} passengers;

//Keeps track of the passengers WAITING for the elevator
struct{
    int total_cnt;
    struct list_head list;
} passenger_queue;

typedef struct passenger{
    int id;
    int weight_int;
    int weight_dec;
    struct list_head list;
} Passenger;

//Our implementation of the start_elevator syscall
extern int (*STUB_start_elevator)(void);
int custom_start_elevator(){

}

//Our implementation of the issue_request syscall
extern int (*STUB_issue_request)(int, int, int);
int custom_issue_request(int passengerType, int startFloor, int destinationFloor){

    Passenger *a;
    a = kmalloc(sizeof(Passenger) * 1, __GFP_RECLAIM);
    a->id = passengerType;

    switch(passengerType){
        //Visitor, 0.5 lbs
        case 0:
            a->weight_int = 0;
            a->weight_dec = 5;
            break;

        //Part-time, 1 lb    
        case 1:
            a->weight_int = 1;
            a->weight_dec = 0;
            break;

        //Lawyers, 1.5 lbs
        case 2:
            a->weight_int = 1;
            a->weight_dec = 5;
            break;
        
        //Boss, 2 lbs
        case 3:
            a->weight_int = 2;
            a->weight_dec = 0;
            break;
        default:
            return -1;
    }

    //Add passenger to end of queue of waiting passengers
    list_add_tail(&a->list, &passenger_queue.list);
    passenger_queue.total_cnt++;

    return 0;
}

//Our implementation of the stop_elevator syscall
extern int (*STUB_stop_elevator)(void);
int custom_stop_elevator(){
    
}

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

    //proc file operations below
    elevator_entry = proc_create(ENTRY_NAME, ENTRY_PERMS, PARENT, &elevator_fops);
    if (elevator_entry == NULL){
        return -ENOMEM;
    }

    //insert rest of init operations here

    //Initializes empty passenger queue
    passenger_queue.total_cnt = 0;
    INIT_LIST_HEAD(&passenger_queue.list);

    //Initializes list for empty elevator
    passengers.total_cnt = 0;
    passengers.total_length = 0;
    passengers.total_weight_int = 0;
    passengers.total_weight_dec = 0;
    INIT_LIST_HEAD(&passengers.list);

    return 0;
}

static void __exit elevator_exit(void){
    //proc file operation below
    proc_remove(elevator_entry);

    //insert rest of exit operations here
}

module_init(elevator_init);
module_exit(elevator_exit);
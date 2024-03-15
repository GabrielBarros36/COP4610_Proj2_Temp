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

//Elevator status options
#define OFFLINE 11
#define IDLE 12
#define LOADING 13
#define UP 14
#define DOWN 15

int start_elevator(void);
int issue_request(int passengerType, int startFloor, int destinationFloor);
int stop_elevator(void);

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

static struct proc_dir_entry* elevator_entry;

struct thread_parameter {

    int status;
    int cur_floor;
    struct task_struct *kthread;

    //Keeps track of the passengers INSIDE the elevator
    struct {
        
        int total_cnt;
        int total_length;
        int total_weight_int;
        int total_weight_dec;
	    int total_serviced;
        struct list_head list;
    } passengerList;

    //Keeps track of the passengers WAITING for the elevator
    struct{
        int total_cnt;
        struct list_head list;
    } passenger_queue;

};

struct thread_parameter elevator;

typedef struct{
    int curFloor;
    int destFloor;
    int id;
    int weight_int;
    int weight_dec;
    struct list_head list;
} Passenger;

//Our implementation of the start_elevator syscall
int custom_start_elevator(void){
        return 0;
}

//Our implementation of the issue_request syscall
//Creates a passenger struct with the passenger's weight and adds that struct
//to the passenger queue
int custom_issue_request(int passengerType, int startFloor, int destinationFloor){

    Passenger *a;
    a = kmalloc(sizeof(Passenger) * 1, __GFP_RECLAIM);

    //Avoids kernel panic if a is null
    if(!a){
        return -ENOMEM;
    }

    a->id = passengerType;
    a->curFloor = startFloor;
    a->destFloor = destinationFloor;

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
    list_add_tail(&a->list, &elevator.passenger_queue.list);
    elevator.passenger_queue.total_cnt++;

    return 0;
}

//Our implementation of the stop_elevator syscall
int custom_stop_elevator(void){
    return 0;
}

//Implementation of the elevator algorithm
int elevator_run(void *data){

    //Elevator algorithm goes here
    return 0;

}

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[10000];
    int len = 0;

    len = sprintf(buf, "Elevator state: \n");     /* Add elevator state when implemented*/
    len += sprintf(buf + len, "Current floor: %d\n", elevator.cur_floor);
    len += sprintf(buf + len, "Current load: %d.%d\n", elevator.passengerList.total_weight_int, elevator.passengerList.total_weight_dec);
    len += sprintf(buf + len, "Elevator status:");     /* Add elevator status (passenger types and destination floor) when implemented*/

    Passenger *pass;	//pointers to iterate through passenger list
    Passenger *pass2;

    list_for_each_entry(pass, &elevator.passengerList.list, list) {	//Print elevator status
	int passengerType = pass->id;
	int passengerDest = pass->destFloor;	//Waiting for Edgar's commit that includes destination_floor

	len += sprintf(buf + len, " %d%d", passengerType, passengerDest);
    }

    len += sprintf(buf + len, "\n");

    for(int i = 5; i > 0; i--){
        if(i == elevator.cur_floor) {
            len += sprintf(buf + len, "[*] Floor %d:", i);
            /* Print waiting passengers type and destination floor after "[*] Floor cur_floor:"*/
	    len += sprintf(buf + len, " %d", elevator.passenger_queue.total_cnt);

        }else{
            len += sprintf(buf + len, "[ ] Floor %d:", i);
	    /* Print waiting passengers type and destination floor after "[ ] Floor cur_floor:"*/
	    len += sprintf(buf + len, " %d", elevator.passenger_queue.total_cnt);
	}

	list_for_each_entry(pass2, &elevator.passenger_queue.list, list) {
        int passengerType = pass2->id;
        int passengerDest = pass2->destFloor;

        if(passengerDest == i){
			len += sprintf(buf + len, " %d%d", passengerType, passengerDest); 
		}
	}

	len += sprintf(buf + len, "\n");

    }
    // you can finish the rest.

    // not sure what else needs to be done here

    len += sprintf(buf + len, "Number of passengers: %d\n", elevator.passengerList.total_cnt);
    len += sprintf(buf + len, "Number of passengers waiting: %d\n", elevator.passenger_queue.total_cnt);
    len += sprintf(buf + len, "Number of passengers serviced:\n");   /* Print passengers serviced once implemented"*/

    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

static ssize_t elevator_write(struct file* file, const char __user* ubuf, size_t count, loff_t* ppos) {
    char buf[100];

    if (*ppos > 0 || count > 100)
        return -EFAULT;

    if (copy_from_user(buf, ubuf, count))
        return -EFAULT;

    printk(KERN_INFO "Data written to /proc/elevator: %s\n", buf); //test message for proc entry

    *ppos = count;
    return count;
}


static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
    .proc_write = elevator_write,
};

void thread_init_parameter(struct thread_parameter *param){
     //Initializes empty passenger queue
    param->passenger_queue.total_cnt = 0;
    INIT_LIST_HEAD(&param->passenger_queue.list);

    //Initializes list for empty elevator
    param->passengerList.total_cnt = 0;
    param->passengerList.total_length = 0;
    param->passengerList.total_weight_int = 0;
    param->passengerList.total_weight_dec = 0;
    param->passengerList.total_serviced = 0;
    INIT_LIST_HEAD(&param->passengerList.list);

    //thread operations
    param->kthread = kthread_run(elevator_run, param, "running elevator thread");
    
}


static int __init elevator_init(void){

    //Mapping syscalls to our custom syscalls
    STUB_start_elevator = custom_start_elevator;
    STUB_issue_request = custom_issue_request;
    STUB_stop_elevator = custom_stop_elevator;

    //proc file operations below
    elevator_entry = proc_create(ENTRY_NAME, ENTRY_PERMS, PARENT, &elevator_fops);
    if (elevator_entry == NULL){
        return -ENOMEM;
    }

    //More proc file operations to be done in here

    //Thread operations
    thread_init_parameter(&elevator);
    if (IS_ERR(elevator.kthread)) {
		printk(KERN_WARNING "error spawning thread");
		remove_proc_entry(ENTRY_NAME, NULL);
		return PTR_ERR(elevator.kthread);
	}

    return 0;
}

static void __exit elevator_exit(void){

    kthread_stop(elevator.kthread);

    //proc file operation below
    proc_remove(elevator_entry);

    STUB_start_elevator = NULL;
	STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;
}

module_init(elevator_init);
module_exit(elevator_exit);

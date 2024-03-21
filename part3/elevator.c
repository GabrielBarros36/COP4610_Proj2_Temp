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
#include <linux/delay.h>

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

    int state;
    int cur_floor;
    int dest_floor;
    struct task_struct *kthread;
    struct mutex mutex;

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
        int waiting_passengers[ELEVATOR_LIMIT];
        struct list_head list;
    } passenger_queue;

};

struct thread_parameter elevator;

typedef struct{
    int curFloor;
    int destFloor;
    int startFloor;
    int id;
    int weight_int;
    int weight_dec;
    struct list_head list;
} Passenger;

//Our implementation of the start_elevator syscall
int custom_start_elevator(void){

    if(elevator.state != OFFLINE){
        printk(KERN_INFO "Elevator Already Online!\n");
        return 1;
    }
        
    if (mutex_lock_interruptible(&elevator.mutex) == 0){
        elevator.state = IDLE; //setting idle state
        elevator.cur_floor = 1;

        INIT_LIST_HEAD(&elevator.passengerList.list); //Initializing passenger list and setting to 0
        elevator.passengerList.total_cnt = 0;
        elevator.passengerList.total_weight_int = 0;
        elevator.passengerList.total_weight_dec = 0;

        INIT_LIST_HEAD(&elevator.passenger_queue.list); //Initializing passenger queue and setting to 0
        elevator.passenger_queue.total_cnt = 0;
        
        for (int i = 0; i < ELEVATOR_MAX_FLOOR; i++){ //using a for loop to reset waiting passengers for each floor
            elevator.passenger_queue.waiting_passengers[i] = 0;
        }

        mutex_unlock(&elevator.mutex);
        printk(KERN_INFO "Elevator Online!");
    } else {
        printk(KERN_ALERT "Issue in locking elevator mutex! (Starting)\n");
    }

    return 0;

}

//checks that the count and weight is not exceeded
void load_elevator(void){
    struct list_head *temp, *dummy;
    Passenger *p;

    if(mutex_trylock(&elevator.mutex) != 0)
        mutex_unlock(&elevator.mutex);
    else
        mutex_unlock(&elevator.mutex);

    if(mutex_lock_interruptible(&elevator.mutex) == 0){
        list_for_each_safe(temp, dummy, &elevator.passenger_queue.list){
            p = list_entry(temp, Passenger, list);

            if (p -> startFloor == elevator.cur_floor &&
                elevator.passengerList.total_cnt < ELEVATOR_LIMIT &&
                (elevator.passengerList.total_weight_int + p -> weight_int) +
                (elevator.passengerList.total_weight_dec + p -> weight_dec) / 10 <= WEIGHT_LIMIT) {

                elevator.passengerList.total_weight_int += p -> weight_int;
                elevator.passengerList.total_weight_dec += p -> weight_dec;

                elevator.passengerList.total_cnt++;
                elevator.passenger_queue.total_cnt--;        //update queue cnt

                list_move_tail(temp, &elevator.passengerList.list);

                printk(KERN_INFO "Passenger Sucessfully Loaded, Start Floor: %d to Destination Floor %d\n", p -> startFloor, p -> destFloor);
            }
        }
        mutex_unlock(&elevator.mutex);
    } else {
        printk(KERN_ALERT "Failure in locking when loading elevator. \n");
    }
}

void unload_elevator(void) {
    struct list_head *temp, *dummy;
    Passenger *p;

    if(mutex_trylock(&elevator.mutex) != 0)
        mutex_unlock(&elevator.mutex);
    else
        mutex_unlock(&elevator.mutex);

    if(mutex_lock_interruptible(&elevator.mutex) == 0) {
        list_for_each_safe(temp, dummy, &elevator.passengerList.list) {
            p = list_entry(temp, Passenger, list);

            if(p->destFloor == elevator.cur_floor) {
                elevator.passengerList.total_weight_int -= p->weight_int;
                elevator.passengerList.total_weight_dec -= p->weight_dec;

                elevator.passengerList.total_cnt--;

                list_del(temp);
                kfree(p);

                elevator.passengerList.total_serviced++;    //incriment total_serviced
            }
        }
        mutex_unlock(&elevator.mutex);
    }
}

int find_next_possible_floor(void) {
    int closest_floor_up = ELEVATOR_MAX_FLOOR + 1;
    int closest_floor_down = -1;

    struct list_head *temp;
    Passenger *p;

    // Passengers Inside
    list_for_each(temp, &elevator.passengerList.list) {
        p = list_entry(temp, Passenger, list);
        if (p -> destFloor > elevator.cur_floor && p -> destFloor < closest_floor_up) {
            closest_floor_up = p -> destFloor;
        } else if (p -> destFloor < elevator.cur_floor && p -> destFloor > closest_floor_down) {
            closest_floor_down = p -> destFloor;
        }
    }

        // CHECKING WAITING PASSENGERS
    if(elevator.state != OFFLINE){
        list_for_each(temp, &elevator.passenger_queue.list) {               //changed to only allow waiting passengers if state is not offline
            p = list_entry(temp, Passenger, list);
            if (p -> startFloor > elevator.cur_floor && p -> startFloor < closest_floor_up) {
                closest_floor_up = p -> startFloor;
            } else if (p -> startFloor < elevator.cur_floor && p -> startFloor > closest_floor_down) {
                closest_floor_down = p -> startFloor;
            }
        }
    }

    bool hasClosestFloorUp = (closest_floor_up != ELEVATOR_MAX_FLOOR + 1);
    bool hasClosestFloorDown = (closest_floor_down != -1);
    int next_floor = -1;

    // NEXT FLOOR BASED ON STATE
    if (elevator.state == UP && hasClosestFloorUp) {
        next_floor = closest_floor_up;
    } else if (elevator.state == DOWN && hasClosestFloorDown) {
        next_floor = closest_floor_down;
    } else if (elevator.state == IDLE) {
        next_floor = hasClosestFloorUp ? closest_floor_up : (hasClosestFloorDown ? closest_floor_down : -1);
    }

    return next_floor;
}

//Our implementation of the issue_request syscall
//Creates a passenger struct with the passenger's weight and adds that struct
//to the passenger queue
int custom_issue_request(int startFloor, int destinationFloor, int passengerType){

    Passenger *a;
    a = kmalloc(sizeof(Passenger) * 1, __GFP_RECLAIM);

    //Avoids kernel panic if a is null
    if(!a){
        return -ENOMEM;
    }

    a->id = passengerType;
    a->curFloor = startFloor;
    a->startFloor = startFloor;
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

    if(elevator.state == OFFLINE){
        printk(KERN_INFO "Elevator already Offline!\n");
        return 0;
    }

    if (mutex_lock_interruptible(&elevator.mutex) == 0){
        elevator.state = OFFLINE; //setting offline state
        mutex_unlock(&elevator.mutex);

        unload_elevator();

        while (elevator.passengerList.total_cnt != 0){
            int next_floor = find_next_possible_floor();
            if (next_floor != -1) {
                elevator.cur_floor = next_floor;
                mutex_unlock(&elevator.mutex);
                unload_elevator();
                msleep(1000);
            } else {
                break;
            }
        }
           
        printk(KERN_INFO "Elevator Offline!\n");
    } else {
        printk(KERN_ALERT "Issue in locking elevator mutex! (Stopping)");
    } 

    return 0;

}

//Implementation of the elevator algorithm
int elevator_run(void *data){

    struct thread_parameter *elevator = data;

    printk(KERN_INFO "Elevator thread started.\n");


    // while(!kthread_should_stop()){
    //     if (mutex_lock_interruptible(&elevator->mutex) != 0) {
    //         continue;
    //     }

    //     if(elevator->state == IDLE){
    //         if (elevator->passenger_queue.total_cnt > 0 || elevator->passengerList.total_cnt > 0) {
    //                 elevator->state = LOADING;
    //             }
    //     }else if(elevator->state == LOADING){
    //         struct list_head *temp, *dummy;
    //         Passenger *p;

    //         if(mutex_trylock(&elevator.mutex) != 0)
    //             mutex_unlock(&elevator);
    //         else
    //             mutex_unlock(&elevator);

    //         if(mutex_lock_interruptible(&elevator.mutex) == 0){
    //             list_for_each_safe(temp, dummy, &elevator.passenger_queue.list){
    //                 p = list_entry(temp, Passenger, list);

    //                 if (p -> startFloor == elevator.cur_floor &&
    //                     elevator.passengerList.total_cnt < ELEVATOR_LIMIT &&
    //                     (elevator.passengerList.total_weight_int + p -> weight_int) +
    //                     (elevator.passengerList.total_weight_dec + p -> weight_dec) / 10 <= WEIGHT_LIMIT) {

    //                     elevator.passengerList.total_weight_int += p -> weight_int;
    //                     elevator.passengerList.total_weight_dec += p -> weight_dec;

    //                     elevator.passengerList.total_cnt++;
    //                     elevator.passenger_queue.total_cnt--;        //update queue cnt

    //                     list_move_tail(temp, &elevator.passengerList.list);

    //                     printk(KERN_INFO "Passenger Sucessfully Loaded, Start Floor: %d to Destination Floor %d\n", p -> startFloor, p -> destFloor);
    //                 }
    //             }
    //             mutex_unlock(&elevator.mutex);
    //         } else {
    //             printk(KERN_ALERT "Failure in locking when loading elevator. \n");
    //         }
    //     }

        
    // }

    while (!kthread_should_stop()) {
        if (mutex_lock_interruptible(&elevator->mutex) != 0) {
            continue;
        }

        switch (elevator->state) {
            case IDLE:
                // Check for passengers or decide the next move
                if (elevator->passenger_queue.total_cnt > 0 || elevator->passengerList.total_cnt > 0) {
                    elevator->state = LOADING;
                }
                break;

            case LOADING:

                if(elevator->passenger_queue.total_cnt != 0){
                    load_elevator();
                }

                // if(elevator->passengerList.total_cnt != 0){
                //     //unload_elevator();

                // }

                if (elevator->passenger_queue.total_cnt > 0) {
                    int next_floor = (elevator->cur_floor == 1 ? 5 : elevator->cur_floor - 1);                         //int next_floor = find_next_possible_floor(); (TESTING SIMPLER ALG TO FIND ISSUES)
                    if (next_floor != -1) {
                        elevator->dest_floor = next_floor;
                        elevator->state = (next_floor > elevator->cur_floor) ? UP : DOWN;
                    }
                } else {
                    elevator->state = IDLE;
                }
                break;

            case UP:
            case DOWN:
                // Move the elevator to the next floor
                if (elevator->cur_floor != elevator->dest_floor) {
                    // Movement
                    msleep(1000); // 1 second

                    elevator->cur_floor += (elevator->state == UP) ? 1 : -1;
                   
                    /*Update elevator.passengerList.curFloor*/
                    Passenger *pass;
                    list_for_each_entry(pass, &elevator->passengerList.list, list){
                        (elevator->state == UP) ? ++pass->curFloor : --pass->curFloor;
                    }
                    printk(KERN_INFO "Elevator moved to floor %d.\n", elevator->cur_floor);
                }

                // Check
                if (elevator->cur_floor == elevator->dest_floor) {
                    elevator->state = LOADING;
                }
                break;

            default:
                printk(KERN_WARNING "Elevator encountered an unhandled state.\n");
                break;
        }

        mutex_unlock(&elevator->mutex);
    }

    printk(KERN_INFO "Elevator thread stopping.\n");
    return 0;

}

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char *buf = kmalloc(10000, GFP_KERNEL); // Allocate memory on the heap
    if (!buf)
        return -ENOMEM; // Return an error if allocation failed

    int len = 0;

    char* state = "";

    switch(elevator.state){
        case OFFLINE:
            state = "OFFLINE";
            break;
        case LOADING:
            state = "LOADING";
            break;
        case UP: 
            state = "UP";
            break;
        case DOWN:
            state = "DOWN";
            break;
        case IDLE:
            state = "IDLE";
            break;
        default:
            printk(KERN_WARNING "Error: Unknown Elevator State");	
    }

    len = sprintf(buf, "Elevator state: %s\n", state);     /* Add elevator state when implemented */
    len += sprintf(buf + len, "Current floor: %d\n", elevator.cur_floor);
    len += sprintf(buf + len, "Current load: %d.%d\n", elevator.passengerList.total_weight_int, elevator.passengerList.total_weight_dec);
    len += sprintf(buf + len, "Elevator status:");     /* Add elevator status (passenger types and destination floor) when implemented */

    Passenger *pass;    //pointers to iterate through passenger list

    char passengerTypes[4] = {'V', 'P', 'L', 'B'}; 

    list_for_each_entry(pass, &elevator.passengerList.list, list) {    //Print elevator status
        char passengerType;
        if(pass->id >= 0 && pass->id <= 3) {
            int index = pass->id;
        	passengerType = passengerTypes[index];
        }else{
                printk(KERN_WARNING "Unknown Passenger Type");
        }

        int passengerDest = pass->destFloor;    

        len += sprintf(buf + len, " %c%d", passengerType, passengerDest);
    }

    len += sprintf(buf + len, "\n");

    for(int i = 5; i > 0; i--){
        if(i == elevator.cur_floor) {
            len += sprintf(buf + len, "[*] Floor %d:", i);
            /*Prints: "[*] Floor cur_floor:"*/
        } else {
            len += sprintf(buf + len, "[ ] Floor %d:", i);
            /*Prints: "[ ] Floor cur_floor:"*/
    	}

        Passenger *pass2, *temp;
        int perFloor = 0;
        list_for_each_entry(temp, &elevator.passenger_queue.list, list){    //Calculate waiting_passengers per floor
            if(temp->startFloor == i){
                 perFloor++; 
            }
                elevator.passenger_queue.waiting_passengers[i-1] = perFloor;
        }

        len += sprintf(buf + len, " %d", elevator.passenger_queue.waiting_passengers[i-1]);

        list_for_each_entry(pass2, &elevator.passenger_queue.list, list) {

	        if(pass2->startFloor == i) {
                char passengerType;
		        if(pass2->id >= 0 && pass2->id <= 3) {
                    int index = pass->id;
		        	passengerType = passengerTypes[index];
		        }else{
		    	    printk(KERN_WARNING "Unknown Passenger Type");
		        }

                int passengerDest = pass2->destFloor;
		        len += sprintf(buf + len, " %c%d ", passengerType, passengerDest);
	        }   
        }

        len += sprintf(buf + len, "\n");
    }
    // Additional logic here...

    len += sprintf(buf + len, "Number of passengers: %d\n", elevator.passengerList.total_cnt);
    len += sprintf(buf + len, "Number of passengers waiting: %d\n", elevator.passenger_queue.total_cnt);
    len += sprintf(buf + len, "Number of passengers serviced: %d\n", elevator.passengerList.total_serviced);   /* Print passengers serviced once implemented */

    ssize_t result = simple_read_from_buffer(ubuf, count, ppos, buf, len);
    kfree(buf); // Free the allocated memory
    return result;
}


static ssize_t elevator_write(struct file* file, const char __user* ubuf, size_t count, loff_t* ppos) {
    char *buf;

    buf = kmalloc(count, GFP_KERNEL);
    if(!buf){
	printk(KERN_ERR "memory allocation failed in elevator_write()\n");
	return -ENOMEM;
    }

    if (*ppos != 0 || count > 100){
	kfree(buf);
        return -EINVAL;
    }

    if (copy_from_user(buf, ubuf, count)){
	kfree(buf);
        return -EFAULT;
    }

    buf[count - 1] = '\0';	


    printk(KERN_INFO "Data written to /proc/elevator: %s\n", buf); //test message for proc entry

    kfree(buf);

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

    if(IS_ERR(param->kthread)){
        printk(KERN_WARNING "Error spawning thread (thread_init_parameter)");
        return;
    }
    
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
		printk(KERN_WARNING "Error spawning thread (elevator_init)");
		remove_proc_entry(ENTRY_NAME, NULL);
		return PTR_ERR(elevator.kthread);
	}

    elevator.state = OFFLINE;

    return 0;
}

static void __exit elevator_exit(void){

    if(elevator.kthread) {
        kthread_stop(elevator.kthread);
        elevator.kthread = NULL;
    }

    //proc file operation below
    proc_remove(elevator_entry);

    STUB_start_elevator = NULL;
	STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;
}

module_init(elevator_init);
module_exit(elevator_exit);

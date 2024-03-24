#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

int (*STUB_start_elevator)(void) = NULL;
int (*STUB_issue_request)(int, int, int) = NULL;
int (*STUB_stop_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_start_elevator);
EXPORT_SYMBOL(STUB_issue_request);
EXPORT_SYMBOL(STUB_stop_elevator);


SYSCALL_DEFINE0(start_elevator){
    printk(KERN_NOTICE "Inside SYSCALL_DEFINE0 block. %s", __FUNCTION__);
    if(STUB_start_elevator != NULL)
        return STUB_start_elevator();
    else
        return -ENOSYS;
}

SYSCALL_DEFINE3(issue_request, int, first_num, int, second_num, int, third_num){
    printk(KERN_NOTICE "Inside SYSCALL_DEFINE0 block. %s: Your int are %d, %d, %d\n", __FUNCTION__, first_num, second_num, third_num);
    if(STUB_issue_request != NULL)
        return STUB_issue_request(first_num, second_num, third_num);
    else
        return -ENOSYS;
}


SYSCALL_DEFINE0(stop_elevator){
    printk(KERN_NOTICE "Inside SYSCALL_DEFINE0 block. %s", __FUNCTION__);
    if(STUB_stop_elevator != NULL)
        return STUB_stop_elevator();
    else
        return -ENOSYS;
}

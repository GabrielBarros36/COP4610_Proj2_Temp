#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>

int main(){

    pid_t pid = fork();

    wait(NULL);
    wait(NULL);

    pid_t pid2 = fork();

    return 0;

}

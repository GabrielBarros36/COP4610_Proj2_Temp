# Shell

A program written in C that emulates the basic functionalities of a UNIX shell. 

## Group Number
7

## Group Name
OS Group

## Group Members
- **Edgar Guerrero**: eg21v@fsu.edu
- **Gabriel Romanini de Barros**: gr22d@fsu.edu
- **Lucas Smith**: lps21a@fsu.edu
## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: Make an "empty" C program and then a program which produces exactly five more system calls than the aforementioned empty program.
- **Assigned to**: Edgar Guerrero, Lucas Smith

### Part 2: Timer Kernel Module
- **Responsibilities**: Make a "timer.ko" kernel module that once loaded to the kernel produces the following behavior: the first time a user uses the command "cat proc/timer" it will print out the current UNIX Epoch time in a "seconds.nanoseconds" format, and in all subsequent times the user runs the commands it should print out the current time as well as the difference from the last time the file was printed, in the same format.
- **Assigned to**: Gabriel Romanini de Barros

### Part 3a: Adding System Calls
- **Assigned to**: Gabriel Romanini de Barros, Edgar Guerrero 

### Part 3b: Kernel Compilation
- **Assigned to**: Gabriel Romanini de Barros, Lucas Smith

### Part 3c: Threads
- **Assigned to**: Edgar Guerrero, Lucas Smith

### Part 3d: Linked List
- **Assigned to**: Edgar Guerrero

### Part 3e: Mutexes
- **Assigned to**: Lucas Smith

### Part 3f: Scheduling Algorithm
- **Assigned to**: Gabriel Romanini de Barros

## File Listing
```
COP4610_Proj2_Temp/
│
├── part1/
│ ├── empty.c
│ ├── empty.trace
│ ├── part1.c
│ └── part1.trace
│
├── part2/src/
│ ├── my_timer.c
│ └── Makefile
│
├── part3/
│ ├── elevator.c
│ ├── syscalls.c
│ └── Makefile
│
└── README.md 
```
## How to Compile & Execute

### Compilation
For C:
```bash
make
```
Run this in the "part2" or "part3" folders, depending on which system module you want to compile.

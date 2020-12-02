#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"
#include "threads/thread.h"
void syscall_init (void);

#define ERROR -1
#define LOADED 1
#define LOAD_FAIL 2
#define NOT_LOADED 0
#define CLOSE_ALL -1

struct exitcode {
    int pid;
    int status;
    struct list_elem elem;
};

struct openfile {
    struct file *file;
    int fd;
    struct list_elem elem;
};

struct semaphore filelock;

void syscall_close(int fd);
void syscall_exit (int status);

int getpage_ptr (const void *vaddr);
struct exitcode* find_exitcode (int pid);
struct file* find_file(int fd);

#endif /* userprog/syscall.h */
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
void get_args (struct intr_frame *f, int *arg, int num_of_args);
void syscall_halt (void);
pid_t syscall_exec(const char* cmdline);
int syscall_wait(pid_t pid);
bool syscall_create(const char* file_name, unsigned starting_size);
bool syscall_remove(const char* file_name);
int syscall_open(const char * file_name);
int syscall_filesize(int fd);
int syscall_read(int fd, void *buffer, unsigned length);
int syscall_write (int fd, const void * buffer, unsigned byte_size);
void syscall_seek (int fd, unsigned new_position);
unsigned syscall_tell(int fildes);
void check_ptr (const void* vaddr);
void check_str (const void* str);


struct semaphore writelock;
struct semaphore mutex;
int readcount;

void
syscall_init (void)
{
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
    sema_init(&writelock, 1);
    sema_init(&mutex, 1);
    readcount = 0;
}

/*
 * This method handles for various case of system command.
 * This handler invokes the proper function call to be carried
 * out base on the command line.
 */
static void
syscall_handler (struct intr_frame *f UNUSED)
{
    int arg[3];
    int esp = getpage_ptr((const void *) f->esp);

    switch (* (int *) esp)
    {
        case SYS_HALT:
            syscall_halt();
            break;

        case SYS_EXIT:
            get_args(f, &arg[0], 1);
            syscall_exit(arg[0]);
            break;

        case SYS_EXEC:
            get_args(f, &arg[0], 1);

            // check if command line is valid
            check_str((const void*)arg[0]);

            // get page pointer
            arg[0] = getpage_ptr((const void *)arg[0]);
            /* syscall_exec(const char* cmdline) */
            f->eax = syscall_exec((const char*)arg[0]); // execute the command line
            break;

        case SYS_WAIT:
            get_args(f, &arg[0], 1);
            f->eax = syscall_wait(arg[0]);
            break;

        case SYS_CREATE:
            get_args(f, &arg[0], 2);
            // get page pointer
            arg[0] = getpage_ptr((const void *) arg[0]);
            /* syscall_create(const char* file_name, unsigned starting_size) */
            f->eax = syscall_create((const char *)arg[0], (unsigned)arg[1]);  // create this file
            break;

        case SYS_REMOVE:
            get_args(f, &arg[0], 1);
            arg[0] = getpage_ptr((const void *) arg[0]);

            /* syscall_remove(const char* file_name) */
            f->eax = syscall_remove((const char *)arg[0]);  // remove this file
            break;

        case SYS_OPEN:
            get_args(f, &arg[0], 1);

            /* Check if command line is valid.
             * We do not want to open junk which can cause a crash
             */
            check_str((const void*)arg[0]);

            // get page pointer
            arg[0] = getpage_ptr((const void *)arg[0]);

            /* syscall_open(int fd) */
            f->eax = syscall_open((const char *)arg[0]);  // open this file
            break;

        case SYS_FILESIZE:
            get_args(f, &arg[0], 1);

            /* syscall_filesize (const char *file_name) */
            f->eax = syscall_filesize(arg[0]);  // obtain file size
            break;

        case SYS_READ:
            get_args(f, &arg[0], 3);
            // get page pointer
            arg[1] = getpage_ptr((const void *)arg[1]);
            /*syscall_read(int fd, void *buffer, unsigned length)*/
            f->eax = syscall_read(arg[0], (void *) arg[1], (unsigned) arg[2]);
            break;

        case SYS_WRITE:
            get_args(f, &arg[0], 3);

            /* Check if the buffer is valid.
             * We do not want to mess with a buffer that is out of our
             * reserved virtual memory
             */

            ((const void*)arg[1], (unsigned)arg[2]);

            // get page pointer
            arg[1] = getpage_ptr((const void *)arg[1]);

            /* syscall_write (int fd, const void * buffer, unsigned bytes)*/
            f->eax = syscall_write(arg[0], (const void *) arg[1], (unsigned) arg[2]);
            break;

        case SYS_SEEK:
            get_args(f, &arg[0], 2);
            /* syscall_seek(int fd, unsigned new_position) */
            syscall_seek(arg[0], (unsigned)arg[1]);
            break;

        case SYS_TELL:
            get_args(f, &arg[0], 1);
            f->eax = syscall_tell(arg[0]);
            break;

        case SYS_CLOSE:
            get_args (f, &arg[0], 1);
            syscall_close(arg[0]);
            break;

        default:
            break;
    }
}

/* get arguments from stack */
void
get_args (struct intr_frame *f, int *args, int num_of_args)
{
    int i;
    int *ptr;
    for (i = 0; i < num_of_args; i++)
    {
        ptr = (int *) f->esp + i + 1;
        check_ptr((const void *) ptr);
        args[i] = *ptr;
    }
}

void
syscall_halt (void)
{
    shutdown_power_off();
}
void
syscall_exit (int status)
{
    status = status < 0? -1 : status;
    struct thread *cur = thread_current();
    if (cur->ret)
        cur->ret->status = status;
    printf("%s: exit(%d)\n", cur->name, status);
    thread_exit(); //call process_exit()
}

pid_t
syscall_exec(const char* cmdline)
{
    // executes the command line
    pid_t pid = process_execute(cmdline);
    struct thread* t = find_thread(pid);

    if (t->load_status == NOT_LOADED)
        sema_down(&t->load_sema);

    if (t->load_status < 0)
        return ERROR;
    return pid;
}

int
syscall_wait(pid_t pid)
{
    return process_wait(pid);
}

bool
syscall_create(const char* file_name, unsigned starting_size)
{
    return filesys_create(file_name, starting_size);
}

bool
syscall_remove(const char* file_name)
{
    return filesys_remove(file_name);
}

int
syscall_open(const char *file_name)
{
    struct file *fp = filesys_open(file_name);
    if (!fp) return ERROR;

    /* add file to file list */
    struct openfile *process_fp = malloc(sizeof(struct openfile));
    if (!process_fp) return ERROR;

    process_fp->file = fp;
    process_fp->fd = thread_current()->fd;
    list_push_back(&thread_current()->file_list, &process_fp->elem);
    thread_current()->fd++;

    return process_fp->fd;
}

int
syscall_filesize(int fd)
{
    return file_length(find_file(fd));
}

int
syscall_read(int fd, void *buffer, unsigned size)
{
  uint8_t* buf = buffer;
  unsigned count = size;
  int value = 0;

  sema_down(&mutex);
  readcount++;
  if(readcount == 1) sema_down(&writelock);
  sema_up(&mutex);

  if(fd == 0) { //no getbuf()
    while(count--){
      int temp = input_getc();
      if (temp != NULL)
        buf[size - count - 1] = temp;
      else break;
    }
    value = size - count;
  }
  else value = file_read(find_file(fd),buffer,size);

  sema_down(&mutex);
  readcount--;
  if(readcount == 0) sema_up(&writelock);
  sema_up(&mutex);

  return value;
}

int
syscall_write (int fd, const void * buffer, unsigned byte_size)
{
    int value = 0;
    if (byte_size <= 0) 
        return byte_size;

    sema_down(&writelock);
    if (fd == 1) { //OUTPUT
        putbuf (buffer, byte_size);
        value = byte_size;
    }
    else 
        value = file_write(find_file(fd), buffer, byte_size);
    
    sema_up(&writelock);
    return value;
}

void
syscall_seek (int fd, unsigned new_position)
{
    file_seek(find_file(fd), new_position);
}

unsigned
syscall_tell(int fd)
{
    return file_tell(find_file(fd));
}

void
syscall_close(int fd)
{
    struct thread *t = thread_current();
    struct list_elem *next;
    struct list_elem *e = list_begin(&t->file_list);
    
    for (;e != list_end(&t->file_list); e = next)
    {
        next = list_next(e);
        struct openfile *process_file_ptr = list_entry (e, struct openfile, elem);
        if (fd == process_file_ptr->fd || fd == CLOSE_ALL_FD)
        {
            file_close(process_file_ptr->file);
            list_remove(&process_file_ptr->elem);
            free(process_file_ptr);
            if (fd != CLOSE_ALL_FD)
            {
                return;
            }
        }
    }
}


//===========================================================

void
check_ptr (const void *vaddr)
{
    if (vaddr < USER_VADDR_BOTTOM || !is_user_vaddr(vaddr))
        syscall_exit(ERROR);
}

void
check_str (const void* str)
{
    for (; *(char*)getpage_ptr(str) != 0; str = (char *)str + 1);
}


/* get the pointer to page */
int
getpage_ptr(const void *vaddr)
{
    if (vaddr >= (void *)0xc0000000)
        syscall_exit(ERROR);
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    if (!ptr)
    {
        syscall_exit(ERROR);
    }
    return (int)ptr;
}

struct exitcode* find_exitcode(int pid)
{
    struct thread *t = thread_current();
    struct list_elem *e;
    struct list_elem *next;

    for (e = list_begin(&t->exitcode_list); e != list_end(&t->exitcode_list); e = next)
    {
        next = list_next(e);
        struct exitcode *ret = list_entry(e, struct exitcode, elem);
        if (pid == ret->pid)
        {
            return ret;
        }
    }
    return NULL;
}

struct file* find_file (int fd)
{
    struct thread *t = thread_current();
    struct list_elem* next;
    struct list_elem* e = list_begin(&t->file_list);

    for (; e != list_end(&t->file_list); e = next)
    {
        next = list_next(e);
        struct openfile *process_file_ptr = list_entry(e, struct openfile, elem);
        if (fd == process_file_ptr->fd)
        {
            return process_file_ptr->file;
        }
    }
    return NULL;
}

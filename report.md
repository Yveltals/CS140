# Project 2 Report

## 小组成员

- 冯玮琪
- 马壮
- 黄培森
- 钱琅毓



## Task1:Argument Passing

### Data structures

#### **A1:** 

**（1）在thread.h中**：

```c++
struct exitcode* ret; 				//记录自身退出状态，销毁thread后仍存在
struct list exitcode_list;  //记录子线程退出状态，子线程 exitcode 的列表
```

**（2）在syscall.h中：**

```c++
struct exitcode {
    int pid;				 //对应线程tid
    int status;		//退出状态，异常情况-1
    struct list_elem elem; //用以保存到父线程 exitcode_list 中
};
```



### Algorithm

#### **A2:**   

我们通过调用process_execute函数来创建用户级进程。我们需要实现参数传递使其支持命令行参数。为此，我们需要将命令行参数传递进load函数和setup_stack函数，并将这些参数按顺序推入栈中。

- 在process_execute函数中，我们用strtok_r函数来分割文件名和参数，并以此来创建子线程
- 在start_process函数中，我们分割出可执行文件名，并将其传递进函数load中
- 在load函数中，我们打开文件，并将file_name传入setup_stack函数中
- 在setup_stack函数中，我们分割file_name，计算出argc，为argv分配空间后将参数复制进argv并将它们的地址推入栈中，最后将argv的地址、argc的地址和返回地址推入栈中



###  Rationale

#### **A3:**   

用strtok_r来代替strtok是为了线程安全的并发解析。
在这个任务中，我们分隔输入的参数，将它们传入load函数中并以正确的顺序压入栈中。

#### **A4:**   

(1)提高内核处理效率

(2)能在传递到内核前对执行文件进行检查，避免参数非法造成内核崩溃



## Task2: System Calls

### Data structures

#### **B1:**

**（1）在thread.h中**：

```c++
int wait;                           /* 标记线程是否被等待（仅可被等待一次） */
int exit;                           /* 标记线程是否已结束退出 */
struct semaphore load_sema;  /* 用于thread_create()，子线程创建完成前令父线程等待 */  
struct semaphore exit_sema;   /* 用于process_wait()，子线程退出前令父线程等待 */

int load_status;              /* 文件打开状态  1：已打开  0：未打开  2：打开失败  */
int fd;                            		  /* 文件描述符 */
struct file* myFile;      /* 不能写入执行文件 */
struct list file_list;      /* 记录打开的文件列表 */
```

**（2）在syscall.h中：**

```c++
struct semaphore filelock; /* 读写信号量，防止同时读写引发冲突 */

struct openfile {
    struct file *file;									/* 打开的文件指针 */
    int fd;															  		/* 文件描述符 */
    struct list_elem elem; /* 用于存入file_list */
};
```

####  **B2:**  

文件描述符对每个文件都有一对一的映射，如果一个文件被打开多次时会返回新的文件描述符，在整个操作系统中是唯一的。



### Algorithm

#### **B3：**

**(1)Read**

1. 检查堆栈指针是否对读和写都有效

2. 获取`filelock`(文件系统信号量)

3. 如果fd是0，则从标准输入读取，调用`input_getc()`；

    否则，从`file_list`列表中根据fd编号查找打开的文件，然后使用`file_read()`读取文件

4. 此后释放信号量并返回状态

**(2)Write**

1. 检查堆栈指针是否对读和写都有效

2. 获取`filelock`(文件系统信号量)

3. 如果fd是1，使用`putbuf()`将缓冲区的内容打印到控制台；

    否则，从`file_list`列表中根据fd编号查找打开的文件，然后使用`file_write()`将缓冲区写入文件

4. 此后释放信号量并返回状态



#### **B4：**

**(1)对于整页的数据:**

最少1次：如果检查返回一个页面头，就可以从地址推测得知可以包含一个页面数据，而不需要再检查。


最多4096次：如果不连续的，就必须检查每个地址，以确保有效的访问。

**(2)对于2字节的数据:**

最少1次：同上


最多2次：无论是否连续，当我们查到一个内核虚拟地址离页面的结尾只有1个字节，只须检查另一个字节的位置即可。



#### **B5:**

调用`process_wait`函数。如果拥有这个child_tid的子进程还存在在这个进程的子进程列表中（使用`find_exitcode()`函数进行查找），则继续等待。由于一个进程最多等待一个子进程一次，所以我们需要在进入等待过程之前先检查其是否已经被检查过。如果未被检查过，则将wait置为true，关闭信号量`exit_sema`。最终我们移除此子进程（函数`list_remove()`）并返回其status。



#### **B6:**

通过预先验证检查来避免错误的用户内存访问，我们编写了`check_str()`和`check_ptr()`函数来检查参数指针是否为空、是否为有效的用户地址。

以`get_args()`函数为例，其作用是从栈中获取参数，对于每个获取到的参数都用`check_ptr()`进行检验，如果无效则终止进程。

当检测到错误时，我们通过在返回控制之前调用为每个函数实现的`free()`函数来确保所有临时分配的资源(锁、缓冲区等)被释放。



### Synchronization

#### **B7:** 

**通过信号量load_sema实现:**  当一个进程在创建新的子进程时，它需要等待直到它得知子进程的可执行文件是否加载正确。所以一旦一个子进程被创建，它关闭信号量wait_sema并阻塞父进程。
一旦子进程的可执行文件加载完成，打开此信号量并唤醒父进程。

#### **B8:** 

**通过信号量exit_sema实现:**  当一个进程开始等待其某个子进程，关闭此信号量，阻塞进程。直到子进程退出，打开此信号量，唤醒父进程。



### Rationale

**B9:**  我们选择将访问过程分解成多步函数以便捕获错误，比如若指针指向内核的虚拟地址空间，那么这个指针是非法的，就在page_fault中调用syscall_exit(-1)退出



**B10：**

**(1)优点:**

1. 线程结构空间占用小
2. 每个文件描述符对每个进程都是唯一的，因此无需进一步考虑竞态条件
3. 将打开文件存入列表中以供内核查看，使得操作打开的文件更加灵活

**(2)缺点:**

1. 占用内核空间，用户程序可能会打开大量文件导致内核崩溃

    

**B11:**  我们未做改变
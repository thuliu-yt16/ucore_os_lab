# Lab4

## 练习0：填写已有实验

使用meld合并工具完成实验1/2/3代码的填入

## 练习1：分配并初始化一个进程控制块

### 设计实现过程：

​	alloc_proc函数的功能为新的proc_struct分配空间并返回，分配完成后需要完成初始化工作，大部分成员变量清零即可。实现如下：

```c
static struct proc_struct *
alloc_proc(void){
	struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
    if (proc != NULL) {
        proc -> state = PROC_UNINIT;
        proc -> pid = -1;
        proc -> runs = 0;
        proc -> kstack = 0;
        proc -> need_resched = 0;
        proc -> parent = NULL;
        proc -> mm = NULL;
        proc -> tf = NULL;
        proc -> context.eip = 0;
        proc -> context.esp = 0;
        proc -> context.ebx = 0;
        proc -> context.ecx = 0;
        proc -> context.edx = 0;
        proc -> context.esi = 0;
        proc -> context.edi = 0;
        proc -> context.ebp = 0;
        proc -> cr3 = boot_cr3;
        proc -> flags = 0;
        proc -> name[0] = '\0';
    }
    return proc;
}
```

### 回答问题：

- 请说明proc_struct中`struct context context`和`struct trapframe *tf`成员变量含义和在本实验中的作用是啥？

  - context

  ​	含义：进程的寄存器上下文，作用：进程切换时用于恢复寄存器值。

  - tf

  ​	含义：发生中断时保存在栈上的信息，作用：中断处理例程中时用于告知系统返回信息，可以在内核态用户态切换时改变。

## 练习2： 为新创建的内核线程分配资源

### 设计实现过程：

​	do_fork函数的功能为创建当前线程的一个副本，将当前线程的状态复制到新线程的控制块中。具体需要调用已经实现的alloc_proc获得一块新的控制块，接下来为进程分配一个内核栈，分配一个新的进程id，并复制上下文和内存管理信息。而后将新进程添加入列表并唤醒新进程。实现如下：

```c
int
do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf) {
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    if (nr_process >= MAX_PROCESS) {
        goto fork_out;
    }
    ret = -E_NO_MEM;
    proc = alloc_proc();
    if(proc == NULL) goto fork_out;
    proc -> parent = current;

    if (setup_kstack(proc) != 0) {
        goto bad_fork_cleanup_proc;
    }
    if (copy_mm(clone_flags, proc) != 0) {
        goto bad_fork_cleanup_kstack;
    }
    copy_thread(proc, stack, tf);
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        proc -> pid = get_pid();
        list_add(&proc_list, &(proc -> list_link));
        hash_proc(proc);
        nr_process ++;
    }
    local_intr_restore(intr_flag);
    wakeup_proc(proc);
    ret = proc -> pid;
fork_out:
    return ret;

bad_fork_cleanup_kstack:
    put_kstack(proc);
bad_fork_cleanup_proc:
    kfree(proc);
    goto fork_out;
}
```

### 回答问题：

- 请说明ucore是否做到给每个新fork的线程一个唯一的id？请说明你的分析和理由

  是。分析get_pid函数，发现它遍历进程链表proc_list, 尝试寻找一块连续的空闲pid([last_pid, next_safe))，因而最后返回的last_pid确实是空闲的id。

## 练习3：阅读代码，理解proc_run函数和它调用的函数如何完成进程切换的。

### 对proc_run函数的分析:

​	proc_run函数的功能为切换进程执行，其实现的关键部分如下：

```c
    current = proc;
    load_esp0(next -> kstack + KSTACKSIZE);
    lcr3(next -> cr3);
    switch_to(&(prev -> context), &(next -> context));
```

​	首先将current指针指向要切换的进程proc，表示当前的进程为proc， 第二句为将当前进程的内核栈栈顶信息保存在TSS中，而后将cr3寄存器改为next->cr3，表示将页表切换为proc的页表，switch_to函数切换了寄存器上下文。

### 回答问题：

- 在本实验的执行过程中，创建且运行了几个内核线程：

  两个，分别为idleproc, initproc，idleproc负责调度其他线程执行，initproc为显示HelloWorld的线程。

- 语句`local_intr_save(intr_flag);....local_intr_restore(intr_flag);`在这里有何作用?请说明理由

  进程切换过程中屏蔽中断，切换结束后允许中断。如果不屏蔽中断可能会打断切换从而发生未知错误，比如上下文未被切换等。

## 和参考答案的区别

- 练习1：基本一样，功能上没有区别。
- 练习2：开始的实现中在获得pid和插入链表时没有屏蔽中断，查阅参考答案时发现了这个问题，加上了屏蔽中断的代码。如果不加可能被时钟中断打断从而出现未知错误。在设置栈和拷贝mm_struct时没有判断是否正确返回，补充了判断的代码。

## OS原理知识点

- 实验中出现的重要知识点：内核线程，进程创建，进程切换
- 实验中没有对应的知识点：进程等待和退出，进程调度
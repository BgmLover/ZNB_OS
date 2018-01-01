#include "task.h"
#include <zjunix/utils.h>
#include <zjunix/sche.h>
#include <zjunix/shm.h>
#include <arch.h>
#include <driver/vga.h>
#include <zjunix/time.h>
#include <zjunix/slub.h>
#include <zjunix/list_pcb.h>
#include <zjunix/buddy.h>
#include <zjunix/bootmem.h>
#include <debug.h>
#include <page.h>

extern struct page *pages;
list_pcb pcbs;//进程队列
unsigned char idmap[32];//设置256个进程id
unsigned char bits_map[8]={1,2,4,8,16,32,64,128};

void task_test()
{
   
/*test shared memory*/
    // struct shared_memory* shm;
    // unsigned int p;
    //  task_union* proc1=( task_union*)kmalloc(PAGE_SIZE);
    //  task_union* proc2=( task_union*)kmalloc(PAGE_SIZE);
	
//     proc1->pcb.asid = (unsigned char)66;
//     proc2->pcb.asid = (unsigned char)77;
//     proc1->pcb.shm=NULL;
//     proc2->pcb.shm=NULL;
//     // shm_init();
//     shm=shm_get(4096);
//     shm_mount(&proc1->pcb, shm);
//     shm_mount(&proc2->pcb, shm);
//     shm_write(&proc1->pcb, 0, 'f');
//     p = shm_read(&proc2->pcb, 0);
//     kernel_printf("shm:%c\n", p);

// add_task(&proc1->pcb.process);
// add_task(&proc2->pcb.process);
/*end test shared memory*/


    // kernel_printf("begin to test\n");
    // unsigned int entry0,entry1,entryhi,index;
    // asm volatile(
    //     "mfc0 %0, $2\n\t"
    //     "mfc0 %1, $3\n\t"
    //     "mfc0 %2, $10\n\t"
    //     "mfc0 %3, $0\n\t"
    //     "nop\n\t"
    //     "nop\n\t"
    //     :"=r"(entry0),"=r"(entry1),"=r"(entryhi),"=r"(index));
    // kernel_printf("entry0:%x\n",entry0);
    // kernel_printf("entry1:%x\n",entry1);
    // kernel_printf("entryhi:%x\n",entryhi);
    // kernel_printf("index:%x\n",index);
    
    
    // context *t;
    // kernel_printf("%s\n",pcbs.next->pcb->name);
    // kernel_printf("%x\n",pcbs.next->pcb);
    // kernel_printf("%x\n",pcbs.next->pcb->context);
    // t=(pcbs.next->pcb->context);
    
    // t->at=123;
    // kernel_printf("%d\n",t->at);
    // do_fork(t,pcbs.next->pcb);
    // kernel_printf("%x\n",pcbs.next->next->pcb->context->at);
    // list_pcb *pos;
    // for(pos=pcbs.next;pos!=&pcbs;pos=pos->next)
    //     kernel_printf("pid:%x   name:%s\n",pos->pcb->asid,pos->pcb->name);
    // del_task(0);
    // for(pos=pcbs.next;pos!=&pcbs;pos=pos->next)
    //     kernel_printf("pid:%x   name:%s\n",pos->pcb->asid,pos->pcb->name);
    // task_union *t[10];
    // int i;
    // for(i=0;i<10;i++)
    // {
    //     t[i]=(task_union *)umalloc(PAGE_SIZE);
    //     kernel_printf("t[%x]:%x\n",i,t[i]);
    // }
}
void init_task()
{
    int i=0;
    INIT_LIST_PCB(&pcbs,NULL);  
    
    kernel_memset(idmap,0,16*sizeof(unsigned char));//初始化进程位图
    task_union *init;
    #ifdef TASK_DEBUG_INIT
    kernel_printf("task_union get start\n");
    #endif
    init=(task_union*)kmalloc(PAGE_SIZE);//init进程的地址
    if(!init){
        kernel_printf("failed to get space for init\n");
        return;
    }
    #ifdef TASK_DEBUG_INIT
    kernel_printf("task_union get\n");
    #endif

    //初始化上下文
    //init->pcb.context=(context*)(init+PAGE_SIZE-(sizeof(context)));
    init->pcb.context=(context*)((unsigned int)init+sizeof(PCB));
    // kernel_printf("address of init:%x\n",init);
    // kernel_printf("size of PCB:%x\n",sizeof(PCB));
    // kernel_printf("address of context:%x\n",init->pcb.context);
    //init->pcb.context->at=15;
    //init分配进程号为0
    init->pcb.asid=get_emptypid();
    if(init->pcb.asid<0){
        kernel_printf("failed to get right asid\n");   
        return;
    }
    #ifdef TASK_DEBUG_INIT
    kernel_printf("pid number %x get\n",init->pcb.asid);
    #endif
    init->pcb.pgd=(pgd_term*)kmalloc(PAGE_SIZE);//分配页目录空间
    if(init->pcb.pgd==NULL)
    {
        kernel_printf("failed to kmalloc space for pgd\n");
        return;
    }
    //初始化pgd每一项
    clean_page(init->pcb.pgd);


    //设置pgd属性为默认属性——可写
    //set_pgd_attr(init->pcb.pgd,Default_attr);
    #ifdef TASK_DEBUG_INIT
    kernel_printf("pgd address:%x\n",init->pcb.pgd);
    #endif
    kernel_strcpy(init->pcb.name, "init");
    init->pcb.parent=0;//init没有父进程
    init->pcb.uid=0;
    init->pcb.counter=DEFAULT_TIMESLICES;
    init->pcb.start_time=0;//get_time();
    init->pcb.state=STATE_WAITTING;
    init->pcb.priority=IDLE_PRIORITY;//设置优先级为最低优先级
    init->pcb.policy=0;//暂未定义调度算法
    init->pcb.shm=NULL; //shared memory

    INIT_LIST_PCB(&init->pcb.sched,&(init->pcb));
    INIT_LIST_PCB(&init->pcb.process,&(init->pcb));
    //暂不考虑线程
   #ifdef TASK_DEBUG_INIT
    kernel_printf("init_list_pcb over\n");
   #endif
    init->pcb.thread_head=NULL;
    init->pcb.num_thread=0;
    
    add_task(&(init->pcb.process));//添加到pcb链表中
    /*
    注册中断和系统调用
    register_syscall
    */
    init->pcb.state=STATE_RUNNING;
    #ifdef TASK_DEBUG_INIT
    kernel_printf("init_proc created successfully\n");
    kernel_printf("Proc name:%s\n",init->pcb.name);
    kernel_printf("pid:%d\n",init->pcb.asid);
    kernel_printf("Address: %x\n",init);
    #endif
}

void copy_context(context* src, context* dest) 
{
    dest->epc = src->epc;
    dest->at = src->at;
    dest->v0 = src->v0;
    dest->v1 = src->v1;
    dest->a0 = src->a0;
    dest->a1 = src->a1;
    dest->a2 = src->a2;
    dest->a3 = src->a3;
    dest->t0 = src->t0;
    dest->t1 = src->t1;
    dest->t2 = src->t2;
    dest->t3 = src->t3;
    dest->t4 = src->t4;
    dest->t5 = src->t5;
    dest->t6 = src->t6;
    dest->t7 = src->t7;
    dest->s0 = src->s0;
    dest->s1 = src->s1;
    dest->s2 = src->s2;
    dest->s3 = src->s3;
    dest->s4 = src->s4;
    dest->s5 = src->s5;
    dest->s6 = src->s6;
    dest->s7 = src->s7;
    dest->t8 = src->t8;
    dest->t9 = src->t9;
    dest->hi = src->hi;
    dest->lo = src->lo;
    dest->gp = src->gp;
    dest->sp = src->sp;
    dest->fp = src->fp;
    dest->ra = src->ra;
}
void clean_context(context* dest)
{
    dest->epc = 0;
    dest->at = 0;
    dest->v0 = 0;
    dest->v1 = 0;
    dest->a0 = 0;
    dest->a1 = 0;
    dest->a2 = 0;
    dest->a3 = 0;
    dest->t0 = 0;
    dest->t1 = 0;
    dest->t2 = 0;
    dest->t3 = 0;
    dest->t4 = 0;
    dest->t5 = 0;
    dest->t6 = 0;
    dest->t7 = 0;
    dest->s0 = 0;
    dest->s1 = 0;
    dest->s2 = 0;
    dest->s3 = 0;
    dest->s4 = 0;
    dest->s5 = 0;
    dest->s6 = 0;
    dest->s7 = 0;
    dest->t8 = 0;
    dest->t9 = 0;
    dest->hi = 0;
    dest->lo = 0;
    dest->gp = 0;
    dest->sp = 0;
    dest->fp = 0;
    dest->ra = 0;
}

unsigned char get_emptypid()
{
    unsigned char number=0;
    unsigned char temp;
    unsigned int index,bits;
    for(index=0;index<16;index++)
    {
        temp=idmap[index];
        for(bits=0;bits<sizeof(unsigned char);bits++)
        {
            if(!(temp&0x01)){
                idmap[index]|=bits_map[bits];
                break;
            }
            temp>>=1;
            number++;
        }
        if(bits<sizeof(unsigned char))
            break;
    }

    return number;
}
void free_pid(unsigned int pid)
{
    unsigned int index=pid/8;
    unsigned int bit_index=pid%8;
    idmap[index]&=(~bits_map[bit_index]);
}
PCB *get_pcb_by_pid(unsigned int pid){
    int index=0;
    list_pcb *pos;
    for(pos=pcbs.next;pos!=&pcbs;pos=pos->next)
    {
        if(pos->pcb->asid==pid)
        {
            PCB * task=pos->pcb;
            return task;
        }
    }
    return NULL;
}
//把一个进程加到进程队列末尾
void add_task(list_pcb* process)
{
    list_pcb_add(process,&pcbs);
}



int do_fork(context* args,PCB*parent)
{
    #ifdef DO_FORK_DEBUG
    kernel_printf("begin to fork\n");
    #endif

    task_union *new;
    new=(task_union*)kmalloc(sizeof(task_union));
    if(new==NULL)
    {
        kernel_printf("error : failed to allocate space for task_union\n");
        goto error1;
    }
    #ifdef DO_FORK_DEBUG
    kernel_printf("address of new task_union %x\n",new);
    #endif
    //复制上下文
    new->pcb.context=(context*)((unsigned int)new+sizeof(PCB));
    copy_context(args,new->pcb.context);
    
    #ifdef DO_FORK_DEBUG
    kernel_printf("old context->at=%x\n",args->at);
    kernel_printf("new context->at=%x\n",new->pcb.context->at);
    kernel_printf("copy context over\n");
    #endif
    //复制页表
    new->pcb.pgd=copy_pagetables(&(new->pcb),parent);
    if(new->pcb.pgd==NULL)
    {
        kernel_printf("error : failed to copy pages\n");
        goto error2;
    }
    #ifdef DO_FORK_DEBUG
    kernel_printf("copy pagetables over\n");
    #endif

    //复制或是设置新的PCB信息
    kernel_memcpy(new->pcb.name,parent->name,sizeof(char)*32);
    new->pcb.asid=get_emptypid();
    if(new->pcb.asid>255)
    {
        kernel_printf("error : no more pid to allocate\n");
        goto error3;
    }
    //复制文件信息
    if(parent->file==NULL)
        new->pcb.file=NULL;
    else{
        new->pcb.file=(FILE*)kmalloc(sizeof(FILE));
        if(new==NULL){
            kernel_printf("error in do_fork:failed to malloc for FILE\n");
            goto error4;
        }
        kernel_memcpy(new->pcb.file,parent->file,sizeof(FILE));
    }
    new->pcb.parent=parent->asid;
    new->pcb.uid=parent->uid;
    new->pcb.counter=parent->counter;
    new->pcb.start_time=0;//这里记得去完善time函数
    new->pcb.priority=parent->priority;
    new->pcb.policy=parent->priority;

    INIT_LIST_PCB(&(new->pcb.sched),&new->pcb);
    INIT_LIST_PCB(&(new->pcb.process),&new->pcb);

    new->pcb.thread_head=NULL;
    new->pcb.num_thread=0;
    new->pcb.shm=NULL; // shared memory

    new->pcb.file=parent->file;

    new->pcb.state=STATE_READY;
    //添加到进程队列中
    add_task(&(new->pcb.process));
    /*
    加入调度队列
    */
    #ifdef DO_FORK_DEBUG
    kernel_printf("child 's name:%s\n",new->pcb.name);
    kernel_printf("child 's pid : %x\n",new->pcb.asid);
    #endif
    //返回新进程的进程号
    return new->pcb.asid;
    

    error1:
        return -1;
    error2:
        kfree(new);
        return -2;
    error3:
        kfree(new->pcb.pgd);
        kfree(new);
        return -3;
    error4:
        kfree(new->pcb.pgd);
        kfree(new);
        free_pid(new->pcb.asid);
        return -4;
}
 


void inc_refrence_by_pte( unsigned int *pte)
{
	 unsigned int index;

	for (index = 0; index < (PAGE_SIZE >> 2); ++index) {
		if (is_V(&(pte[index]))) {
			inc_ref(pages + (pte[index] >> PAGE_SHIFT), 1);
		}
	}
}

void dec_refrence_by_pte(unsigned int *pte)
{
	unsigned int index;
	for (index = 0; index < (PAGE_SIZE >> 2); ++index) {
		if (pte[index]) {
            //物理页地址
            unsigned int phy_addr=pte[index]&(~OFFSET_MASK);
            struct page *phy_page=pages+(phy_addr>>PAGE_SHIFT);
            //引用次数--
            dec_ref(phy_page,1);
            //如果引用次数为0，则将该页free掉
            if(phy_page->reference==0)
                kfree((void*)phy_addr);
			pte[index] = 0;
		}
	}
}

pgd_term *copy_pagetables(PCB* child,PCB* parent)
{
    pgd_term* old_pgd;
    pgd_term* new_pgd=NULL;
    pte_term* temp_pte;
    pte_term* old_pte;
    unsigned int index,ip;//索引
    unsigned int count=0;//记录pgd中已经成功分配的页数

    //分配一张新的页作为页目录
    old_pgd=parent->pgd;
    new_pgd=(pgd_term*)kmalloc(PAGE_SIZE);
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("new pgd address:%x\n",new_pgd);
    #endif
    if(new_pgd==NULL)
    {
        kernel_printf("copy_pagetables failed : failed to malloc for pgd\n");
        goto error1;
    }
    kernel_memcpy(new_pgd,old_pgd,PAGE_SIZE);
    for(index=0;index<KERNEL_ENTRY>>PGD_SHIFT;index++)
    {
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("pgd index:%x\n",index);
    #endif
        if(old_pgd[index]){
            temp_pte=(pte_term*)kmalloc(PAGE_SIZE);
            if(!temp_pte){
                kernel_printf("copy_pagetables failed : failed to malloc for pgte\n");
                goto error2;
            }
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("new_pte_addr=%x\n",temp_pte);
    #endif
            count++;
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("count:%x\n",count);
    kernel_printf("old_pgd[%x]=%x\n",index,old_pgd[index]);
    #endif
            //将新的pgd的每一项pte设置为新分配的pte页地址
            new_pgd[index] &= OFFSET_MASK;
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("after &=mask: new_pgd[%x]=%x\n",index,new_pgd[index]);
    #endif
            new_pgd[index] |= (unsigned int)temp_pte;
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("new_pgd[%x]=%x\n",index,new_pgd[index]);
    #endif
            old_pte=(pte_term*) (old_pgd[index]&(~OFFSET_MASK));
            kernel_memcpy(temp_pte,old_pte,PAGE_SIZE);
            for(ip=0;ip<(PAGE_SIZE>>2);ip++)
            {
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("ip:%x\n",ip);
    #endif
                if(temp_pte[ip])
                {
                    unsigned int va;
                    va=index<<PGD_SHIFT;
                    va|=(ip<<PTE_SHIFT);
                    //新老页表现在都不能往这个地址上写
                    clean_W(&(old_pte[ip]));
                    clean_W(&(temp_pte[ip]));
                    tlbp(va,parent->asid);
                    unsigned int tlb_index=get_tlb_index();
                    if(tlb_index&(1<<31)==0)//在TLB里存在这项内容，需要将其修改
                    tlbwi(va,parent->asid,old_pte[ip],tlb_index);
                }
            }
        }

    }
    #ifdef COPY_PAGE_DEBUG
    kernel_printf("copy pgd and pte over\n");
    #endif
    for(index=0;index<KERNEL_ENTRY>>PGD_SHIFT;index++){
        if(new_pgd[index])
        {
            //在物理内容中对应的页里增加引用次数
            inc_refrence_by_pte((pte_term*) (new_pgd[index]&(~OFFSET_MASK)));
        }
    }
    return new_pgd;

    error2:
        if(count){
            for(index=0;index<KERNEL_ENTRY>>PGD_SHIFT;index++)
            {
                if(count==0) break;
                if(new_pgd[index])
                {
                    temp_pte=(pte_term*) (new_pgd[index]&(~OFFSET_MASK));
                    old_pte= (pte_term*) ((old_pgd[index]&(~OFFSET_MASK)));
                    
                    if(old_pte==temp_pte){
                        kfree(temp_pte);
                        count--;
                    }
                }
            }     
        }
    error1:
        kfree(new_pgd);
        return NULL;
}


void delete_pages(PCB *task)
{
	pgd_term *pgd = task->pgd;
	pte_term *pte;
	unsigned int index;
	//只删去属于用户空间的页表
	for (index = 0; index < (KERNEL_ENTRY >> PGD_SHIFT); ++index) {
		if (pgd[index]) {
			pte =(pte_term*) (pgd[index] & (~OFFSET_MASK));
			dec_refrence_by_pte(pte);
			kfree(pte);
			pgd[index] = 0;
		}
	}
}

void delete_pagetables(PCB *task)
{
	delete_pages(task);
	kfree(task->pgd);
}
//把一个进程从进程队列中删去
unsigned int del_task(unsigned int pid)
{
    int index=0;
    list_pcb *pos;
    PCB * task_to_del=get_pcb_by_pid(pid);
    if(task_to_del==0){
        kernel_printf("process not found,pid %d",pid);
        return 1;
    }
    kfree(task_to_del->context);//删去上下文
    delete_pagetables(task_to_del);  //删去页表
    kfree(task_to_del->file);      //删去文件信息
    free_pid(task_to_del->asid);      //释放进程号
    kfree((task_union*)task_to_del);//删去整个task_union
    list_pcb_del_init(pos);
    //在调度队列中删去它
    return 0;
    
}
int exec1(char* filename) {
    FILE file;
    const unsigned int CACHE_BLOCK_SIZE = 64;
    #ifdef EXEC_DEBUG
    kernel_printf("begin to exec\n");
    kernel_printf("filename:%s\n",filename);
    #endif
    unsigned char buffer[512];
    int result = fs_open(&file, filename);
    if (result != 0) {
        kernel_printf("File %s not exist\n", filename);
        return 1;
    }
    unsigned int size = get_entry_filesize(file.entry.data);
    unsigned int n = size / CACHE_BLOCK_SIZE + 1;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int ENTRY = (unsigned int)kmalloc(4096);
    for (j = 0; j < n; j++) {
        fs_read(&file, buffer, CACHE_BLOCK_SIZE);
        kernel_memcpy((void*)(ENTRY + j * CACHE_BLOCK_SIZE), buffer, CACHE_BLOCK_SIZE);
        kernel_cache(ENTRY + j * CACHE_BLOCK_SIZE);
    }
    unsigned int cp0EntryLo0 = ((ENTRY >> 6) & 0x01ffffc0) | 0x1e;
    asm volatile(
        "li $t0, 1\n\t"
        "mtc0 $t0, $10\n\t"
        "mtc0 $zero, $5\n\t"
        "move $t0, %0\n\t"
        "mtc0 $t0, $2\n\t"
        "mtc0 $zero, $3\n\t"
        "mtc0 $zero, $0\n\t"
        "nop\n\t"
        "nop\n\t"
        "tlbwi"
        :
        : "r"(cp0EntryLo0));
    int (*f)() = (int (*)())(0);
#ifdef EXEC_DEBUG
    kernel_printf("Exec load at: 0x%x\n", ENTRY);
#endif  // ! EXEC_DEBUG
    unsigned int s1=*(unsigned int*)0;
    //unsigned int s2=*(unsigned int*);
    kernel_printf("s1=%x\n",s1);
    //kernel_printf("s2=%x\n",s2);
    int r = f();
    kernel_printf("run the program over\n");
    kfree((void*)ENTRY);
    return r;
}

int exec2(PCB *task,char* filename){
    #ifdef EXEC_DEBUG
    kernel_printf("begin to exec\n");
    kernel_printf("task name:%s\n",task->name);
    kernel_printf("task pid:%x\n",task->asid);
    #endif
    //清除PCB的上下文并重新设置
    clean_context(task->context);
    task->context->epc=0;
    task->context->sp=(unsigned int)task+PAGE_SIZE;
    unsigned int init_gp;
    asm volatile("la %0, _gp\n\t" : "=r"(init_gp));
    task->context->gp=init_gp;

    //清除PCB的页表并分配第一张页
    delete_pagetables(task);
    task->pgd=(pgd_term*)kmalloc(PAGE_SIZE);
    pgd_term*pgd=task->pgd;
    clean_page(task->pgd);
    //设置第一张页目录的属性和pte的值
    set_V(&pgd[0]);
    set_W(&pgd[0]);
    pte_term*pte=(pte_term*)kmalloc(PAGE_SIZE);
    pgd[0]|=(unsigned int)pte;

    //申请 文件控制块的大小
    if(task->file==NULL)
        task->file=(FILE*)kmalloc(5*PAGE_SIZE);//此处还有bug
    else{
        kfree(task->file);
        task->file=(FILE*)kmalloc(5*PAGE_SIZE);
    }
    #ifdef EXEC_DEBUG
    kernel_printf("address of task:%x\n",task);
    kernel_printf("size of FILE:%x\n",sizeof(FILE));
    kernel_printf("address of task_file:%x\n",task->file);
    #endif
    
    // fopen操作
    int result = fs_open(task->file, filename);
    if (result != 0) {
        kfree(task->file);
        delete_pagetables(task);
        clean_context(task->context);
        kernel_printf("File %s not exist\n", filename);
        return -1;
    }
    #ifdef EXEC_DEBUG
    kernel_printf("fopen over\n");
    #endif
    //把文件的第一张页大小的内容读取到内存的一张页中
    unsigned int phy_addr;
    phy_addr=read_file_to_page(task->file,0);
    if(phy_addr==0){
        kfree(task->file);
        delete_pagetables(task);
        clean_context(task->context);
        return -1;
    }
    #ifdef EXEC_DEBUG
    kernel_printf("read file over\n");
    #endif

    //物理地址转化为EntryLo0的值
    unsigned int cp0EntryLo0=va2pfn(phy_addr);
    unsigned int asid=task->asid;

    //TLB中随机写入这一项
    asm volatile(
        "mtc0 %0, $10\n\t"
        "mtc0 $zero, $5\n\t"
        "mtc0 %1, $2\n\t"
        "mtc0 $zero, $3\n\t"
        "mtc0 $zero, $0\n\t"
        "nop\n\t"
        "nop\n\t"
        "tlbwi"
        :
        : "r"(asid),"r"(cp0EntryLo0));
    
#ifdef EXEC_DEBUG
    kernel_printf("Exec load at: 0x%x\n", phy_addr);
    unsigned int hi,e0;
    asm volatile(
         "mfc0 %0,$10\n\t"
         "mfc0 %1,$2\n\t"
         "nop\n\t"
         "nop\n\t"
         :"=r"(hi),"=r"(e0));
    kernel_printf("entryhi=%x\n",hi);
    kernel_printf("entry0=%x\n",e0);
    unsigned int s1=*(unsigned int*)0;
    kernel_printf("s1=%x\n",s1);
    int (*f)() = (int (*)())(0);
    int r = f();
    while(1);
#endif  // ! EXEC_DEBUG


    return 0;
}

int exec(char *filename,char* taskname)
{
    PCB *current=get_current_pcb();
    //do fork
    unsigned int child_pid=do_fork(current->context,current);
    if(child_pid<0){
        kernel_printf("error! failed to do_fork\n");
        return -1;
    }
    PCB *child=get_pcb_by_pid(child_pid);
    //从文件中读取数据并替换
    if(exec2(child,filename)!=0){
        kernel_printf("error! failed to exec\n");
        return -1;
    }
    kernel_memcpy(child->name,taskname,sizeof(char)*32);
    return 0;
}

void print_tasks(){
    int i=0;
    while(1)
    {
        PCB* task=get_pcb_by_pid(i);
        if(task==NULL)
            break;
        else{
            kernel_printf("pid: %d  ",task->asid);
            kernel_printf("task name: %s  ",task->name);
            kernel_printf("priority: %d  ",task->priority);
            kernel_printf("state: %d",task->state);

            kernel_printf("\n");
        }
    }
}
void syscall_fork_31(unsigned int status, unsigned int cause, context* pt_context)
{
    context*args=(context*)(pt_context->a0);
    PCB*parent=(PCB*)(pt_context->a1);
    do_fork(args,parent);
}
void syscall_exec_32(unsigned int status, unsigned int cause, context* pt_context)
{
    char*filename=(char*)(pt_context->a0);
    char*taskname=(char*)(pt_context->a1);
    exec(filename,taskname);
}
void syscall_kill_33(unsigned int status, unsigned int cause, context* pt_context)
{
    unsigned int asid=pt_context->a0;
    del_task(asid);
}
void syscall_exit_34(unsigned int status, unsigned int cause, context* pt_context)
{

}
void syscall_print_tasks_35(unsigned int status, unsigned int cause, context* pt_context)
{
    print_tasks();
}


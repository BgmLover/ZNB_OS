#include <arch.h>
#include <driver/vga.h>
#include <zjunix/bootmem.h>
#include <zjunix/utils.h>

struct bootmem mm;//bootmem主体
unsigned char bootmem_map[MACHINE_MMSIZE >> PAGE_SHIFT];//位图
char *mem_msg[] = {"Kernel code/data", "Mm Bitmap", "Vga Buffer", "Kernel page directory", "Kernel page table", "Dynamic", "Reserved"};


//设置bootmem中的某一块已分配的内存
void bootmem_set_mminfo(struct bootmem_info *info, unsigned int start, unsigned int end, unsigned int type){
    info->start_pfn = start;
    info->end_pfn = end;
    info->type = type;
}

//向info数组中加入一块
void bootmem_insert_mminfo(unsigned int start, unsigned int end, unsigned int type){
    unsigned int index;

    //找可以合并的另一个块
    for(index = 0; index < mm.cnt_infos; index++){
        if(mm.info[index].type != type) continue;
        if(mm.info[index].end_pfn == start-1){
            if((index+1) < mm.cnt_infos){
                if(mm.info[index+1].type != type) goto merge1;
                if(mm.info[index+1].start_pfn - 1 == end){
                    mm.info[index].end_pfn = mm.info[index+1].end_pfn;
                    bootmem_remove_mminfo(index);
                    return;
                }
            }

            merge1:
            mm.info[index].end_pfn = end;
            return;
        }
    }
    bootmem_set_mminfo(mm.info+mm.cnt_infos, start, end, type);
    ++(mm.cnt_infos);
    kernel_printf("++cnt infos!\n");


}

// 删除info数组中的一个块，后面的块往前移动
void bootmem_remove_mminfo(unsigned int index){
    unsigned int tmp;

    if(index >= mm.cnt_infos) return;
    for(tmp = (index+1); tmp!=mm.cnt_infos; ++tmp){
        mm.info[tmp-1] = mm.info[tmp];
    }
    --(mm.cnt_infos);
}

//初始化bootmem
void bootmem_init(){
    // unsigned int index;
    unsigned char* t_map;
    unsigned int end = 16*1024*1024; // 16mb kernel memory
    unsigned int i;

    mm.phymm = get_phymm_size();//获得物理内存
    mm.max_pfn = mm.phymm >> PAGE_SHIFT;//计算最大页框数
    mm.s_map = bootmem_map;
    mm.e_map = bootmem_map + sizeof(bootmem_map);//位图指针指向应该的区域
    mm.cnt_infos = 0;
    for(i=0; i<sizeof(bootmem_map); i++){
        bootmem_map[i]=PAGE_FREE;
    } 
    bootmem_insert_mminfo(0, (unsigned int)(end-1), _MM_KERNEL);//分配16mb内存作为内核空间
    mm.last_alloc = (((unsigned int)(end)>>PAGE_SHIFT)-1);

    //设置为已被占用
    for(i = 0; i < end>>PAGE_SHIFT; i++){
        mm.s_map[i] = PAGE_USED;
    }

    /*debug*/
    kernel_printf("\nmm.cnt_infos= %x\n", mm.cnt_infos);
    kernel_printf("\nstart_pfn= %x\nend_pfn=%x\n", mm.info[0].start_pfn, mm.info[0].end_pfn);
    /*end debug*/
}

//输出info数组的信息
void bootmem_bootmap_info(unsigned char *msg) {
    unsigned int index;
    kernel_printf("%s :\n", msg);
    for (index = 0; index < mm.cnt_infos; ++index) {
        kernel_printf("\t%x-%x : %s\n", mm.info[index].start_pfn, mm.info[index].end_pfn, mem_msg[mm.info[index].type]);
    }
}

//设置位图某些位被占用
void bootmem_set_maps(unsigned int s_pfn, unsigned int cnt, unsigned char value) {
    while (cnt) {
        mm.s_map[s_pfn] = value;
        --cnt;
        ++s_pfn;
    }
}

//找到空闲页，用于分配
unsigned char* bootmem_find_pages(unsigned int count, unsigned int s_pfn, unsigned int e_pfn, unsigned int align_pfn){
    unsigned int index, tmp;
    unsigned int cnt;

    s_pfn += (align_pfn - 1);
    s_pfn &= ~(align_pfn - 1);

    //遍历位图，找到能分配的空闲页
    for(index = s_pfn; index < e_pfn;){
        if(mm.s_map[index]){
            ++index;
            continue;

        }
        tmp = index;
        cnt = count;
        while(cnt){
            if(tmp >= e_pfn) goto end;
            if(mm.s_map[tmp]) goto next;
            ++tmp;
            --cnt;
        }
        mm.last_alloc = index + count - 1;
        bootmem_set_maps(index, count, PAGE_USED);

        return (unsigned char*)(index << PAGE_SHIFT);
        next:
        index = tmp + align_pfn;
    }
    end:
    return 0;
}


// return 0 means error
//bootmem分配空间
unsigned char *bootmem_alloc_pages(unsigned int size, unsigned int type, unsigned int align) {
    unsigned int cnt;
    unsigned char *res;

    size += ((1 << PAGE_SHIFT) - 1);
    size &= PAGE_ALIGN;
    cnt = size >> PAGE_SHIFT;

    // 从当前位置往后找，一般能找到空闲页
    res = bootmem_find_pages(cnt, mm.last_alloc + 1, mm.max_pfn, align >> PAGE_SHIFT);
    if (res) {
        bootmem_insert_mminfo((unsigned int)res, (unsigned int)res + size - 1, type);
        return res;
    }

    // 从开始位置找到当前位置，一般不需要这么做
    res = bootmem_find_pages(cnt, 0, mm.last_alloc, align >> PAGE_SHIFT);
    if (res) {
        bootmem_insert_mminfo((unsigned int)res, (unsigned int)res + size - 1, type);
        return res;
    }
    return 0; 
} 

// return 0 means error
// 分开mminfo块
unsigned int bootmem_split_mminfo(unsigned int index, unsigned int split_start) {
    unsigned int start, end;
    unsigned int tmp;

    start = mm.info[index].start_pfn;
    end = mm.info[index].end_pfn;
    split_start &= PAGE_ALIGN;

    if ((split_start <= start) || (split_start >= end))
        return 0; 

    if (mm.cnt_infos == MAX_INFO)
        return 0; 
    for (tmp = mm.cnt_infos - 1; tmp >= index; --tmp) {
        mm.info[tmp + 1] = mm.info[tmp];
    }
    mm.info[index].end_pfn = split_start - 1;
    mm.info[index + 1].start_pfn = split_start;
    mm.cnt_infos++;
    return 1;
}

//设置位图
unsigned int bootmem_set_map(unsigned int start_pfn, unsigned int end_pfn, unsigned int type){
    int i;
    if(start_pfn < 0 || end_pfn > mm.phymm >> PAGE_SHIFT - 1) return -1;
//    if(type < _MM_KERNEL || type > _MM_COUNT) return -2;
    for(i = start_pfn; i <= end_pfn; i++) bootmem_map[i] = (unsigned char)type;
    return 0;
}

// bootmem释放内存
unsigned int bootmem_free_pages(unsigned int start, unsigned int size){
    unsigned int index, cnt;
    size &= ~((1<<PAGE_SHIFT)-1);
    cnt = size>>PAGE_SHIFT;
    if(!cnt) return 0; // error

    start &= ~((1<<PAGE_SHIFT)-1);
    for(index=0; index<mm.cnt_infos; ++index){
        if(mm.info[index].start_pfn<=start &&
            mm.info[index].end_pfn>=start &&
            start + size - 1 <= mm.info[index].end_pfn)
            break;
    }
    if(index == mm.cnt_infos){
        kernel_printf("bootmem_free_pages: not alloc space(%x:%x)\n", start, size);
        return 0; // error
    }

    bootmem_set_map(start>>PAGE_SHIFT, (start>>PAGE_SHIFT)+cnt, PAGE_FREE);

    if(mm.info[index].start_pfn==start){
        if(mm.info[index].end_pfn==(start+size-1)){//前后都相接
            bootmem_remove_mminfo(index);
        }
        else{//前相接，后不相接
            bootmem_set_mminfo(&mm.info[index], mm.info[index].start_pfn, start+size-1, mm.info[index].type);
        }
    }
    else{
        if(mm.info[index].end_pfn==(start+size-1)){//前不相接，后相接
            bootmem_set_mminfo(&mm.info[index], start, mm.info[index].end_pfn, mm.info[index].type);
        }
        else{//前后都不相接
            bootmem_split_mminfo(index, start);
            bootmem_split_mminfo(index+1, start+size);
            bootmem_remove_mminfo(index+1);
        }
    }
    return 1;
}
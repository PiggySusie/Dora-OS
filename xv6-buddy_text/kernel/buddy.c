// 基于伙伴分配算法的内存管理系统
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define LEAF_SIZE     16                         // 最小的块大小
#define MAXSIZE       (nsizes-1)                 // buddy_sizes 数组中最大的索引
#define BLK_SIZE(k)   ((1L << (k)) * LEAF_SIZE)  // 大小为 k 的块的大小
#define HEAP_SIZE     BLK_SIZE(MAXSIZE)          // 堆的总大小
#define NBLK(k)       (1 << (MAXSIZE-k))         // 大小为 k 的块的数量
#define ROUNDUP(n,sz) (((((n)-1)/(sz))+1)*(sz))  // 向上对齐到下一个 sz 的倍数


typedef struct list_node buddy_list;
struct sz_info {
  buddy_list free;
  char *alloc; 
  char *split; 
};
typedef struct sz_info Sz_info;

// 可能需要的变量声明如下：
// static int nsizes;     
// static Sz_info *buddy_sizes; 
// static void *buddy_base;  
// static struct spinlock lock;

int bit_is_set(char *array, int index) {
  // TODO: 实现 bit_is_set，用于判断给定的 array 中某个位置的位是否被设置为 1
  return 0;
}

void set_bit(char *array, int index) {
  // TODO: 实现 set_bit，用于设置 array 中某个位为 1
}

void clear_bit(char *array, int index) {
  // TODO: 实现 clear_bit，清除 array 中某个位
}

int
find_first_k(uint64 n) {
  // TODO: 实现 find_first_k，根据输入的 n，返回一个最小的 k，使得 2^k >= n
  return 0; 
}

int
block_index(int k, char *p) {
  // TODO: 实现 block_index，通过给定块大小k和内存地址p计算一个块的内存索引
  return 0; 
}

void *address(int k, int bi) {
  // TODO: 实现 address，给定一个大小为k的块的索引，计算出该块的起始内存地址
  return 0; 
}

int
size(char *p) {
  // TODO: 实现 size，根据p的位置和分配的内存块的规则，计算出该块的大小
  return 0; 
}

int
block_index_next(int k, char *p) {
  // TODO: 实现 block_index_next，计算在大小为k的块中，不包含给定地址p的下一个块的索引
  return 0; 
}

int
log2(uint64 n) {
  // TODO: 实现 log2
  return 0; 
}

void *
buddy_malloc(uint64 nbytes)
{
  // TODO: 实现 buddy_malloc，分配 nbytes 大小的内存，保证不会分配小于 LEAF_SIZE 的块
  // Hint：首先找到足够大的块来分配给 nbytes，然后遍历空闲块列表，使用拆分算法分配内存
  return 0; 
}

void
buddy_free(void *p) {
  // TODO: 实现 buddy_free，释放通过 buddy_malloc 分配的内存
  // Hint：找到对应的块，检查是否需要合并邻近的伙伴块。如果有伙伴空闲，则合并它们，并递归释放更大的块
}

void
buddy_mark(void *start, void *stop)
{
  // TODO: 实现 buddy_mark，标记内存区间 [start, stop) 为已分配
}

int buddy_initfree(void *buddy_left, void *buddy_right) {
  // TODO：实现 buddy_initfree ，初始化空闲链表，注意需要初始化一个伙伴对的空闲链表，并将空闲块放入链表
  return 0; 
}

// 初始化 Buddy 分配器：它管理从 [base, end) 范围的内存
void buddy_init(void *base, void *end) {
  // TODO: 实现 buddy_init，初始化 Buddy 分配器
  // Hint：首先对整个内存区域进行初始化。确保 Buddy 系统不会分配掉元数据区域和已经分配的内存区域。
  // 同时初始化每个块级别的空闲链表，并确保内存能够按 Buddy 算法正确分配。
}

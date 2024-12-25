// 基于伙伴分配算法的内存管理系统
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

static int nsizes;                               // 静态整型变量nsizes，记录buddy_sizes数组包含的条目数量
#define LEAF_SIZE     16                         // 最小的块大小为16，是相关块大小计算的基准值
#define MAXSIZE       (nsizes-1)                 // buddy_sizes 数组中最大的索引
#define BLK_SIZE(k)   ((1L << (k)) * LEAF_SIZE)  // 大小为 k的块的实际大小
#define HEAP_SIZE     BLK_SIZE(MAXSIZE)          // 堆的总大小
#define NBLK(k)       (1 << (MAXSIZE-k))         // 大小为 k 的块的数量
#define ROUNDUP(n,sz) (((((n)-1)/(sz))+1)*(sz))  // 向上对齐到下一个 sz 的倍数


typedef struct list_node buddy_list;

// 每个 sz_info 都含一个空闲列表以及两个用于跟踪块状态的数组分配
struct sz_info {
  buddy_list free;
  char *alloc; // 跟踪已被分配的数据块
  char *split; // 跟踪已被拆分的数据块
};
typedef struct sz_info Sz_info;

static Sz_info *buddy_sizes; // Buddy 指向一个Sz_info类型的数组
static void *buddy_base;   /// Buddy 所管理的内存区域的起始地址
static struct spinlock lock;

// 判断给定的 array 中某个位置的位是否被设置为 1
int bit_is_set(char *array, int index) {
  char b = array[index/8];
  char m = (1 << (index % 8));
  return (b & m) == m;
}

// 设置 array 中某个位为 1
void set_bit(char *array, int index) {
  char b = array[index/8];
  char m = (1 << (index % 8));
  array[index/8] = (b | m);
}

// 清除 array 中某个位
void clear_bit(char *array, int index) {
  char b = array[index/8];
  char m = (1 << (index % 8));
  array[index/8] = (b & ~m);
}

// 使用一个比特位表示一对 buddy 块的状态
void flip_mutual_bit(char *array, int index) {
  index /= 2; 
  if(bit_is_set(array, index)){
    clear_bit(array, index);
  }
  else
  {
    set_bit(array, index);
  }  
}
// 获取指定 index 对应 buddy 块的当前状态
int get_mutual_bit(char *array, int index){
  index /= 2;
  return bit_is_set(array, index);
}


// 找到第一个满足 2^k >= n 的 k
int
find_first_k(uint64 n) {
  int k = 0;
  uint64 size = LEAF_SIZE;

  while (size < n) {
    k++;
    size *= 2;
  }
  return k;
}

// 通过给定块大小k和内存地址p计算一个块的内存索引
int
block_index(int k, char *p) {
  int n = p - (char *) buddy_base;
  return n / BLK_SIZE(k);
}

// 给定一个大小为k的块的索引，计算出该块的起始内存地址
void *address(int k, int bi) {
  int n = bi * BLK_SIZE(k);
  return (char *) buddy_base + n;
}

// 计算一个块的大小
int
size(char *p) {
  for (int k = 0; k < nsizes; k++) {
    if(bit_is_set(buddy_sizes[k+1].split, block_index(k+1, p))) {
      return k;
    }
  }
  return 0;
}
// 计算在大小为k的块中，不包含给定地址p的下一个块的索引
int
block_index_next(int k, char *p) {
  int n = (p - (char *) buddy_base) / BLK_SIZE(k);
  if((p - (char*) buddy_base) % BLK_SIZE(k) != 0)
      n++;
  return n ;
}

int
log2(uint64 n) {
  int k = 0;
  while (n > 1) {
    k++;
    n = n >> 1;
  }
  return k;
}

// 分配内存，保证分配的内存块大于等于 LEAF_BLOCK_SIZE
void *
buddy_malloc(uint64 nbytes)
{
  int fk, k;
   //获取锁，在多线程环境下保证对Buddy互斥性
  acquire(&lock);
   // 从最小的 k 开始，寻找第一个足够大的空闲块
  fk = find_first_k(nbytes);
  for (k = fk; k < nsizes; k++) {
    if(!lst_empty(&buddy_sizes[k].free))
      break;
  }
  if(k >= nsizes) { // 没有足够的空闲块
    release(&lock); //返回空指针
    return 0;
  }

  // 找到空闲块，从空闲链表中移除，并进行可能的分裂
  char *p = lst_pop(&buddy_sizes[k].free);
  flip_mutual_bit(buddy_sizes[k].alloc, block_index(k, p));
  // 如果超过实际需求，分裂大块，直到满足需求
  for(; k > fk; k--) {
    char *q = p + BLK_SIZE(k-1);   
    set_bit(buddy_sizes[k].split, block_index(k, p)); //设置对应块的拆分位
    flip_mutual_bit(buddy_sizes[k-1].alloc, block_index(k-1, p));//翻转对应分配位的状态
    lst_push(&buddy_sizes[k-1].free, q);
  }


  release(&lock);

  return p;
}

//释放通过buddy分配器分配的内存块
void
buddy_free(void *p) {
  void *q;
  int k;
  int bi;
  int buddy;
  int is_buddy_free;

  acquire(&lock);

  // 获取块的大小 k
  for (k = size(p); k < MAXSIZE; k++) {
    bi = block_index(k, p);
    buddy = (bi % 2 == 0) ? bi + 1 : bi - 1;

    // 标记该块为已释放
    flip_mutual_bit(buddy_sizes[k].alloc, bi);  
    
    // 检查该块的伙伴是否已分配
    is_buddy_free = !get_mutual_bit(buddy_sizes[k].alloc, buddy);
    if (is_buddy_free) {
      // 伙伴是空闲的，进行延迟合并
      set_bit(buddy_sizes[k + 1].split, block_index(k + 1, p));

      // 延迟合并：移除伙伴块
      q = address(k, buddy);
      lst_remove(q);  // 从空闲链表中移除 buddy

      // 如果是偶数块，更新p指针指向合并后的块
      if (buddy % 2 == 0) {
        p = q;
      }
    } else {
      //伙伴块已分配，不需要进行合并了，直接跳出循环
      break;
    }
  }
  // 将合并后的块加入到空闲链表
  lst_push(&buddy_sizes[k].free, p);
  release(&lock);
}



// 将[start, stop)范围的内存标记为已分配
void
buddy_mark(void *start, void *stop)
{
  int bi, bj;
  // 首先进行边界对齐的校验:起始和结束地址都要是LEAF_SIZE的整数倍
  if (((uint64) start % LEAF_SIZE != 0) || ((uint64) stop % LEAF_SIZE != 0))
    panic("buddy_mark");
//从最小块大小（0）开始，逐步处理每个块
  for (int k = 0; k < nsizes; k++) {
    bi = block_index(k, start);
    bj = block_index_next(k, stop);
    for(; bi < bj; bi++) {
      if(k > 0) {
        // 如果某个块在大小为k时已被分配，标记它为已拆分状态
        set_bit(buddy_sizes[k].split, bi);
      }
      flip_mutual_bit(buddy_sizes[k].alloc, bi);
    }
  }
}

// 如果块还未分配而其buddy块已被分配，则将它放入k大小块的空闲链表
int left;
int right;
int
buddy_initfree_pair(int k, int bi) {
  int buddy = (bi % 2 == 0) ? bi+1 : bi-1;
  int free = 0;
  // 先检查给定块是否已被标记为已分配
  if(get_mutual_bit(buddy_sizes[k].alloc, bi)){
    free = BLK_SIZE(k);
     // 根据块的索引判断是当前块还是其伙伴块放入空闲链表
    if(bi == left) {
      lst_push(&buddy_sizes[k].free, address(k, bi));
    }
    else
    {
      lst_push(&buddy_sizes[k].free, address(k, buddy));
    }
  }  
  return free;
}
  
// 初始化每个大小k的空闲链表
int
buddy_initfree(void *buddy_left, void *buddy_right) {
  int free = 0;

  for (int k = 0; k < MAXSIZE; k++) {   // 跳过最大大小
    left = block_index_next(k, buddy_left);
    right = block_index(k, buddy_right);
    //先处理left
    free += buddy_initfree_pair(k, left);
    if(right <= left)
      continue;
    free += buddy_initfree_pair(k, right);
  }
  return free;  //返回所有初始化后空闲内存的总大小（字节)
}


// 初始化buddy分配器：它管理从[base, end)对应的buddy_sizes数组的条目数量
void
buddy_init(void *base, void *end) {
  char *p = (char *) ROUNDUP((uint64)base, LEAF_SIZE);
  int sz;

  initlock(&lock, "buddy");
  buddy_base = (void *) p;

  // 计算需要管理的[base, end)内存的大小
  nsizes = log2(((char *)end-p)/LEAF_SIZE) + 1;
  if((char*)end-p > BLK_SIZE(MAXSIZE)) {
    nsizes++;  // 向上取整到下一个2的幂
  }
  // 分配buddy_sizes数组
  buddy_sizes = (Sz_info *) p;
  p += sizeof(Sz_info) * nsizes;
  memset(buddy_sizes, 0, sizeof(Sz_info) * nsizes); // 将buddy_sizes数组的内容初始化为0

  // 初始化空闲链表并为每个大小k分配alloc数组
  for (int k = 0; k < nsizes; k++) {
    lst_init(&buddy_sizes[k].free);
    sz = sizeof(char)* ROUNDUP(NBLK(k), 16)/8;
    sz /= 2;
    buddy_sizes[k].alloc = p;
    memset(buddy_sizes[k].alloc, 0, sz);
    p += sz;
  }

  // 为每个大小k分配split数组（除了k=0，因为我们不会拆分大小为0的块）
  for (int k = 1; k < nsizes; k++) {
    sz = sizeof(char)* (ROUNDUP(NBLK(k), 8))/8;
    buddy_sizes[k].split = p;
    memset(buddy_sizes[k].split, 0, sz);
    p += sz;
  }
  p = (char *) ROUNDUP((uint64) p, LEAF_SIZE);

  int meta = p - (char*)buddy_base;
  // 标记从buddy_base到p范围的内存为已分配
  buddy_mark(buddy_base, p);
  // 计算[end, HEAP_SIZE)范围不可用内存的大小
  int unavailable = BLK_SIZE(MAXSIZE)-(end-buddy_base);
  if(unavailable > 0)
    unavailable = ROUNDUP(unavailable, LEAF_SIZE);

  void *buddy_end = buddy_base+BLK_SIZE(MAXSIZE)-unavailable;
  buddy_mark(buddy_end, buddy_base+BLK_SIZE(MAXSIZE));
  
  // 初始化每个大小k的空闲链表，并得到初始化后空闲内存的总大小
  int free = buddy_initfree(p, buddy_end);
  // 检查空闲内存的数量是否符合预期
  if(free != BLK_SIZE(MAXSIZE)-meta-unavailable) {
    printf("free %d %d\n", free, BLK_SIZE(MAXSIZE)-meta-unavailable);
    panic("buddy_init: free mem");
  }
}

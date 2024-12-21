// 基于伙伴分配算法的内存管理系统
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

static int nsizes;     // buddy_sizes 数组的条目数量
#define LEAF_SIZE     16                         // 最小的块大小
#define MAXSIZE       (nsizes-1)                 // buddy_sizes 数组中最大的索引
#define BLK_SIZE(k)   ((1L << (k)) * LEAF_SIZE)  // 大小为 k 的块的大小
#define HEAP_SIZE     BLK_SIZE(MAXSIZE)          // 堆的总大小
#define NBLK(k)       (1 << (MAXSIZE-k))         // 大小为 k 的块的数量
#define ROUNDUP(n,sz) (((((n)-1)/(sz))+1)*(sz))  // 向上对齐到下一个 sz 的倍数


typedef struct list_node buddy_list;

// 每个 sz_info 都有一个空闲列表和一个数组分配器。
// alloc 和 split 数组的类型是 char（1字节），但实际使用时每个位表示一个块的状态
struct sz_info {
  buddy_list free;
  char *alloc; // 跟踪哪些数据块已被分配
  char *split; // 跟踪哪些数据块已被拆分
};
typedef struct sz_info Sz_info;

static Sz_info *buddy_sizes; // Buddy 分配器的大小数组
static void *buddy_base;   /// Buddy 分配器管理的内存起始地址
static struct spinlock lock;

// 判断数组中第 index 位是否为 1
int bit_is_set(char *array, int index) {
  char b = array[index/8];
  char m = (1 << (index % 8));
  return (b & m) == m;
}

// 将数组中第 index 位设置为 1
void set_bit(char *array, int index) {
  char b = array[index/8];
  char m = (1 << (index % 8));
  array[index/8] = (b | m);
}

// 将数组中第 index 位清除
void clear_bit(char *array, int index) {
  char b = array[index/8];
  char m = (1 << (index % 8));
  array[index/8] = (b & ~m);
}
// 将公用buddy的bit用一个来表示，互斥地翻转两个 buddy 的状态。
void flip_mutual_bit(char *array, int index) {
  index /= 2; // 因为是两两 buddy 对比
  if(bit_is_set(array, index)){
    clear_bit(array, index);
  }
  else
  {
    set_bit(array, index);
  }  
}
// 获取互斥 bit 的状态
int get_mutual_bit(char *array, int index){
  index /= 2;
  return bit_is_set(array, index);
}


// // 打印一个 bit 向量（按连续的 1 区间打印）
// void
// print_bit_vector(char *vector, int len) {
//   int last, lb;
  
//   last = 1;
//   lb = 0;
//   for (int b = 0; b < len; b++) {
//     if (last == bit_is_set(vector, b))
//       continue;
//     if(last == 1)
//       printf(" [%d, %d)", lb, b);
//     lb = b;
//     last = bit_is_set(vector, b);
//   }
//   if(lb == 0 || last == 1) {
//     printf(" [%d, %d)", lb, len);
//   }
//   printf("\n");
// }

// // 打印 Buddy 分配器的数据结构
// void
// print_buddy_allocator() {
//   for (int k = 0; k < nsizes; k++) {
//     printf("size %d (blksz %d nblk %d): free list: ", k, BLK_SIZE(k), NBLK(k));
//     lst_print(&buddy_sizes[k].free);
//     printf("  alloc:");
//     print_bit_vector(buddy_sizes[k].alloc, NBLK(k));
//     if(k > 0) {
//       printf("  split:");
//       print_bit_vector(buddy_sizes[k].split, NBLK(k));
//     }
//   }
// }

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

// 计算大小为 k 的块在内存中的索引
int
block_index(int k, char *p) {
  int n = p - (char *) buddy_base;
  return n / BLK_SIZE(k);
}

// 将大小为 k 的块的索引转换为内存地址
void *address(int k, int bi) {
  int n = bi * BLK_SIZE(k);
  return (char *) buddy_base + n;
}
// 查找一个块的大小
int
size(char *p) {
  for (int k = 0; k < nsizes; k++) {
    if(bit_is_set(buddy_sizes[k+1].split, block_index(k+1, p))) {
      return k;
    }
  }
  return 0;
}
// 计算第k大小块中不包含p的第一个块的索引
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

  acquire(&lock);
   // 从最小的 k 开始，寻找第一个足够大的空闲块
  fk = find_first_k(nbytes);
  for (k = fk; k < nsizes; k++) {
    if(!lst_empty(&buddy_sizes[k].free))
      break;
  }
  if(k >= nsizes) { // 没有足够的空闲块
    release(&lock);
    return 0;
  }

  // 找到空闲块，从空闲链表中移除，并进行可能的分裂
  char *p = lst_pop(&buddy_sizes[k].free);
  flip_mutual_bit(buddy_sizes[k].alloc, block_index(k, p));
  // 如果需要，分裂大块，直到满足需求
  for(; k > fk; k--) {
    char *q = p + BLK_SIZE(k-1);   // p 的 buddy 块
    set_bit(buddy_sizes[k].split, block_index(k, p));
    flip_mutual_bit(buddy_sizes[k-1].alloc, block_index(k-1, p));
    lst_push(&buddy_sizes[k-1].free, q);
  }
  // 页对齐：确保返回的内存地址是页大小的整数倍
  p = (char *)ROUNDUP((uint64)p, PAGE_SIZE);
  

  release(&lock);

  return p;
}



void
buddy_free(void *p) {
  void *q;
  int k;
  int bi;
  int buddy;
  int is_buddy_free;

  acquire(&lock);
  // 页对齐：确保释放的内存地址是页大小的整数倍
  p = (char *)ROUNDUP((uint64)p, PAGE_SIZE);

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

      // 标记合并
      set_bit(buddy_sizes[k + 1].split, block_index(k + 1, p));

      // 延迟合并：移除伙伴块
      q = address(k, buddy);
      lst_remove(q);  // 从空闲链表中移除 buddy

      // 如果当前块是偶数块，p 就是合并后的块
      if (buddy % 2 == 0) {
        p = q;
      }
    } else {
      // 伙伴已分配，跳过合并
      break;
    }
  }

  // 将合并后的块加入到空闲链表
  lst_push(&buddy_sizes[k].free, p);
  release(&lock);
}



// 将[start, stop)范围的内存标记为已分配，从大小0开始
void
buddy_mark(void *start, void *stop)
{
  int bi, bj;
  // 首先进行边界对齐的校验
  if (((uint64) start % LEAF_SIZE != 0) || ((uint64) stop % LEAF_SIZE != 0))
    panic("buddy_mark");

  for (int k = 0; k < nsizes; k++) {
    bi = block_index(k, start);
    bj = block_index_next(k, stop);
    for(; bi < bj; bi++) {
      if(k > 0) {
        // if a block is allocated at size k, mark it as split too.
        set_bit(buddy_sizes[k].split, bi);
      }
      flip_mutual_bit(buddy_sizes[k].alloc, bi);
    }
  }
}

// 如果块被标记为已分配且buddy是空闲的，则将其放入k大小块的空闲链表
int left;
int right;
int
buddy_initfree_pair(int k, int bi) {
  int buddy = (bi % 2 == 0) ? bi+1 : bi-1;
  int free = 0;
  if(get_mutual_bit(buddy_sizes[k].alloc, bi)){
    free = BLK_SIZE(k);

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
  
// 初始化每个大小k的空闲链表。
int
buddy_initfree(void *buddy_left, void *buddy_right) {
  int free = 0;

  for (int k = 0; k < MAXSIZE; k++) {   // 跳过最大大小
    left = block_index_next(k, buddy_left);
    right = block_index(k, buddy_right);
    free += buddy_initfree_pair(k, left);
    if(right <= left)
      continue;
    free += buddy_initfree_pair(k, right);
  }
  return free;
}


// 初始化buddy分配器：它管理从[base, end)的内存。
void
buddy_init(void *base, void *end) {
  char *p = (char *) ROUNDUP((uint64)base, LEAF_SIZE);
  int sz;

  initlock(&lock, "buddy");
  buddy_base = (void *) p;

  // 计算我们需要管理的[base, end)内存的大小
  nsizes = log2(((char *)end-p)/LEAF_SIZE) + 1;
  if((char*)end-p > BLK_SIZE(MAXSIZE)) {
    nsizes++;  // 向上取整到下一个2的幂
  }
  // 分配buddy_sizes数组
  buddy_sizes = (Sz_info *) p;
  p += sizeof(Sz_info) * nsizes;
  memset(buddy_sizes, 0, sizeof(Sz_info) * nsizes);

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
  buddy_mark(buddy_base, p);
  // 标记不可用内存范围[end, HEAP_SIZE)为已分配
  int unavailable = BLK_SIZE(MAXSIZE)-(end-buddy_base);
  if(unavailable > 0)
    unavailable = ROUNDUP(unavailable, LEAF_SIZE);

  void *buddy_end = buddy_base+BLK_SIZE(MAXSIZE)-unavailable;
  buddy_mark(buddy_end, buddy_base+BLK_SIZE(MAXSIZE));
  
  // 初始化每个大小k的空闲链表
  int free = buddy_initfree(p, buddy_end);
  // 检查空闲内存的数量是否符合预期
  if(free != BLK_SIZE(MAXSIZE)-meta-unavailable) {
    printf("free %d %d\n", free, BLK_SIZE(MAXSIZE)-meta-unavailable);
    panic("buddy_init: free mem");
  }
}

// 基于伙伴分配算法的内存管理系统
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

static int nsizes;   
#define LEAF_SIZE     16                         
#define MAXSIZE       (nsizes-1)                 
#define BLK_SIZE(k)   ((1L << (k)) * LEAF_SIZE)  
#define HEAP_SIZE     BLK_SIZE(MAXSIZE)          
#define NBLK(k)       (1 << (MAXSIZE-k))        
#define ROUNDUP(n,sz) (((((n)-1)/(sz))+1)*(sz))  


typedef struct list_node buddy_list;

struct sz_info {
  buddy_list free;
  char *alloc; 
  char *split; 
};
typedef struct sz_info Sz_info;

static Sz_info *buddy_sizes;
static void *buddy_base;  
static struct spinlock lock;


// 统计并输出内存碎片信息。统计方法为：碎片总量-计算所有空闲块的总内存; 碎片块数量分布：统计每种大小块的空闲数量。
void buddy_fragmentation_statistics() {
  acquire(&lock); // 加锁
  int total_free_mem = 0;
  int total_free_blocks = 0;

  printf("Buddy Memory Fragmentation Statistics:\n");
  printf("Size Index | Block Size | Free Blocks | Total Free Memory\n");
  printf("---------------------------------------------------------\n");

  for (int k = 0; k < nsizes; k++) {
    int free_blocks = 0;
    buddy_list *list = &buddy_sizes[k].free;

    for (struct list_node *node = list->next; node != list; node = node->next) {
      free_blocks++;
    }

    int block_size = BLK_SIZE(k);
    int free_memory = free_blocks * block_size;
    printf("%10d | %10d | %11d | %17d\n", k, block_size, free_blocks, free_memory);

    total_free_mem += free_memory;
    total_free_blocks += free_blocks;
  }

  printf("---------------------------------------------------------\n");
  printf("Total Free Memory: %d bytes in %d blocks\n", total_free_mem, total_free_blocks);
  release(&lock); 
}


int bit_is_set(char *array, int index) {
  char b = array[index/8];
  char m = (1 << (index % 8));
  return (b & m) == m;
}
void set_bit(char *array, int index) {
  char b = array[index/8];
  char m = (1 << (index % 8));
  array[index/8] = (b | m);
}

void clear_bit(char *array, int index) {
  char b = array[index/8];
  char m = (1 << (index % 8));
  array[index/8] = (b & ~m);
}
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
int get_mutual_bit(char *array, int index){
  index /= 2;
  return bit_is_set(array, index);
}

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

int
block_index(int k, char *p) {
  int n = p - (char *) buddy_base;
  return n / BLK_SIZE(k);
}

void *address(int k, int bi) {
  int n = bi * BLK_SIZE(k);
  return (char *) buddy_base + n;
}

int
size(char *p) {
  for (int k = 0; k < nsizes; k++) {
    if(bit_is_set(buddy_sizes[k+1].split, block_index(k+1, p))) {
      return k;
    }
  }
  return 0;
}

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

void *
buddy_malloc(uint64 nbytes)
{
  int fk, k;

  acquire(&lock);
  fk = find_first_k(nbytes);
  for (k = fk; k < nsizes; k++) {
    if(!lst_empty(&buddy_sizes[k].free))
      break;
  }
  if(k >= nsizes) { 
    release(&lock);
    return 0;
  }

  char *p = lst_pop(&buddy_sizes[k].free);
  flip_mutual_bit(buddy_sizes[k].alloc, block_index(k, p));
  for(; k > fk; k--) {
    char *q = p + BLK_SIZE(k-1);  
    set_bit(buddy_sizes[k].split, block_index(k, p));
    flip_mutual_bit(buddy_sizes[k-1].alloc, block_index(k-1, p));
    lst_push(&buddy_sizes[k-1].free, q);
  }

  p = (char *)ROUNDUP((uint64)p, PAGE_SIZE);
  

  release(&lock);
  buddy_fragmentation_statistics();

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
  p = (char *)ROUNDUP((uint64)p, PAGE_SIZE);

  for (k = size(p); k < MAXSIZE; k++) {
    bi = block_index(k, p);
    buddy = (bi % 2 == 0) ? bi + 1 : bi - 1;

    flip_mutual_bit(buddy_sizes[k].alloc, bi);  
    
    is_buddy_free = !get_mutual_bit(buddy_sizes[k].alloc, buddy);
    if (is_buddy_free) {

      set_bit(buddy_sizes[k + 1].split, block_index(k + 1, p));

      q = address(k, buddy);
      lst_remove(q);  

      if (buddy % 2 == 0) {
        p = q;
      }
    } else {
      break;
    }
  }

  lst_push(&buddy_sizes[k].free, p);
  release(&lock);
  buddy_fragmentation_statistics();
}


void
buddy_mark(void *start, void *stop)
{
  int bi, bj;
  if (((uint64) start % LEAF_SIZE != 0) || ((uint64) stop % LEAF_SIZE != 0))
    panic("buddy_mark");

  for (int k = 0; k < nsizes; k++) {
    bi = block_index(k, start);
    bj = block_index_next(k, stop);
    for(; bi < bj; bi++) {
      if(k > 0) {
        set_bit(buddy_sizes[k].split, bi);
      }
      flip_mutual_bit(buddy_sizes[k].alloc, bi);
    }
  }
}

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
  
int
buddy_initfree(void *buddy_left, void *buddy_right) {
  int free = 0;

  for (int k = 0; k < MAXSIZE; k++) { 
    left = block_index_next(k, buddy_left);
    right = block_index(k, buddy_right);
    free += buddy_initfree_pair(k, left);
    if(right <= left)
      continue;
    free += buddy_initfree_pair(k, right);
  }
  return free;
}

void
buddy_init(void *base, void *end) {
  char *p = (char *) ROUNDUP((uint64)base, LEAF_SIZE);
  int sz;

  initlock(&lock, "buddy");
  buddy_base = (void *) p;

  nsizes = log2(((char *)end-p)/LEAF_SIZE) + 1;
  if((char*)end-p > BLK_SIZE(MAXSIZE)) {
    nsizes++; 
  }

  buddy_sizes = (Sz_info *) p;
  p += sizeof(Sz_info) * nsizes;
  memset(buddy_sizes, 0, sizeof(Sz_info) * nsizes);

  for (int k = 0; k < nsizes; k++) {
    lst_init(&buddy_sizes[k].free);
    sz = sizeof(char)* ROUNDUP(NBLK(k), 16)/8;
    sz /= 2;
    buddy_sizes[k].alloc = p;
    memset(buddy_sizes[k].alloc, 0, sz);
    p += sz;
  }

  for (int k = 1; k < nsizes; k++) {
    sz = sizeof(char)* (ROUNDUP(NBLK(k), 8))/8;
    buddy_sizes[k].split = p;
    memset(buddy_sizes[k].split, 0, sz);
    p += sz;
  }
  p = (char *) ROUNDUP((uint64) p, LEAF_SIZE);

  int meta = p - (char*)buddy_base;
  buddy_mark(buddy_base, p);
  int unavailable = BLK_SIZE(MAXSIZE)-(end-buddy_base);
  if(unavailable > 0)
    unavailable = ROUNDUP(unavailable, LEAF_SIZE);

  void *buddy_end = buddy_base+BLK_SIZE(MAXSIZE)-unavailable;
  buddy_mark(buddy_end, buddy_base+BLK_SIZE(MAXSIZE));
  

  int free = buddy_initfree(p, buddy_end);
  if(free != BLK_SIZE(MAXSIZE)-meta-unavailable) {
    printf("free %d %d\n", free, BLK_SIZE(MAXSIZE)-meta-unavailable);
    panic("buddy_init: free mem");
  }
  buddy_fragmentation_statistics();
}

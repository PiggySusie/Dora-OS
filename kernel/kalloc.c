#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel, defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  int free_count;       // Count of free pages
  int allocated_count;  // Count of allocated pages
} kmem;

// Print memory fragmentation statistics.
void
kmem_fragmentation_statistics()
{
  acquire(&kmem.lock);

  if (kmem.free_count < 0 || kmem.allocated_count < 0) {
    panic("Invalid kmem counters in fragmentation statistics");
  }

  int total_memory = kmem.free_count * PGSIZE;
  int allocated_memory = kmem.allocated_count * PGSIZE;

  //printf("Memory Fragmentation Statistics:\n");
  //printf("--------------------------------\n");
  printf("Total Free Pages: %d, ", kmem.free_count);
  printf("Total Allocated Pages: %d,", kmem.allocated_count);
  printf("Total Free Memory: %d bytes, ", total_memory);
  printf("Total Allocated Memory: %d bytes\n", allocated_memory);
  //printf("--------------------------------\n");

  release(&kmem.lock);
}


void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.free_count = 0;
  kmem.allocated_count = 0;
  freerange(end, (void*)PHYSTOP);
  kmem_fragmentation_statistics(); 
}


void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);

  acquire(&kmem.lock); 
  for (; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    struct run *r = (struct run*)p;
    r->next = kmem.freelist;
    kmem.freelist = r;
    kmem.free_count++; 
  }
  release(&kmem.lock);
}

void
kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree: invalid address");

  acquire(&kmem.lock);

 
  for (struct run *cur = kmem.freelist; cur != 0; cur = cur->next) {
    if (cur == pa) {
      release(&kmem.lock);
      panic("kfree: double free detected");
    }
  }


  r = (struct run*)pa;
  r->next = kmem.freelist;
  kmem.freelist = r;

  kmem.free_count++;
  if (kmem.allocated_count > 0) {  
    kmem.allocated_count--;
  }

  release(&kmem.lock);
  kmem_fragmentation_statistics();
}


void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r) {
    kmem.freelist = r->next;
    kmem.free_count--;
    if (kmem.free_count < 0) {  
      release(&kmem.lock);
      panic("free_count underflow in kalloc");
    }
    kmem.allocated_count++;
  }
  release(&kmem.lock);

  if (r)
    memset((char*)r, 5, PGSIZE); 
  kmem_fragmentation_statistics();
  return (void*)r;
}


#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/memlayout.h"
#include "user/user.h"

// Allocate all free memory and count how it is
void test1()
{
  void *a;
  int tot = 0;
  char buf[1];
  int fds[2];
  
  printf("memtest: start\n");  
  if(pipe(fds) != 0){
    printf("pipe() failed\n");
    exit(1);
  }
  int pid = fork();
  if(pid < 0){
    printf("fork failed");
    exit(1);
  }
  if(pid == 0){
      close(fds[0]);
      while(1) {
        a = sbrk(PGSIZE);
        if (a == (char*)0xffffffffffffffffL)
          exit(0);
        *(int *)(a+4) = 1;
        if (write(fds[1], "x", 1) != 1) {
          printf("write failed");
          exit(1);
        }
      }
      exit(0);
  }
  close(fds[1]);
  while(1) {
      if (read(fds[0], buf, 1) != 1) {
        break;
      } else {
        tot += 1;
      }
  }
  //int n = (PHYSTOP-KERNBASE)/PGSIZE;
  //printf("allocated %d out of %d pages\n", tot, n);
  if(tot < 31950) {
    printf("expected to allocate at least 31950, only got %d\n", tot);
    printf("memtest: FAILED\n");  
  } else {
    printf("memtest: OK\n");  
  }
}

int
main(int argc, char *argv[])
{
  test1();
  exit(0);
}

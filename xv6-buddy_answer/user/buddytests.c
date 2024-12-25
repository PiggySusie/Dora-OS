// #include "kernel/types.h"
// #include "user/user.h"
// #include "kernel/param.h"

// void buddytest() {
//     void *ptr1, *ptr2, *ptr3;
//     ptr1 = buddy_malloc();
//     if (ptr1 == 0) {
//         printf("kalloc failed for ptr1\n");
//         return;
//     }
//     printf("Allocated memory at %p\n", ptr1);

//     ptr2 = buddy_malloc();
//     if (ptr2 == 0) {
//         printf("kalloc failed for ptr2\n");
//         return;
//     }
//     printf("Allocated memory at %p\n", ptr2);

//     ptr3 = buddy_malloc();
//     if (ptr3 == 0) {
//         printf("kalloc failed for ptr3\n");
//         return;
//     }
//     printf("Allocated memory at %p\n", ptr3);

    
//     buddy_free(ptr1);
//     printf("Freed memory at %p\n", ptr1);

//     buddy_free(ptr2);
//     printf("Freed memory at %p\n", ptr2);

//     buddy_free(ptr3);
//     printf("Freed memory at %p\n", ptr3);
//     printf("buddytest: OK\n");
// }

// int main() {
//     buddytest();

//     return 0;
// }
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fcntl.h"
#include "kernel/memlayout.h"
#include "user/user.h"


int buddytest() {
    void *ptr1, *ptr2, *ptr3;
    ptr1 = buddy_malloc();
    if (ptr1 == 0) {
        printf("buddy_malloc failed for ptr1\n");
        return 0;  
    }
    printf("Allocated memory at %p\n", ptr1);

    ptr2 = buddy_malloc();
    if (ptr2 == 0) {
        printf("buddy_malloc failed for ptr2\n");
        return 0;  
    }
    printf("Allocated memory at %p\n", ptr2);

    ptr3 = buddy_malloc();
    if (ptr3 == 0) {
        printf("buddy_malloc failed for ptr3\n");
        return 0; 
    }
    printf("Allocated memory at %p\n", ptr3);

    buddy_free(ptr1);
    printf("Freed memory at %p\n", ptr1);

    buddy_free(ptr2);
    printf("Freed memory at %p\n", ptr2);

    buddy_free(ptr3);
    printf("Freed memory at %p\n", ptr3);
    printf("buddytest: OK\n");
    
    return 1; 
}


void optimaltest() {
    void *a;
    int tot = 0;
    char buf[1];
    int fds[2];

    printf("optimaltest: start\n");
    if (pipe(fds) != 0) {
        printf("pipe() failed\n");
        exit(1);
    }
    int pid = fork();
    if (pid < 0) {
        printf("fork failed");
        exit(1);
    }
    if (pid == 0) {
        close(fds[0]);
        while (1) {
            a = sbrk(PGSIZE);
            if (a == (char *)0xffffffffffffffffL)
                exit(0);
            *(int *)(a + 4) = 1;
            if (write(fds[1], "x", 1) != 1) {
                printf("write failed");
                exit(1);
            }
        }
        exit(0);
    }
    close(fds[1]);
    while (1) {
        if (read(fds[0], buf, 1) != 1) {
            break;
        } else {
            tot += 1;
        }
    }

    if (tot < 31950) {
        printf("expected to allocate at least 31950, only got %d\n", tot);
        printf("optimaltest: FAILED\n");
    } else {
        printf("optimaltest: OK\n");
    }
}

int main(int argc, char *argv[]) {
    if (buddytest()) {
        optimaltest();  
    } else {
        printf("buddytest failed, skipping optimaltest\n");
    }
    exit(0);
}

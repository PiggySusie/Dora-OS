#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

void buddytests() {
    void *ptr1, *ptr2, *ptr3;
    
    // 分配内存页面
    ptr1 = kalloc();
    if (ptr1 == 0) {
        printf("kalloc failed for ptr1\n");
        return;
    }
    printf("Allocated memory at %p\n", ptr1);

    // 分配第二个页面
    ptr2 = kalloc();
    if (ptr2 == 0) {
        printf("kalloc failed for ptr2\n");
        return;
    }
    printf("Allocated memory at %p\n", ptr2);

    // 分配第三个页面
    ptr3 = kalloc();
    if (ptr3 == 0) {
        printf("kalloc failed for ptr3\n");
        return;
    }
    printf("Allocated memory at %p\n", ptr3);

    // 释放内存
    kfree(ptr1);
    printf("Freed memory at %p\n", ptr1);

    kfree(ptr2);
    printf("Freed memory at %p\n", ptr2);

    kfree(ptr3);
    printf("Freed memory at %p\n", ptr3);
    printf("buddytests: OK\n");
}

int main() {
    // 测试 buddy 分配器
    buddytests();

    return 0;
}

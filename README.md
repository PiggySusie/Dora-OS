# 基于伙伴分配算法的内存管理系统

## 1、基本信息

- **所属学校**：华东师范大学

- **比赛方向**：OS原理赛道—模块实验创新

- **队伍编号**：T202410269994414 

- **队伍名**：朵拉探险队

- **队伍成员及分工**：

  | 姓名   | 学号        | 负责功能                                             |
  | ------ | ----------- | ---------------------------------------------------- |
  | 朱子玥 | 10230720412 | 项目框架设计、代码整理与主体代码编写，实验指导书编写 |
  | 贾馨雨 | 10235501437 | 代码整理、代码分析与注释，实验指导书编写             |
  | 韩悦   | 10213903418 | 代码整理、内存碎片统计功能实现，实验指导书编写       |

- **指导老师**：石亮



## 2、项目完成情况介绍

​	本项目完成了在xv6操作系统中伙伴分配器的实现。伙伴分配器是一种高效的内存分配算法，它将内存划分为大小为2的幂次方的块，并通过合并和拆分这些块来优化内存的使用，以优化内存分配和减少碎片化。

1. **优化内存分配效率**：
   - 实现了伙伴分配器，通过内存块的拆分和合并，有效减少了内存碎片化。
   - 分配和回收内存时具有较高的效率，能够快速定位到合适大小的空闲内存块。
2. **增强内存管理的可预测性**：
   - 伙伴分配器将内存划分为固定大小（2的幂次方）的内存块，使得内存管理更加规则和可预测。
   - 能够动态适应内存需求变化，在系统运行过程中灵活地分配和回收内存块。



## 3、实验目标达成情况

- **内存分配与释放**：实现了`buddy_malloc`和`buddy_free`函数，能够正确地分配和释放指定大小的内存块。
- **内存初始化与管理**：实现了`buddy_init`函数，用于初始化内存管理系统，包括计算内存大小、初始化空闲链表、分配管理结构和标记已使用区域。
- **空闲链表的管理**：通过双向链表来动态地管理内存块的分配和释放，提高了内存分配和释放的效率。
- **内存碎片统计**：编写了`buddy_fragmentation_statistics`函数，用于统计并输出内存碎片信息，包括每种大小的空闲块数量及其占用的总内存等。
- **进一步优化**：利用一个比特位表示一对 `buddy` 内存块的状态，以节省内存，提升内存利用率。

## 4、实验过程概述

1. **理解伙伴分配算法**：
   - 深入学习了伙伴分配算法的基本概念，包括内存块划分、分配过程、合并机制等。
2. **查看内存管理结构**：
   - 分析了xv6操作系统中现有的内存管理结构，并设计了用于实现伙伴分配器的数据结构。
3. **实现初始化**：
   - 实现了`buddy_init`函数，用于初始化内存管理系统，包括计算内存大小、初始化空闲链表、分配管理结构和标记已使用区域。
4. **实现内存分配与释放**：
   - 实现了`buddy_malloc`函数，用于分配指定大小的内存块。
   - 实现了`buddy_free`函数，用于释放内存块并进行合并操作。
5. **测试与验证**：
   - 通过运行`buddytests`测试，验证了伙伴分配器的基本功能和性能。
   - 优化了伙伴分配器，使用`XOR`操作来判断`buddy`块的状态，进一步节省了内存。



## 5、实验创新与难度

- **创新性**：
  - 在实现基本的伙伴分配算法基础上，进行了优化，基于 **XOR** 操作减少了内存管理中对比特位的冗余使用，显著提高了内存块状态的管理效率，降低了内存开销。这个优化方案可以在较大内存环境下有效节省资源，提升系统性能。在释放内存时，通过 XOR 操作判断是否需要合并相邻的 **buddy** 块，并根据块的大小进行动态调整。这种内存管理方法能够减少碎片化，提高内存的整体利用率。
  - 建立了系统调用，使用户态下的测试文件能够通过系统调用访问内核态的伙伴分配器。

- **难度**：
  - 伙伴分配算法的实现涉及复杂的内存管理逻辑，包括内存块的拆分、合并、查找等。
  - 需要深入理解xv6操作系统的内存管理结构，并进行相应的修改和扩展。
  - 测试与验证过程需要确保伙伴分配器的正确性和稳定性，尤其是在大量内存分配和释放的场景下。



## 6、实验指导书和过程性材料

- 实验指导书PDF文档：[基于伙伴分配算法的内存管理系统-实验指导书](<https://gitlab.eduxiji.net/T202410269994414/project2608132-274517/-/blob/main/%E5%9F%BA%E4%BA%8E%E4%BC%99%E4%BC%B4%E5%88%86%E9%85%8D%E7%AE%97%E6%B3%95%E7%9A%84%E5%86%85%E5%AD%98%E7%AE%A1%E7%90%86%E7%B3%BB%E7%BB%9F-%E5%AE%9E%E9%AA%8C%E6%8C%87%E5%AF%BC%E4%B9%A6.pdf>)
- 所有材料的压缩文件：[zip](<https://gitlab.eduxiji.net/T202410269994414/project2608132-274517/-/tree/main/Experimental%20Process%20Materials-zip>)
- 仓库中提供了四个分支，对应了实验过程性材料：
  - 实验题目的完整工程文件：[buddy-text分支](<https://gitlab.eduxiji.net/T202410269994414/project2608132-274517/-/tree/buddy-text?ref_type=heads>)
  - 实验答案的完整工程文件：[buddy-answer分支](<https://gitlab.eduxiji.net/T202410269994414/project2608132-274517/-/tree/buddy-answer?ref_type=heads>)
  - `xv6`原内存分配的输出验证性文件：[print-without-buddy分支](<https://gitlab.eduxiji.net/T202410269994414/project2608132-274517/-/tree/print-without-buddy?ref_type=heads>)
  - 添加伙伴分配器后内存分配的输出验证性文件：[print-with-buddy分支](<https://gitlab.eduxiji.net/T202410269994414/project2608132-274517/-/tree/print-with-buddy?ref_type=heads>)



## 7、测试方法

1. **克隆远程仓库**：

   ```bash
   git clone https://gitlab.eduxiji.net/T202410269994414/project2608132-274517.git
   
   cd project2608132-274517
   ```

   以下操作均在project2608132-274517文件夹下进行。

2. **查看所有远程分支**：

   ```bash
   git branch -r
   ```

   这将列出所有远程分支，应该会看到类似以下的输出：

   ```bash
    origin/HEAD -> origin/main
     origin/buddy-answer
     origin/buddy-text
     origin/main
     origin/print-with-buddy
     origin/print-without-buddy
   ```

   以下以`buddy-answer`分支为例。

3. **创建并切换到本地分支 `buddy-answer`**：

   ```bash
   git checkout -b buddy-answer origin/buddy-answer
   ```

4. **在此分支下进行测试，包括：**

   - make qemu
   - buddytests
   - usertests
   - grade-lab-buddy

## 8、参考资料

[1]（美）AbrahamSilberschatz，（美）PeterBaerGalvin，（美）格雷格·加涅.操作系统概念原书  第10版[M].北京：机械工业出版社,2023

[2] xv6操作系统源代码 [mit-pdos/xv6-public: xv6 OS](https://github.com/mit-pdos/xv6-public)

[3] xv6手册  [xv6: a simple, Unix-like teaching operating system](https://pdos.csail.mit.edu/6.828/2021/xv6/book-riscv-rev2.pdf)

[4] Lab: Allocator for xv6  https://pdos.csail.mit.edu/6.828/2019/labs/alloc.html

[5]MIT 6.S081 https://blog.csdn.net/zzy980511/category_11740137.html


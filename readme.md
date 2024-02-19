# Malloc Implementation

## Overview
Malloc (**m**emory **alloc**ation) is a standard library function in C that allocates memory dynamically during runtime. The function returns a pointer to the allocated memory, or `NULL` if the request fails. After the memory is no longer needed, it should be deallocated using the `free` function.

## Malloc Design Requirements
 - 16-byte alignment

## Goals
 - Maximize memory utilization
 - Maximize throughput
 - Minimize fragmentation

## Malloc Implementation

### Headers & Footers (Structure)
In memory management, the use of headers and footers provides essential structural organization to heap blocks. These structures aid in efficient traversal of the heap, allowing us to move from one block to another via pointer arithmetic. Typically, the header contains crucial information such as the size of the block and its allocation status (free or allocated). Similarly, the footer mirrors this information, serving as a redundant check. When a block is freed, these header and footer values are updated accordingly to reflect its new status. Thus, by employing headers and footers, we establish a systematic approach to navigating and managing memory allocation within the heap.

```
Heade/Footer Structure:
+--------------------------+---+---+---+---+
|           Size           | A | P | N | E |
+--------------------------+---+---+---+---+
|          60 bits         | 1 | 1 | 1 | 1 |
+--------------------------+---+---+---+---+
``` 
 - S: Size of the block (with header, data, and footer)
 - A: Allocation status
 - P: Previous block allocation status
 - N: Next block allocation status
 - E: Epilogue flag

### Heap Traversal (Free Blocks)
Heap traversal is a fundamental operation in memory management, especially when searching for free blocks to allocate. Utilizing information stored in headers and footers, we can efficiently traverse the heap from one block to another. By examining the allocation status indicated in these structures, we can identify free blocks and proceed accordingly with memory operations. This traversal mechanism enables the dynamic allocation and deallocation of memory within the heap, contributing to the overall efficiency of memory management algorithms.

### Increase Heap Size
When the heap reaches its capacity limit, necessitating the allocation of additional memory, increasing the heap size becomes imperative. This process involves dynamically expanding the heap to accommodate more memory allocations. By extending the heap's size, we ensure the availability of sufficient memory space for ongoing program execution and data storage requirements, thereby preventing memory exhaustion errors and facilitating seamless memory management operations.

### Coalescing (Free Blocks)
Coalescing refers to the merging of adjacent free memory blocks into larger contiguous blocks. This practice helps alleviate fragmentation issues within the heap, as it consolidates fragmented memory regions into more significant, more usable blocks. By identifying and merging neighboring free blocks, we reduce the fragmentation overhead and enhance memory utilization efficiency, ultimately optimizing the performance of memory allocation and deallocation processes.

### Splitting (Free Blocks)
In scenarios where a free block exceeds the size required for a memory allocation, splitting comes into play. This technique involves dividing a larger free block into two separate blocks: one sized to fulfill the allocation request and the other retaining the remaining memory space. By splitting free blocks judiciously, we maximize memory utilization without compromising efficiency. This approach is instrumental in accommodating varying memory allocation needs while mitigating potential wastage of available memory resources.

### Wilderness Preservation (Free Blocks)
Wilderness preservation entails leveraging the last free block in the heap to satisfy memory allocation requests instead of immediately expanding the heap size. By extending the size of the last free block to accommodate additional memory needs, we optimize memory utilization and minimize unnecessary heap expansions. This strategy contributes to more efficient memory management by capitalizing on existing free memory space before resorting to heap enlargement.

### Free Lists (Free Blocks)
Introducing free lists enhances the efficiency of locating and allocating free memory blocks within the heap. By maintaining a linked list of free blocks, memory management algorithms can quickly identify available memory regions without traversing the entire heap. Storing pointers to the next free block within the free space of each payload facilitates rapid access and manipulation of the free block list. Employing doubly linked lists further streamlines block removal operations, thereby improving the overall performance of memory allocation and deallocation processes.

### Segregated Free Lists (Free Blocks)
Segregated free lists optimize memory allocation by categorizing free blocks based on their sizes. Rather than relying on a single free list, this approach maintains multiple lists tailored to accommodate specific block sizes. By partitioning free blocks into segregated lists, memory management algorithms can expedite the search for suitable memory regions, enhancing both speed and efficiency. This strategy minimizes fragmentation and maximizes memory utilization by aligning allocation requests with appropriately sized free blocks.

### Footer Optimization (Structure)
Footer optimization offers a space-efficient alternative to traditional header and footer structures for managing free blocks. Instead of assigning a footer to every block, this approach selectively applies footers solely to free blocks, thereby conserving memory. By embedding footers within the free space of payload blocks, we reduce overhead while still facilitating essential memory management operations. This optimization technique contributes to a more streamlined memory management process, particularly in scenarios where memory footprint reduction is paramount.

```
Heap Example:
           <-Allocated Block-><----------------Free Block---------------->
+----------+----------+------+----------+---------------------+----------+----------+
|  Pr Hdr  |  Header  | Data |  Header  |      Free Block     |  Footer  |  Ep Hdr  |
+----------+----------+------+----------+---------------------+----------+----------+
|  S  |APNE|  S  |APNE| .... |  S  |APNE|    PREV - |  NEXT - |  S  |APNE|  S  |APNE|
+----------+----------+------+----------+---------------------+----------+----------+
```
 - S: Size of the block (with header, data, and footer)
 - A: Allocation status
 - P: Previous block allocation status
 - N: Next block allocation status
 - E: Epilogue flag
 - PREV*: Pointer to the previous free block
 - NEXT*: Pointer to the next free block

### Mini Blocks (Structure)
Mini blocks introduce a finer granularity in memory allocation by addressing blocks of smaller sizes, typically 8 bytes in a 64-bit system. This approach optimizes memory utilization by accommodating smaller memory allocations more efficiently. Mini blocks leverage singly linked lists stored within the free space of payloads, enabling rapid traversal and allocation of these smaller memory regions. While introducing additional complexity, mini blocks offer a tailored solution for managing smaller memory allocations effectively.

### Slabs (Structure)
Slabs offer a specialized approach to memory management, particularly suited for scenarios where fixed-size memory allocations are predominant. By organizing memory into fixed-size blocks within slabs, we eliminate the overhead associated with headers and footers, streamlining memory allocation processes. Slabs simplify memory management by grouping similar-sized allocations together, facilitating faster allocation and deallocation operations. This structured approach optimizes memory utilization and enhances overall system performance, especially in applications characterized by repetitive memory allocation patterns.


## Resources
 - [A Memory Allocator by Doug Lea](https://gee.cs.oswego.edu/dl/html/malloc.html)
 - [glibc Implantation](https://raw.githubusercontent.com/bminor/glibc/master/malloc/malloc.c)
 - [Free-Space Management](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-freespace.pdf)
 - [Vmalloc: A General and Efficient Memory Allocator](https://onlinelibrary.wiley.com/doi/epdf/10.1002/%28SICI%291097-024X%28199603%2926%3A3%3C357%3A%3AAID-SPE15%3E3.0.CO%3B2-%23)
 - [Vmalloc Implementation](https://opensource.apple.com/source/ksh/ksh-13/ksh/src/lib/libast/include/vmalloc.h.auto.html)
 - [A Good Malloc Implementation](https://github.com/hzj010427/Memory-Allocator/blob/8b187577419fc8bc44ffd1b1996d20e34e46d9fa/mm.c)
 - [A Good Malloc Implementation](https://www.cs.cmu.edu/afs/cs/academic/class/15213-f00/public/www/labs/malloc/malloc.html)

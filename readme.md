# Malloc Implementation

## Overview
This project implements a simplistic version of memory allocation functions `malloc`, `free`, `realloc`, and `calloc`. The allocator utilizes a structure named `divider` to represent headers (and footers in the case of free blocks). This structure contains essential information about the block such as size, allocation status, and neighboring block allocation status. Moreover, the structure is designed to be 8 bytes in size to ensure alignment to 16 bytes.

## Initialization
Upon initialization, the heap is allocated with a prologue and an epilogue. The prologue consists of a header marking the beginning of the heap, while the epilogue signifies the end. These blocks serve to handle edge cases efficiently.

## Malloc
The `malloc` function searches for a suitable free block in the heap, categorized by size. This categorization allows quicker allocation by narrowing down the search space. Instead of scanning through the entire heap, the allocator can focus on blocks likely to fulfill the size requirement. Additionally, `malloc` extends the heap when necessary to accommodate larger allocation requests, thus reducing the frequency of system calls for heap expansion.

## Free
The `free` function updates the allocation status of a block and creates a footer. This optimization enables efficient tracking of allocated and free blocks within the heap. Furthermore, the `free` function coalesces adjacent free blocks to prevent fragmentation. Coalescing merges neighboring free blocks into larger contiguous ones, reducing unusable fragments in the heap and improving memory utilization.

## Realloc
The `realloc` function reallocates a new block, transfers data from the old block, and subsequently frees the old block. This optimization avoids unnecessary copying of data when resizing memory blocks. It modifies the existing block if sufficient space is available, allocating a new block only when necessary.

## Divider Structure
The divider structure consists of:
- Size: 60 bits for block size
- A: Allocation status
- P: Previous block allocation status
- N: Next block allocation status
- E: Epilogue flag

## Heap Example
```
           <-Allocated Block-><----------------Free Block---------------->
+----------+----------+------+----------+---------------------+----------+----------+
|  Pr Hdr  |  Header  | Data |  Header  |      Free Block     |  Footer  |  Ep Hdr  |
+----------+----------+------+----------+---------------------+----------+----------+
|  S  |APNE|  S  |APNE| .... |  S  |APNE|    PREV*  |  NEXT*  |  S  |APNE|  S  |APNE|
+----------+----------+------+----------+---------------------+----------+----------+
```

Where:
- S: Size of the block (with header, data, and footer)
- A: Allocation status
- P: Previous block allocation status
- N: Next block allocation status
- E: Epilogue flag
- PREV*: Pointer to the previous free block
 NEXT*: Pointer to the next free block

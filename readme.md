# Memory Allocator

## Overview
This memory allocator, implemented in C by Aditya Kendre, provides a simplistic version of `malloc`, `free`, `realloc`, and `calloc` functionalities. It operates using a structure named `divider`, representing both headers and footers of allocated blocks. This structure contains information about the size, allocation status, and neighboring block statuses. The allocator is designed with alignment considerations in mind, ensuring that the heap is aligned to 8 bytes.

## Initialization
Upon initialization, the heap is allocated with a prologue and an epilogue. The prologue consists of a header and footer marking the beginning of the heap, while the epilogue signifies its end. These blocks serve to mitigate edge cases and ensure proper heap management.

## Functionality
- **malloc**: Searches for a free block in the heap or extends the heap if necessary based on the size requirement.
- **free**: Updates the allocation status of a block in both the header and footer.
- **realloc**: Allocates a new block, transfers data from the old block, and subsequently frees the old block.

## Divider Structure
The `divider` structure is designed as follows:
```
+--------------------------+---+---+---+---+
|           Size           | A | P | N | E |
+--------------------------+---+---+---+---+
|          60 bits         | 1 | 1 | 1 | 1 |
+--------------------------+---+---+---+---+
```
- **Size**: Size of the block (with header, data, and footer)
- **A**: Allocation status
- **P**: Previous block allocation status
- **N**: Next block allocation status
- **E**: Epilogue flag

## Heap Example
```
+-----------------+-----------------+-----------------+-----------------+---------+-----------------+-----------------+
|     Padding     | Prologue Header | Prologue Footer |      Header     |   Data  |      Footer     | Epilogue Header |
+-----------------+-----------------+-----------------+-----------------+---------+-----------------+-----------------+
|                 |   Size  |A|P|N|E|   Size  |A|P|N|E|   Size  |A|P|N|E|   ....  |   Size  |A|P|N|E|   Size  |A|P|N|E|
+-----------------+-----------------+-----------------+-----------------+---------+-----------------+-----------------+
```
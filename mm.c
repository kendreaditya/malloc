/*
 * mm.c
 *
 * Name: Aditya Kendre
 *
 * Overview:
 * Here we implement a simplistic version of malloc, free, realloc and calloc. The
 * allocator uses a structure named divider to represent both headers and footers of
 * allocated blocks. This structure contains information about the size, allocation
 * status, and whether the neighboring blocks are allocated or not. Additionally, the
 * structure is designed to be 8 bytes in size to ensure that the heap is aligned to
 * 8 bytes.
 *
 * When initialized, the heap is allocated with a prologue and an epilogue. The
 * prologue consists of a header and footer marking the beginning of the heap,
 * while the epilogue signifies the end of the heap. The main function of these
 * blocks is to decrease the instances of edge cases.
 *
 * The malloc function searches for a free block in the heap or extends the heap
 * if necessary based on the size requirement. The free function, on the other hand,
 * updates the allocation status of a block in the header and footer. The realloc
 * function is implemented to allocate a new block, transfer data from the
 * old block, and subsequently free the old block.
 *
 * Divider Structure:
 * +--------------------------+---+---+---+---+
 * |           Size           | A | P | N | E |
 * +--------------------------+---+---+---+---+
 * |          60 bits         | 1 | 1 | 1 | 1 |
 * +--------------------------+---+---+---+---+
 *
 * Heap Example:
 * +-----------------+-----------------+-----------------+-----------------+---------+-----------------+-----------------+
 * |     Padding     | Prologue Header | Prologue Footer |      Header     |   Data  |      Footer     | Epilogue Header |
 * +-----------------+-----------------+-----------------+-----------------+---------+-----------------+-----------------+
 * |                 |   Size  |A|P|N|E|   Size  |A|P|N|E|   Size  |A|P|N|E|   ....  |   Size  |A|P|N|E|   Size  |A|P|N|E|
 * +-----------------+-----------------+-----------------+-----------------+---------+-----------------+-----------------+
 *
 * where ...
 *  Size: Size of the block (with header, data, and footer)
 *  A: Allocation status
 *  P: Previous block allocation status
 *  N: Next block allocation status
 *  E: Epilogue flag
 * 
 * Coalescing:
 * TODO: Add description
 * 
 * Free List:
 * TODO: Add description
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include "mm.h"
#include "memlib.h"

/*
 * If you want to enable your debugging output and heap checker code,
 * uncomment the following line. Be sure not to have debugging enabled
 * in your final submission.
 */
// #define DEBUG

#ifdef DEBUG
// When debugging is enabled, the underlying functions get called
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
// When debugging is disabled, no code gets generated
#define dbg_printf(...)
#define dbg_assert(...)
#endif // DEBUG

// do not change the following!
#ifdef DRIVER
// create aliases for driver tests
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mm_memset
#define memcpy mm_memcpy
#endif // DRIVER

#define ALIGNMENT 16

// rounds up to the nearest multiple of ALIGNMENT
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

/**
 * The divider struct is used for headers and footers as to split data block in memory.
 * It includes information about the size, allocation status, and neighboring blocks.
**/

typedef struct {
    uint64_t size : 60;      // Size of the memory block                60 bits
    uint16_t alloc : 1;      // Allocation status                       1 bit
    uint16_t prev_alloc : 1; // Previous block allocation status        1 bit
    uint16_t next_alloc : 1; // Next block allocation status            1 bit
    uint16_t epilogue : 1;   // Epilogue flag                           1 bit
} divider;

#define DIVIDER_SIZE sizeof(divider)

/**
 * The free_block struct is create explicit free list to keep track of free blocks in the heap.
 * It includes pointers to the previous and next free blocks.
*/
typedef struct free_block {
    struct free_block *prev_free_block; // Pointer to the previous free block
    struct free_block *next_free_block; // Pointer to the next free block
} free_block;

void *heap; // Pointer to the first byte of the heap
free_block *free_list; // Pointer to the first free block in the free list

/*
 * Checks if a given pointer is within the valid heap range.
 *
 * Parameters:
 * p (const void*): The pointer to be checked.
 *
 * Returns:
 * bool: True if the pointer is within the heap, false otherwise.
 */
static bool in_heap(const void* p)
{
    return p <= mm_heap_hi() && p >= mm_heap_lo();
}
/**
 * Creates a divider struct with specified attributes for use as header or footer.
 *
 * Parameters:
 * size (unit64_t): The size of the memory block represented by the divider.
 * alloc (bool): Allocation status (true if allocated, false if free).
 * prev_alloc (bool): Previous block allocation status (true if allocated, false if free).
 * next_alloc (bool): Next block allocation status (true if allocated, false if free).
 * epilogue (bool): Epilogue flag (true if it's an epilogue, false otherwise).
 *
 * Returns:
 * divider: A divider struct with the specified attributes.
 */
divider make_divider(uint64_t size, bool alloc, bool prev_alloc, bool next_alloc, bool epilogue)
{
    return (divider) {
        .size = size,
        .alloc = alloc,
        .prev_alloc = prev_alloc,
        .next_alloc = next_alloc,
        .epilogue = epilogue
    };
}

/**
 * Calculates the footer address based on the header address.
 *
 * Parameters:
 * header (divider*): Pointer to the header of a memory block.
 *
 * Returns:
 * divider*: Pointer to the footer of the corresponding memory block.
 */
divider *footer_from_header(divider *header)
{
    return (divider *)((uint8_t *)header + header->size - DIVIDER_SIZE);
}

/**
 * Calculates the header address based on the data pointer.
 *
 * Parameters:
 * ptr (void*): Pointer to the data section within a memory block.
 *
 * Returns:
 * divider*: Pointer to the header of the corresponding memory block.
 */
divider *header_from_data(void *ptr)
{
    return (divider *)((uint8_t *)ptr - DIVIDER_SIZE);
}

/**
 * Calculates the data section address based on the header address.
 *
 * Parameters:
 * header (divider*): Pointer to the header of a memory block.
 *
 * Returns:
 * void*: Pointer to the data section within the memory block.
 */
void *data_from_header(divider *header)
{
    return (void *)((uint8_t *)header + DIVIDER_SIZE);
}

/**
 * Calculates the header address of the next block based on the current block's header address.
 *
 * Parameters:
 * header (divider*): Pointer to the header of a memory block.
 *
 * Returns:
 * divider*: Pointer to the header of the next memory block.
 */
divider *header_to_header(divider *header)
{
    return (divider *)((uint8_t *)header + header->size);
}

/**
 * Adds a free block to the doubly linked list of free blocks.
 * 
 * Parameters:
 * header (divider*): Pointer to the header of a free memory block.
 * 
 * Returns:
 * free_block*: Pointer to the first free block in the free list.
 * 
 * Assumptions:
 * - The header is a valid pointer to a free memory block and is not NULL.
*/
free_block* add_to_free_list(divider *header)
{
    free_block *new_free_block = (free_block *)((uint8_t *)header + DIVIDER_SIZE);
    new_free_block->prev_free_block = NULL;
    new_free_block->next_free_block = free_list;
    
    if (free_list != NULL) {
        free_list->prev_free_block = new_free_block;
    }
    free_list = new_free_block;

    return free_list;
}

/**
 * Removes a free block from the doubly linked list of free blocks.
 * 
 * Parameters:
 * header (divider*): Pointer to the header of a free memory block.
 * 
 * Returns:
 * free_block*: Pointer to the first free block in the free list.
 * 
 * Assumptions:
 * - The header is a valid pointer to a free memory block and is not NULL.
*/
free_block* remove_from_free_list(divider *header)
{
    free_block *current_free_block = (free_block *)((uint8_t *)header + DIVIDER_SIZE);
    free_block *prev_free_block = current_free_block->prev_free_block;
    free_block *next_free_block = current_free_block->next_free_block;

    if (prev_free_block != NULL) {
        prev_free_block->next_free_block = next_free_block;
    } else {
        free_list = next_free_block;
    }

    if (next_free_block != NULL) {
        next_free_block->prev_free_block = prev_free_block;
    }

    return free_list;
}


/**
 * Initializes the initial heap structure.
 *
 * Returns:
 * bool: true on successful initialization, false on failure.
 * 
 * Assumptions:
 * - The heap as not been initialized before.
 */
bool mm_init(void)
{
    heap = mm_sbrk(4 * sizeof(uint64_t));

    if (heap == (void *)-1)
    {
        return false;
    }

    // Create prologue header and set attributes with padding
    divider *prologue_header = (divider *)((uint8_t *)heap + DIVIDER_SIZE);
    *prologue_header = make_divider(sizeof(divider) + sizeof(divider), true, true, true, false);

    // Create prologue footer
    divider *prologue_footer = footer_from_header(prologue_header);
    *prologue_footer = make_divider(sizeof(divider) + sizeof(divider), true, true, true, false);

    // Create epilogue header
    divider *epilogue_header = (divider *)((uint8_t *)prologue_footer + DIVIDER_SIZE);
    *epilogue_header = make_divider(0, true, true, true, true);

    free_list = NULL;

    return true;
}

/**
 * Changes the allocation status of a block and updates the header and footer.
 * 
 * Parameters:
 * header (divider*): Pointer to the header of the memory block to change.
 * dummy_divider (divider): The new divider to update the header and footer with.
 * 
 * Returns:
 * divider*: Pointer to the footer of the memory block after the change.
 *
 * Assumptions:
 * - The header is a valid pointer to a memory block and is not NULL.
*/
divider* change_alloc(divider *header, divider dummy_divider)
{
    *header = dummy_divider;
    divider *footer = footer_from_header(header);
    *footer = dummy_divider;

    // Marks the next block's previous allocation status and epilogue flag.
    divider *next_header = (divider *)((uint8_t *)footer + DIVIDER_SIZE);
    *next_header = make_divider(next_header->size, next_header->alloc, header->alloc, next_header->next_alloc, next_header->epilogue);

    // If the next block is not an epilogue, updates its footer with the same information as the header.
    if (!next_header->epilogue)
    {
        divider *next_footer = footer_from_header(next_header);
        *next_footer = *next_header;
    }

    // Marks the previous block's next allocation status and epilogue flag.
    divider *prev_footer = (divider *)((uint8_t *)header - DIVIDER_SIZE);
    *prev_footer = make_divider(prev_footer->size, prev_footer->alloc, prev_footer->prev_alloc, header->alloc, prev_footer->epilogue);

    // Marks the previous block's allocation status.
    divider *prev_header = (divider *)((uint8_t *)prev_footer - prev_footer->size + DIVIDER_SIZE);
    *prev_header = *prev_footer;

    return footer;
}

/**
 * Splits a memory block into two blocks, one of the specified size and the other with the remaining size.
 *
 * Parameters:
 * divider (divider*): Pointer to the header of the memory block to split.
 * size (size_t): The size of the memory block to allocate.
 *
 * Returns:
 * divider*: Pointer to the header of the newly created memory block from the split.
 */
divider* split(divider *header, size_t size)
{
    divider old_header = *header;
    old_header = old_header;
    divider* old_footer = footer_from_header(header);

    // Update the header and create footer with the new size and allocation status
    *header = make_divider(size, true, header->prev_alloc, false, header->epilogue);
    divider *new_footer = footer_from_header(header);
    *new_footer = *header;

    // Calculate the new size of the remaining block
    // TODO: Check why the old_footer->size is not working and is giving wrong size
    // uint64_t remaining_size = old_footer->size - size;
    uint64_t remaining_size = old_header.size - size;

    // Create a new header with the remaining size and update the old footer
    divider *new_header = (divider *)((uint8_t *)new_footer + DIVIDER_SIZE);
    *new_header = make_divider(remaining_size, false, header->alloc, old_header.next_alloc, header->epilogue);
    *old_footer = *new_header;

    change_alloc(header, *header);
    change_alloc(new_header, *new_header);

    // Adds the new block to the free list
    add_to_free_list(new_header);

    return new_header;
}

/**
 * Calculates the header address of the free block from the free block pointer.
 * 
 * Parameters:
 * free_block (free_block*): Pointer to the free block in the free list.
 * 
 * Returns:
 * divider*: Pointer to the header of the free block.
 * 
 * Assumptions:
 * - The free_block is a valid pointer to a free block and is not NULL. 
 */
divider* header_from_free_block(free_block *free_block) {
    return (divider *)((uint8_t *)free_block - DIVIDER_SIZE);
}

/**
 * Finds a free space in the heap with the given size and allocates it if available.
 * Updates the header and footer of the allocated space.
 *
 * Parameters:
 * size (size_t): The size of the memory block requested.
 *
 * Returns:
 * divider*: Pointer to the header of the allocated space, or NULL if no suitable space found.
 */
divider *find_free_space(size_t size)
{
    free_block *current_free_block = free_list;
    free_block *best_fit = NULL;
    double best_fit_margin = .75;

    while (current_free_block != NULL)
    {
        // Calculate the header of the current free block
        divider *current_header = header_from_free_block(current_free_block);
        
        if (in_heap(current_free_block) && !current_header->alloc && current_header->size >= size) {
            // If the current block is free and has sufficient size, update the best fit block
            if (best_fit == NULL || current_header->size < header_from_free_block(best_fit)->size) {
                best_fit = current_free_block;

                // Check if current_free_block is withint the best_fit_margin of the requested size
                if (current_header->size >= size && current_header->size <= size + (size * best_fit_margin)) {
                    break;
                }
            }
        }


        // Move to the next block in the free list
        current_free_block = current_free_block->next_free_block;
    }

    // Check if the current block is free and has sufficient size
    if (best_fit != NULL)
    {
        divider *current_header = header_from_free_block(best_fit);
        if (in_heap(best_fit) && !current_header->alloc && current_header->size >= size) {
            // Split the block if it is larger than the requested size to account for new header size
            if (current_header->size > size + DIVIDER_SIZE + sizeof(free_block)) {
                split(current_header, size);

            // Update header and footer with allocation status
            } else {
                change_alloc(current_header, make_divider(current_header->size, true, current_header->prev_alloc, current_header->next_alloc, current_header->epilogue));
            }

            return current_header;
        }
    }

    return NULL;
}

/**
 * Increases the heap size by size.
 *
 * Parameters:
 * size (size_t): The size to increase the heap by and the size of the new memory block.
 *
 * Returns:
 * divider*: A pointer to the header of the newly created memory block.
 *           Returns NULL if the heap expansion fails.
 *
 * Assumptions:
 * - The size is already aligned and includes the size of header, data, and footer.
 */
divider *increase_heap(size_t size)
{
    void *extended_heap = mm_sbrk(size);

    if (extended_heap == (void *)-1)
    {
        return NULL;
    }

    // Calculate and initalize the header position for the new memory block
    divider *current_header = (divider *)((uint8_t *)extended_heap - DIVIDER_SIZE);
    *current_header = make_divider(size, true, true, true, false);

    // Calculate the initalize the footer position
    divider *current_footer = footer_from_header(current_header);
    *current_footer = make_divider(size, true, true, true, false);

    // Calculate and initalize the epilogue header position
    divider *epilogue_header = (divider *)((uint8_t *)current_footer + DIVIDER_SIZE);
    *epilogue_header = make_divider(0, true, true, true, true);

    change_alloc(current_header, *current_header);

    return current_header;
}

/**
 * Allocates a memory block of the specified size and if no suitable free block is found,
 * it extends the heap and allocates space.
 *
 * Parameters:
 * size (size_t): The size of the memory block to allocate.
 *
 * Returns:
 * void*: A pointer to the allocated memory block, or NULL if allocation fails.
 *
 * Assumptions:
 * - Size is greater than 0.
 * - The heap and free list have been initialized using mm_init.
 */
void *malloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    // Align the size and add space for headers and footers.
    size = align(size) + 2 * DIVIDER_SIZE;

    divider *free_header = find_free_space(size);

    if (free_header == NULL)
    {
        // If no free block is found, extend the heap and allocate space.
        free_header = increase_heap(size);

    } else {
        // Remove the allocated block from the free list
        remove_from_free_list(free_header);
    }

    return data_from_header(free_header);
}


/**
 * Coalesces two blocks into one and updates the header and footer.
 * 
 * Parameters:
 * prev_header (divider*): Pointer to the header of a memory block.
 * next_footer (divider*): Pointer to the footer of a memory block.
 * 
 * Returns:
 * divider*: Pointer to the header of the new coalesced memory block.
 * 
 * Assumptions:
 * - The previous and next blocks are valid pointers to memory blocks and are not NULL.
*/
divider* coalesce(divider *header, divider *footer)
{
    // Calculate the new size of the coalesced block
    uint64_t new_size = (uint8_t*)footer - (uint8_t*)header + DIVIDER_SIZE;

    *header = make_divider(new_size, false, header->prev_alloc, footer->next_alloc, header->epilogue);
    *footer = *header;

    // Update the header and footer with the new size and allocation status
    change_alloc(header, *header);
    return header;
}

/**
 * Frees the memory block ptr and updates allocation status.
 *
 * Parameters:
 * ptr (void*): Pointer to the memory block to be freed. If NULL, no action is taken.
 *
 * Assumptions:
 * - The given pointer is a valid pointer returned by malloc.
 */
void free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    // Marks the memory block as unallocated by updating the header and footer.
    divider *current_header = header_from_data(ptr);
    divider *current_footer = change_alloc(current_header,
                                            make_divider(current_header->size,
                                            false,
                                            current_header->prev_alloc,
                                            current_header->next_alloc,
                                            current_header->epilogue));

    // Calculates the previous and next block's header and footer                                            
    divider *prev_footer = (divider *)((uint8_t *)current_header - DIVIDER_SIZE);
    divider *prev_header = (divider *)((uint8_t *)prev_footer - prev_footer->size + DIVIDER_SIZE);
    divider *next_header = (divider *)((uint8_t *)current_footer + DIVIDER_SIZE);

    // Coalesces the current block with the previous and next blocks if they are both free
    if (!current_header->prev_alloc && !current_header->next_alloc && !next_header->epilogue) {
        divider *next_footer = footer_from_header(next_header);
        remove_from_free_list(prev_header);
        remove_from_free_list(next_header);
        current_header = coalesce(prev_header, next_footer);

    // Coalesces the current block with the previous block if it is free (when the next block is allocated)
    } else if (!current_header->prev_alloc) {
        remove_from_free_list(prev_header);
        current_header = coalesce(prev_header, current_footer);

    // Coalesces the current block with the next block if it is free (when the previous block is allocated)
    } else if (!current_header->next_alloc) {
        current_footer = footer_from_header(next_header);
        remove_from_free_list(next_header);
        current_header = coalesce(current_header, current_footer);
    } 

    // Adds the current block to the free list
    add_to_free_list(current_header);

    return;
}

/**
 * Transfers data from an old memory block to a new memory block.
 *
 * Parameters:
 * old_header (divider*): Pointer to the header of the old memory block.
 * new_header (divider*): Pointer to the header of the new memory block.
 *
 * Returns:
 * bool: True if the data transfer was successful, false otherwise.
 *
 * Assumptions:
 * - The old and new headers are valid pointers to memory blocks and are not NULL.
 */
bool transfer(divider *old_header, divider *new_header)
{
    // Calculate the starting addresses of data in both old and new memory blocks
    void *new_data_start = data_from_header(new_header);
    void *old_data_start = data_from_header(old_header);

    // Determine the data transfer size cutoff
    uint64_t transfer_size = (old_header->size < new_header->size) ? old_header->size - (2 * sizeof(divider)) : new_header->size - (2 * sizeof(divider));

    memcpy(new_data_start, old_data_start, transfer_size);

    return true;
}

/**
 * Resizes the memory block pointed to by oldptr to the specified size.
 *
 * Parameters:
 * oldptr (void*): Pointer to the old memory block.
 * size (size_t): The new size for the memory block.
 *
 * Returns:
 * void*: Pointer to the newly resized memory block. NULL if reallocation fails.
 * 
 * Assumptions:
 * - The oldptr is a valid pointer returned by malloc.
 */
void *realloc(void *oldptr, size_t size)
{
    if (oldptr == NULL)
    {
        return malloc(size);
    }

    if (size == 0)
    {
        free(oldptr);
        return NULL;
    }

    // If the existing block is large enough, return the original pointer
    if (header_from_data(oldptr)->size - (2 * DIVIDER_SIZE) >= size)
    {
        return oldptr;
    }

    divider *new_header = header_from_data(malloc(size));
    bool transferred = transfer(header_from_data(oldptr), new_header);

    if (!transferred)
    {
        return NULL;
    }

    free(oldptr);

    return data_from_header(new_header);
}

/*
 * Note: This function is not tested by mdriver, and has been implemented for you.
 *
 * Parameters:
 * nmemb (size_t): Number of elements in the array.
 * size (size_t): Size of each element in bytes.
 *
 * Returns:
 * void*: A pointer to the allocated memory block
 */
void* calloc(size_t nmemb, size_t size)
{
    void* ptr;
    size *= nmemb;
    ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/*
 * Checks if a given pointer is aligned to 8 bytes.
 * 
 * Parameters:
 * p (const void*): The pointer to be checked.
 * 
 * Returns:
 * bool: True if the pointer is aligned, false otherwise.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/**
 * Compares a divider header with its corresponding footer.
 *
 * Parameters:
 * header (divider*): Pointer to the header of a memory block.
 * footer (divider*): Pointer to the footer of the same memory block.
 *
 * Returns:
 * bool: True if the header and footer match, false otherwise.
 */
bool compare_divider(divider *header, divider *footer)
{
    if (header->size != footer->size ||
        header->alloc != footer->alloc ||
        header->prev_alloc != footer->prev_alloc ||
        header->next_alloc != footer->next_alloc ||
        header->epilogue != footer->epilogue)
    {
        return false;
    }

    return true;
}

/**
 * Traverses the heap, checking invariants between header and footer of each block.
 *
 * Returns:
 * bool: True if invariants hold for all blocks, false otherwise.
 */
bool traverse_heap()
{
    divider *current_header = (divider *)(heap + sizeof(divider));

    while (!current_header->epilogue)
    {
        dbg_printf("Checking block at header: %p\n", current_header);
        if (compare_divider(current_header, (current_header + current_header->size - sizeof(divider))))
        {
            dbg_printf("Header and footer do not match at %p and %p\n", current_header, (current_header + current_header->size - sizeof(divider)));
            return false;
        }

        current_header = (divider *)((uint8_t *)current_header + current_header->size);
    }

    dbg_printf("Epilogue block at %p\n", current_header);

    return true;
}

bool tranverse_free_list()
{
    free_block *current_free_block = free_list;

    while (current_free_block != NULL)
    {
        dbg_printf("Checking free block at %p\n", current_free_block);
        if (!in_heap(current_free_block))
        {
            dbg_printf("Free block not in heap at %p\n", current_free_block);
            return false;
        }

        current_free_block = current_free_block->next_free_block;
    }

    return true;
}

/**
 * You call the function via mm_checkheap(__LINE__)
 * The line number can be used to print the line number of the calling
 * function where there was an invalid heap.
 *
 * Parameters:
 * line_number (int): The line number of the calling function for error reporting.
 *
 * Returns:
 * bool: True if heap invariants hold, false otherwise.
 */
bool mm_checkheap(int line_number)
{
#ifdef DEBUG
    if (!traverse_heap())
    {
        return false;
    }
    // dbg_printf("Heap is consistent at line %d\n", line_number);

    if (!tranverse_free_list())
    {
        return false;
    }
    // dbg_printf("Free list is consistent at line %d\n", line_number);

    /* TODO: Write a real heap checker
    * Is every block in the free list marked as free?
    * Are there any contiguous free blocks that somehow escaped coalescing?
    * Is every free block actually in the free list?
    * Do the pointers in the free list point to valid free blocks?
    * Do any allocated blocks overlap?
    * Do the pointers in a heap block point to valid heap addresses?
    */

#endif // DEBUG

    return true;
}
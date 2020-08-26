# heap_allocators

## Implicit Allocator Performance:
- 73% utilization
- 11,048 instructions/request

## Explicit Allocator Performance:
- 89% utilization
- 597 instructions/request

**performance calculated using a series of 8 comprehensive, course-provided test scripts*
<br />

## Instructions

### Implement An Implicit Free List Allocator

Now you're ready to implement your first heap allocator design. The specific features that your implicit free list allocator must support are:

- Headers that track block information (size, status in-use or free) - you must use the header design mentioned in lecture that is 8 bytes big, using any of the least significant 3 bits to store the status
- Free blocks that are recycled and reused for subsequent malloc requests if possible
- A malloc implementation that searches the heap for free blocks via an implicit list (i.e. traverses block-by-block).

Your implicit free list allocator is not required to:
- implement any coalescing of freed blocks (the explicit allocator will do this)
- support in-place realloc (the explicit allocator will do this); for realloc requests you may satisfy the request by moving the memory to another location
- use a footer, as done in the textbook
- resize the heap when out of memory. If your allocator runs out of memory, it should indicate that to the client.
- This allocator won't be that zippy of a performer but recycling freed nodes should certainly improve utilization over the bump allocator.

The bulleted list above indicates the minimum specification that you are required to implement. Further details are intentionally unspecified as we leave these design decisions up to you; this means it is your choice whether you search using first-fit/next-fit/best-fit/worst-fit, and so on. It can be fun to play around with these decisions to see the various effects. Any reasonable and justified choices will be acceptable for grading purposes.

### Implement An Explicit Free List Allocator

Building on what you learned from writing the implicit version, use the file explicit.c to develop a high-performance allocator that adds an explicit free list as specified in class and in the textbook and includes support for coalescing and in-place realloc.

The specific features that your explicit free list allocator must support:

- Block headers and recycling freed nodes as specified in the implicit implementation (you can copy from your implicit version)
- An explicit free list managed as a doubly-linked list, using the first 16 bytes of each free block's payload for next/prev pointers
- Note that the header should not be enlarged to add fields for the pointers. Since the list only consists of free blocks, the economical approach is to store those pointers in the otherwise unused payload.
- Malloc should search the explicit list of free blocks
- A freed block should be coalesced with its neighbor block to the right if it is also free. Coalescing a pair of blocks must operate in O(1) time. You should perform this coalescing when a block is freed.
- Realloc should resize a block in-place whenever possible, e.g. if the client is resizing to a smaller size, or neighboring block(s) to its right are free and can be absorbed. Even if an in-place realloc is not possible, you should still absorb adjacent free blocks as much as possible until you either can realloc in place, or can no longer absorb and must reallocate elsewhere.

Your explicit free list allocator is not required to (but may optionally):
- coalesce free blocks to the left or otherwise merge blocks to the left
- coalesce more than once for a free block. In other words, if you free a block and there are multiple free blocks directly to its right, you are only required to coalesce once for the first two. However, for realloc you should support in place realloc which may need to absorb multiple blocks to its right.
- resize the heap when out of memory. If your allocator runs out of memory, it should indicate that to the client.

**Full Assignment Specs:** https://web.stanford.edu/class/archive/cs/cs107/cs107.1206/assign7/

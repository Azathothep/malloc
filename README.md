# ft_malloc

**ft_malloc** is a custom implementation of malloc, using mmap(2) and munmap(2) to request and return memory pages.

It provides an efficient allocation process, designed to reduce fragmentation as much as possible while maintaining good performances.

## Getting started

You must have [cmake](https://cmake.org/) installed on your system to build the application.

Clone the repository and build the library using `make`.
You will end up with two files:
- The proper library `libft_malloc_[hardware]_[kernel].so`, where `[hardware]` and `[kernel]` are replaced by the system informations provided by `uname -m` and `uname -s`
- A symbolic link to the library `libft_malloc.so`. Use this one to include in your own applications so it stays system-independent.

For the compiler to know where to look for the library, use `-rpath=[path]` and replace the `[path]` with the directory path containing the `libft_malloc.so`. Don't forget to also add `-lft_malloc`.

## Notes

- the minimum size of an allocation is **16 bits**
- the returned memory pointer is **8-bit** aligned
- malloc(0) returns **NULL**
- freeing an **already freed pointer** doesn't do anything

## Show the memory 

**ft_malloc** also provides access to the `print_mem()` function. It will print out all the memory slots currently mapped, using the following color code:

- green: free slot
- blue: free slot in unsorted bin
- red: allocated slot

## Testing

A test main is available in the `test` folder. Build it using `make -C test/` and run the executable.

The test will request and free a large number of allocations randomly. It can take up to 4 arguments to customize the test:

```
./a.out [Max number of allocations] [Max allocation size] [Loops] [Seed]
```

- 1: the maximum number of slots to allocate at the same time
- 2: the maximum allocation size
- 3: the number of times to run the whole allocation - freeing process
- 4: the seed to use for the randomization.

It will print out some informations you can use to evaluate the performances, such as the average time each loop took to complete.

You can compare the resulting performances with the standard `malloc` implementation by using `make fclean && make std`, which will recompile the application without linking to `ft_malloc`.

## Implementation details

### Headers

Every allocated slot is preceded by 32 bytes of datas used by ft_malloc to store allocation informations:

```c
struct s_header {
	struct s_header	*Prev; // The adress of the previous allocation's header
	struct s_header *Next; // The adress of the next allocation's header
	t_usage			State; // The state of the allocation (in use, in unsorted bin or free)
	size_t			SlotSize; // The size of the slot
};
```

Additionally, when the slot is free, its first 16 bytes are used to keep track of the freed memory:
```c
struct {
   struct s_header *PrevFree;
   struct s_header *NextFree;
};
```

### Zones

Allocations are distributed between three different memory zones:
- the TINY zone for allocation between `16` and `64 bytes`
- the SMALL zone for allocations between `65` and `1024 bytes`
- the LARGE zone for allocations of more than `1024 bytes`

Each zone manages its own memory pages, requested through `mmap` and chained together by using their first 32 bytes.

Requested page sizes are all aligned to the value returned by `getpagesize()`.

For TINY and SMALL zones, each request to `mmap` is able to contain at least 100 slots of the maximum allocation size.

For LARGE zones, an `mmap` request is equal to at least 20 pages.

### Minimum mapped memory

**ft_malloc** keeps, at all time, a minimum amount of mapped memory for each zone. This memory will never be unmapped.

This amount depends on the system's page size but for pages of 4096 bytes, it equals:
- 3 pages for the TINY zone
- 26 pages for the SMALL zone
- 20 pages for the LARGE zone


### Memory coalescion

If no free slot big enough to satisfy the allocation request has been found, ft_malloc first try to **coalesce** all the slots in the target zone.

A coalescion looks at all the free neighbouring slots and merge them into one single continuous slot. This reduces memory fragmentation by recycling small slots into bigger ones.

After this process, if it still doesn't find any big enough slot to fit the request, the program calls `mmap` to get more memory pages from the system.

### Bins

Each zone contains a fix number of **bins** providing a fast access to freed memory.

A bin stores a linked list of headers all sharing the same size.
- The TINY zone contains `8 regular bins` of size going from `16 to 64 bytes`, `8-bytes` spaced (16, 24, 32, ..., 64), stored in FIFO order
- The SMALL zone contains `121 regular bins` going from `72 to 1024 bytes`, `8-bytes` spaced, stored in FIFO order
- The LARGE zone contains `64 regular bins` going from `1032 to 2 796 544 bytes`, irregularly spaced. They are spaced by at least 64 bytes so they can store headers of slightly different size, from biggest to smallest.

LARGE zone bins are not equally spaced, but are divided into bins segments. Each segment contains a different number of bins, but all bin in the same segment are equally spaced.

- The first segment contains `32 bins` (2^5), apart by `64 bytes` (2^(3 + 3 * 1))
- The second one contains `16 bins` (2^4), apart by `512 bytes` (2^(3 + 3 * 2))
- etc...
- Until the last segment which contains only `1 bin` (2^0), apart from the previous segment's last bin by `2 097 152 bytes` (2^(3 + 3 * 6)).

Additionally, each zone contains an additional *dump bin* storing all the other slots that belong to the associated zone, are too big to fit in one of the regular bins and are waiting to be broken in tinier slots.

### Unsorted bin

**ft_malloc** also contains a special global bin: the **unsorted bin**.

When memory slots are freed, they don't actually go in their bin right away. They are first stacked into the unsorted bin.

Slots into the unsorted bin get exactly one chance of beeing re-allocated.

When the program calls malloc, it first check into the unsorted bin for a perfect fit: if it doesn't find one, it 'flushes' the entirety of the unsorted bin into regular bins.

This comes from the known observation that frees are often grouped together, and followed by allocation requests of the same size.

### Unmapping memory

`mmap` and `munmap` are very expensive calls. To save these calls as much as possible, **ft_malloc** follows a tight process and only unmaps memory when multiple conditions are met.

To know if a memory chunk can be unmaped, a coalescion request is necessary. Since a coalescion can be quite costly, we only do it if we know enough memory has been freed since the last coalescion for the process to be worth it.

- After having flushed the unsorted bin content, ft_malloc first check the total size of freed memory in each zone since the last coalescion.
- If it exceeds a certain threshold, it coalesces the zone and verify the size of each chunk's first slot. If the slot size equals the chunk size, it means the whole chunk is free and can be unmapped.
- Then, it finally checks if the total size of all unmappable chunks exceed another predefined threshold. Only then, if this final threshold is exceeded, it unmaps all the relevant memory pages.

This tight process makes sure that unmap only happens if a large enough size of memory is free.

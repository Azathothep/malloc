#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

typedef struct 	s_header {
	struct s_header	*Prev;
	struct s_header *Next;
	size_t		Size;
	size_t		RealSize;

	// Only used when block is freed
	struct s_header *PrevFree;
	struct s_header *NextFree;
}		t_header;

typedef struct	s_memchunks {
	void		*StartingBlockAddr;
	t_header	*FreeList;
}		t_memchunks;

# define TINY_BINS_COUNT	9
# define TINY_BINS_DUMP		(TINY_BINS_COUNT - 1)

typedef struct	s_memlayout {
	t_memchunks	TinyZone;
	t_header	*TinyBins[TINY_BINS_COUNT];	
	
	t_memchunks	SmallZone;
	
	t_memchunks	LargeZone;
}		t_memlayout;

extern	t_memlayout MemoryLayout;

# define PAGE_SIZE		getpagesize()

# define TINY_ALLOC_MAX		64
# define SMALL_ALLOC_MAX	1024 	
# define MIN_ENTRY		100

# define LARGE_PREALLOC		(PAGE_SIZE * 20)

# define CHUNK_ALIGN(c)		(((c) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1)) 	

# define HEADER_SIZE		32
# define MIN_ALLOC		16
# define CHUNK_HEADER     	(sizeof(size_t) + sizeof(void *)) // size of chunk + pointer to previous chunk
# define CHUNK_FOOTER	    	(sizeof(void *) + sizeof(void *)) // last header pointer + pointer to next chunk
# define CHUNK_OVERHEAD		(CHUNK_HEADER + CHUNK_FOOTER)
# define CHUNK_STARTING_ADDR(p) (p + CHUNK_HEADER)

# define MIN_CHUNK_SIZE(s)	((s + HEADER_SIZE) * MIN_ENTRY + CHUNK_OVERHEAD)

# define TINY_CHUNK		CHUNK_ALIGN(MIN_CHUNK_SIZE(TINY_ALLOC_MAX))
# define SMALL_CHUNK		CHUNK_ALIGN(MIN_CHUNK_SIZE(SMALL_ALLOC_MAX))
# define LARGE_CHUNK(s)		CHUNK_ALIGN(s + HEADER_SIZE + CHUNK_OVERHEAD)

# define CHUNK_USABLE_SIZE(s)	(size_t)(s - CHUNK_OVERHEAD)
# define GET_CHUNK_SIZE(p)	*((size_t *)(p))
# define SET_CHUNK_SIZE(p, s)	*((size_t *)p) = s
# define GET_NEXT_CHUNK(p)	(*((void **)(p + GET_CHUNK_SIZE(p) - sizeof(void *)))) // go to the end, then backtrack to previous
# define SET_NEXT_CHUNK(p, n)	(GET_NEXT_CHUNK(p) = n)
# define GET_PREV_CHUNK(p)	(*((void **)(p + sizeof(size_t)))) // go to the chunk's second slot
# define SET_PREV_CHUNK(p, n)	(GET_PREV_CHUNK(p) = n)

# define ALIGNMENT		8
# define SIZE_ALIGN(s)		(((s) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

# define TINY_SPACE_MIN		(ALIGNMENT + HEADER_SIZE)
# define SMALL_SPACE_MIN	(SIZE_ALIGN(TINY_ALLOC_MAX + 1) + HEADER_SIZE)
# define LARGE_SPACE_MIN	(SIZE_ALIGN(SMALL_ALLOC_MAX + 1) + HEADER_SIZE) 

# define GET_HEADER(p)		((t_header *)((void *)p - HEADER_SIZE))	
# define GET_SLOT(p)		((void *)((void *)p + HEADER_SIZE))

# define ALLOC_FLAG		1
# define FLAG(p)		((t_header *)((uint64_t)p | ALLOC_FLAG))
# define UNFLAG(p)		((t_header *)((uint64_t)p & (~ALLOC_FLAG)))
# define IS_FLAGGED(p)		((uint64_t)p & ALLOC_FLAG)

void lst_free_add(t_header **BeginList, t_header *Hdr);
void lst_free_remove(t_header **BeginList, t_header *Hdr);

int get_tiny_bin_index(size_t AlignedSize);
void put_tiny_slot_in_bin(t_header *Hdr);
void remove_tiny_slot_from_bin(t_header *Hdr);

void coalesce_tiny_slots();

void show_tiny_bins();
void scan_memory_integrity();

#endif

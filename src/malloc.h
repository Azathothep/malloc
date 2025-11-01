#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

typedef enum e_zonetype {
	TINY,
	SMALL,
	LARGE
}			t_zonetype;

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
	t_zonetype	ZoneType;
	void		*StartingBlockAddr;
	t_header	*FreeList;
	int			BinsCount;
	t_header	*Bins[];
}		t_memchunks;

# define TINY_ALLOC_MAX		64
# define SMALL_ALLOC_MAX	1024
# define MIN_ENTRY		100

# define LARGE_PREALLOC		(PAGE_SIZE * 20)

# define TINY_BINS_COUNT	9
# define TINY_BINS_DUMP		(TINY_BINS_COUNT - 1)

# define SMALL_BINS_COUNT	121
# define SMALL_BINS_DUMP	(SMALL_BINS_COUNT - 1)

# define LARGE_BINS_COUNT	64
# define LARGE_BINS_DUMP	(LARGE_BINS_COUNT - 1)

# define LARGE_BINS_SEGMENTS_COUNT 6
# define GET_LARGE_BINS_SEGMENTS_SPACE_BETWEEN(n)	(1 << (6 + (n * 3)))
# define LARGE_BINS_SEGMENTS { 3072, 11264, 44032, 175104, 699392, 2796544 }

typedef struct	s_memlayout {
	t_zonetype	TinyZoneType;
	void		*TinyStartingBlockAddr;
	t_header	*TinyFreeList;
	int			TinyBinsCount;
	t_header	*TinyBins[TINY_BINS_COUNT];	
		
	t_zonetype	SmallZoneType;
	void		*SmallStartingBlockAddr;
	t_header	*SmallFreeList;
	int			SmallBinsCount;	
	t_header	*SmallBins[SMALL_BINS_COUNT];

	t_zonetype	LargeZoneType;
	void		*LargeStartingBlockAddr;
	t_header	*LargeFreeList;
	int			LargeBinsCount;
	t_header	*LargeBins[LARGE_BINS_COUNT];
}		t_memlayout;

extern	t_memlayout MemoryLayout;

# define POINTER_SIZE		8
# define TINY_ZONE_SIZE		(sizeof(t_memchunks) + (TINY_BINS_COUNT * POINTER_SIZE))
# define SMALL_ZONE_SIZE	(sizeof(t_memchunks) + (SMALL_BINS_COUNT * POINTER_SIZE))

# define GET_TINY_ZONE()	((t_memchunks*)((void *)&MemoryLayout))
# define GET_SMALL_ZONE()	((t_memchunks*)((void *)&MemoryLayout + TINY_ZONE_SIZE))
# define GET_LARGE_ZONE()	((t_memchunks*)((void *)&MemoryLayout + TINY_ZONE_SIZE + SMALL_ZONE_SIZE))

# define PAGE_SIZE		getpagesize()

# define CHUNK_ALIGN(c)		(((c) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1)) 	

# define HEADER_SIZE		32
# define MIN_ALLOC		16
# define MIN_TINY_ALLOC		MIN_ALLOC
# define MIN_SMALL_ALLOC	(TINY_ALLOC_MAX + ALIGNMENT)
# define MIN_LARGE_ALLOC	(SMALL_ALLOC_MAX + ALIGNMENT)
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

int	get_bin_index(size_t AlignedSize, t_zonetype ZoneType);
void put_slot_in_bin(t_header *Hdr, t_memchunks *Zone);
void remove_slot_from_bin(t_header *Hdr, t_memchunks *Zone);
void coalesce_slots(t_memchunks *Zone);

void show_bins(t_memchunks *Zone);
void scan_memory_integrity();

int *get_large_bin_segments();

#endif

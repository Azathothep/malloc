#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>
# include <stdint.h>

typedef struct 	s_memchunk {
	struct s_memchunk	*Prev;
	struct s_memchunk	*Next;
	size_t	FullSize;
	uint64_t Padding;
}				t_memchunk;

typedef enum	s_usage {
	FREE,
	UNSORTED_FREE,
	INUSE
}				t_usage;

# define PAGE_SIZE			getpagesize()
# define CHUNK_ALIGN(c)		(((c) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1)) 	

# define CHUNK_OVERHEAD		sizeof(t_memchunk)
# define CHUNK_STARTING_ADDR(p) (((void *)p) + CHUNK_OVERHEAD)//(p + CHUNK_HEADER)

# define MIN_ENTRY			100
# define MIN_CHUNK_SIZE(s)	((s + HEADER_SIZE) * MIN_ENTRY + CHUNK_OVERHEAD)
# define CHUNK_USABLE_SIZE(s)	(size_t)(s - CHUNK_OVERHEAD)

# define TINY_CHUNK			CHUNK_ALIGN(MIN_CHUNK_SIZE(TINY_ALLOC_MAX))
# define SMALL_CHUNK		CHUNK_ALIGN(MIN_CHUNK_SIZE(SMALL_ALLOC_MAX))
# define LARGE_CHUNK(s)		CHUNK_ALIGN(s + HEADER_SIZE + CHUNK_OVERHEAD)
# define LARGE_PREALLOC		(PAGE_SIZE * 20)

typedef struct 	s_header {
	struct s_header	*Prev;
	struct s_header *Next;
	t_usage			State;
	size_t			RealSize;

	// Only used when block is freed
	struct s_header *PrevFree;
	struct s_header *NextFree;
}		t_header;

# define HEADER_SIZE		32
# define ALIGNMENT		8
# define SIZE_ALIGN(s)		(((s) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

# define GET_HEADER(p)		((t_header *)((void *)p - HEADER_SIZE))	
# define GET_SLOT(p)		((void *)((void *)p + HEADER_SIZE))

typedef enum e_zonetype {
	TINY,
	SMALL,
	LARGE
}			t_zonetype;

typedef	struct	s_memstatus {
	size_t		TotalMappedMemSize;
	size_t		TotalFreedMemSize;
	size_t		FreedMemSinceLastCoalescion;
}				t_memstatus;

typedef struct	s_memzone {
	t_zonetype	ZoneType;
	t_memchunk	*FirstChunk;
	t_header	*FreeList;
	t_memstatus	MemStatus;
	int			BinsCount;
	t_header	*Bins[];
	}		t_memzone;

# define MIN_ALLOC			16
# define MIN_TINY_ALLOC		MIN_ALLOC
# define MIN_SMALL_ALLOC	(TINY_ALLOC_MAX + ALIGNMENT)
# define MIN_LARGE_ALLOC	(SMALL_ALLOC_MAX + ALIGNMENT)

# define TINY_ALLOC_MAX		64
# define SMALL_ALLOC_MAX	1024

# define TINY_BINS_COUNT	9
# define TINY_BINS_DUMP		(TINY_BINS_COUNT - 1)

# define SMALL_BINS_COUNT	121
# define SMALL_BINS_DUMP	(SMALL_BINS_COUNT - 1)

# define LARGE_BINS_COUNT	64
# define LARGE_BINS_DUMP	(LARGE_BINS_COUNT - 1)

# define LARGE_BINS_SEGMENTS_COUNT 6
# define GET_LARGE_BINS_SEGMENTS_SPACE_BETWEEN(n)	(1 << (6 + (n * 3)))
# define LARGE_BINS_SEGMENTS { 3072, 11264, 44032, 175104, 699392, 2796544 }

# define POINTER_SIZE		sizeof(void*)
# define TINY_ZONE_SIZE		(sizeof(t_memzone) + (TINY_BINS_COUNT * POINTER_SIZE))
# define SMALL_ZONE_SIZE	(sizeof(t_memzone) + (SMALL_BINS_COUNT * POINTER_SIZE))

# define GET_TINY_ZONE()	((t_memzone*)((void *)&MemoryLayout))
# define GET_SMALL_ZONE()	((t_memzone*)((void *)&MemoryLayout + TINY_ZONE_SIZE))
# define GET_LARGE_ZONE()	((t_memzone*)((void *)&MemoryLayout + TINY_ZONE_SIZE + SMALL_ZONE_SIZE))

# define MIN_MEM_BEFORE_UNMAP	(PAGE_SIZE * 50)

# define MIN_MEM_TO_KEEP_TINY					(TINY_CHUNK * 5)
# define MIN_FREED_MEM_BEFORE_COALESCE_TINY		(TINY_CHUNK * 5)
# define MIN_CHUNK_MEM_BEFORE_UNMAP_TINY		(TINY_CHUNK * 5)

# define MIN_MEM_TO_KEEP_SMALL					(SMALL_CHUNK * 5)
# define MIN_FREED_MEM_BEFORE_COALESCE_SMALL	(SMALL_CHUNK * 5)
# define MIN_CHUNK_MEM_BEFORE_UNMAP_SMALL		(SMALL_CHUNK * 5)

# define MIN_MEM_TO_KEEP_LARGE					(LARGE_PREALLOC * 2)
# define MIN_FREED_MEM_BEFORE_COALESCE_LARGE	(LARGE_PREALLOC)
# define MIN_CHUNK_MEM_BEFORE_UNMAP_LARGE		(LARGE_PREALLOC)

typedef struct	s_memlayout {
	t_zonetype	TinyZoneType;
	t_memchunk	*TinyFirstChunk;
	t_header	*TinyFreeList;
	t_memstatus	TinyMemStatus;
	int			TinyBinsCount;
	t_header	*TinyBins[TINY_BINS_COUNT];	
		
	t_zonetype	SmallZoneType;
	t_memchunk	*SmallFirstChunk;
	t_header	*SmallFreeList;
	t_memstatus	SmallMemStatus;
	int			SmallBinsCount;	
	t_header	*SmallBins[SMALL_BINS_COUNT];

	t_zonetype	LargeZoneType;
	t_memchunk	*LargeFirstChunk;
	t_header	*LargeFreeList;
	t_memstatus	LargeMemStatus;
	int			LargeBinsCount;
	t_header	*LargeBins[LARGE_BINS_COUNT];

	t_header	*UnsortedBin;
}		t_memlayout;

extern	t_memlayout MemoryLayout;

void lst_free_add(t_header **BeginList, t_header *Hdr);
void lst_free_remove(t_header **BeginList, t_header *Hdr);

int	get_bin_index(size_t AlignedSize, t_zonetype ZoneType);
void put_slot_in_bin(t_header *Hdr, t_memzone *Zone);
void remove_slot_from_bin(t_header *Hdr, t_memzone *Zone);
void coalesce_slots(t_memzone *Zone);

void scan_memory_integrity();

int *get_large_bin_segments();

void	flush_unsorted_bin();

void	show_bins(t_memzone *Zone);

#endif

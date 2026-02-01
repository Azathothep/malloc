#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>
# include <stdint.h>

// A chunk of memory allocated using mmap
typedef struct 	s_memchunk {
	struct s_memchunk	*Prev;
	struct s_memchunk	*Next;
	size_t	FullSize;
	uint64_t Padding; // Padding to be 16-bit aligned
}				t_memchunk;

# define PAGE_SIZE			getpagesize()
# define CHUNK_ALIGN(c)		(((c) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1)) // Returns a size beeing the minimum multiple of PAGE_SIZE that can hold the provided size

# define CHUNK_OVERHEAD		sizeof(t_memchunk)
# define CHUNK_STARTING_ADDR(p) (((void *)p) + CHUNK_OVERHEAD) // Get the first address that can be used for a slot in the chunk

# define MIN_ENTRY			100 // The minimum of entries a tiny or small chunk must store
# define MIN_CHUNK_SIZE(s)	((s + HEADER_SIZE) * MIN_ENTRY + CHUNK_OVERHEAD) // The minimum chunk a size must have, relative to the MIN_ENTRY
# define CHUNK_USABLE_SIZE(s)	(size_t)(s - CHUNK_OVERHEAD) // The "usable" size of the chunk for memory allocation

# define TINY_CHUNK			CHUNK_ALIGN(MIN_CHUNK_SIZE(TINY_ALLOC_MAX)) // A tiny cunk size
# define SMALL_CHUNK		CHUNK_ALIGN(MIN_CHUNK_SIZE(SMALL_ALLOC_MAX)) // A small chunk size
# define LARGE_CHUNK(s)		CHUNK_ALIGN(s + HEADER_SIZE + CHUNK_OVERHEAD) // A large chunk size, relative to the provided size
# define LARGE_PREALLOC		(PAGE_SIZE * 20) // The size of a large pre-allocated chunk

typedef enum	s_usage {
	FREE,
	UNSORTED_FREE,
	INUSE
}				t_usage;

typedef struct 	s_header {
	struct s_header	*Prev;
	struct s_header *Next;
	t_usage			State;
	size_t			SlotSize;

	// The following datas are only accessed when the slot is free
	struct s_header *PrevFree;
	struct s_header *NextFree;
}		t_header;

# define HEADER_SIZE		32 // The size of the header, not including the free-only datas access
# define ALIGNMENT			8  // The size alignment
# define SIZE_ALIGN(s)		(((s) + (ALIGNMENT-1)) & ~(ALIGNMENT-1)) // Align the provided size

# define GET_HEADER(p)		((t_header *)((void *)p - HEADER_SIZE))	// Get the t_header address given the usable address
# define GET_SLOT(p)		((void *)((void *)p + HEADER_SIZE)) // Get the usable address given the header one

typedef enum e_zonetype {
	TINY,
	SMALL,
	LARGE
}			t_zonetype;

typedef	struct	s_memstatus {
	size_t		TotalMappedMemSize; // The total memory mapped for this zone (equals to the total size of this zone's chunk)
	size_t		TotalFreedMemSize; // The total free memory in the zone
	size_t		FreedMemSinceLastCoalescion; // The total freed memory size since the last time it has been coalesced
}				t_memstatus;

typedef struct	s_memzone {
	t_zonetype	ZoneType;
	t_memchunk	*FirstChunk;
	t_memstatus	MemStatus;
	int			BinsCount;
	t_header	*Bins[];
	}		t_memzone;

# define MIN_ALLOC			16 // The minimum allocation size

# define MIN_TINY_ALLOC		MIN_ALLOC // The minimum allocation size in the TINY zone
# define TINY_ALLOC_MAX		64 // The maximum allocation size in the TINY zone
# define MIN_SMALL_ALLOC	(TINY_ALLOC_MAX + ALIGNMENT) // The minimum allocation size in the SMALL zone
# define SMALL_ALLOC_MAX	1024 // The maximum allocation size in the SMALL zone
# define MIN_LARGE_ALLOC	(SMALL_ALLOC_MAX + ALIGNMENT) // The minimum allocation size in the LARGE zone

# define TINY_BINS_COUNT	9 // The number of bins in the TINY zone
# define TINY_BINS_DUMP		(TINY_BINS_COUNT - 1) // The index of the dump bin in the TINY zone

# define SMALL_BINS_COUNT	121 // The number of bins in the SMALL zone
# define SMALL_BINS_DUMP	(SMALL_BINS_COUNT - 1) // The index of the dump bin in the SMALL zone

# define LARGE_BINS_COUNT	64 // The number of bins in the LARGE zone
# define LARGE_BINS_DUMP	(LARGE_BINS_COUNT - 1) // The index of the dump bin in the LARGE zone

# define LARGE_BINS_SEGMENTS_COUNT 6 // The number of large bins segments (all bins in the same segment are spaced out equally)
# define GET_LARGE_BINS_SEGMENTS_SPACE_BETWEEN(n)	(1 << (6 + (n * 3))) // The number of bytes between each bin of the provided segment
# define LARGE_BINS_SEGMENTS { 3072, 11264, 44032, 175104, 699392, 2796544 } // The max size of each bin segments (precalculated)

# define POINTER_SIZE		sizeof(void*)
# define TINY_ZONE_SIZE		(sizeof(t_memzone) + (TINY_BINS_COUNT * POINTER_SIZE))
# define SMALL_ZONE_SIZE	(sizeof(t_memzone) + (SMALL_BINS_COUNT * POINTER_SIZE))

# define GET_TINY_ZONE()	((t_memzone*)((void *)&MemoryLayout))
# define GET_SMALL_ZONE()	((t_memzone*)((void *)&MemoryLayout + TINY_ZONE_SIZE))
# define GET_LARGE_ZONE()	((t_memzone*)((void *)&MemoryLayout + TINY_ZONE_SIZE + SMALL_ZONE_SIZE))

# define MIN_MEM_BEFORE_UNMAP	(PAGE_SIZE * 50) // The minimum amount of memory to keep mapped while the application is active

# define MIN_MEM_TO_KEEP_TINY					(TINY_CHUNK * 5) // The minimum memory to keep mapped in the TINY zone
# define MIN_FREED_MEM_BEFORE_COALESCE_TINY		(TINY_CHUNK * 2) // The minimum memory to free before attempting to coalesce in the TINY zone
# define MIN_CHUNK_MEM_BEFORE_UNMAP_TINY		(TINY_CHUNK * 3) // The minimum amount of unmappable memory to reach before unmapping

# define MIN_MEM_TO_KEEP_SMALL					(SMALL_CHUNK * 5)
# define MIN_FREED_MEM_BEFORE_COALESCE_SMALL	(SMALL_CHUNK * 2)
# define MIN_CHUNK_MEM_BEFORE_UNMAP_SMALL		(SMALL_CHUNK * 3)

# define MIN_MEM_TO_KEEP_LARGE					(LARGE_PREALLOC)
# define MIN_FREED_MEM_BEFORE_COALESCE_LARGE	(LARGE_PREALLOC)
# define MIN_CHUNK_MEM_BEFORE_UNMAP_LARGE		(LARGE_PREALLOC)

// The global struct holding all the base datas for malloc & free
typedef struct	s_memlayout {
	t_zonetype	TinyZoneType;
	t_memchunk	*TinyFirstChunk;
	t_memstatus	TinyMemStatus;
	int			TinyBinsCount;
	t_header	*TinyBins[TINY_BINS_COUNT];	
		
	t_zonetype	SmallZoneType;
	t_memchunk	*SmallFirstChunk;
	t_memstatus	SmallMemStatus;
	int			SmallBinsCount;	
	t_header	*SmallBins[SMALL_BINS_COUNT];

	t_zonetype	LargeZoneType;
	t_memchunk	*LargeFirstChunk;
	t_memstatus	LargeMemStatus;
	int			LargeBinsCount;
	t_header	*LargeBins[LARGE_BINS_COUNT];

	t_header	*UnsortedBin;
}		t_memlayout;

extern	t_memlayout MemoryLayout;

int	get_bin_index(size_t AlignedSize, t_zonetype ZoneType);
void put_slot_in_bin(t_header *Hdr, t_memzone *Zone);
void remove_slot_from_bin(t_header *Hdr, t_memzone *Zone);
void coalesce_slots(t_memzone *Zone);

void scan_memory_integrity();

int *get_large_bin_segments();

void	flush_unsorted_bin();

#endif

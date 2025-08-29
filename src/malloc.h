#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

typedef struct 	s_header {
	struct s_header	*Prev;
	struct s_header *Next;
	size_t	Size;
}		t_header;

typedef struct	s_free {
	size_t	Size;
	struct s_free	*Prev;
	struct s_free	*Next;
}		t_free;

typedef struct	s_memchunks {
	void	*StartingBlockAddr;
	t_free	*FreeList;
}		t_memchunks;

typedef struct	s_memlayout {
	t_memchunks	TinyZone;
	t_memchunks	SmallZone;
	t_memchunks	LargeZone;	
}		t_memlayout;

extern	t_memlayout MemoryLayout;

# define TINY_ALLOC	sizeof(t_free) 	// + overhead = + 16 = 40, * 100 = 4000
# define SMALL_ALLOC	256 		// + overhead = + 16 = 272, * 100 = 27200 
# define MIN_ENTRY	100

# define PAGE_SIZE		getpagesize()
# define CHUNK_ALIGN(c)		(((c) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1)) 	

# define HEADER_SIZE		sizeof(t_header)
# define CHUNK_NEXT_SLOT	sizeof(void *)
# define CHUNK_SIZE_SLOT	sizeof(size_t)
# define CHUNK_OVERHEAD		(CHUNK_NEXT_SLOT + CHUNK_SIZE_SLOT)
# define CHUNK_STARTING_ADDR(p) (p + CHUNK_SIZE_SLOT)

# define MIN_CHUNK_SIZE(s)	((s + HEADER_SIZE) * MIN_ENTRY + CHUNK_OVERHEAD)

# define TINY_CHUNK		CHUNK_ALIGN(MIN_CHUNK_SIZE(TINY_ALLOC))
# define SMALL_CHUNK		CHUNK_ALIGN(MIN_CHUNK_SIZE(SMALL_ALLOC))
# define LARGE_CHUNK(s)		CHUNK_ALIGN(s + HEADER_SIZE + CHUNK_OVERHEAD)
# define INTERN_CHUNK		CHUNK_ALIGN(sizeof(t_free) * 100 + CHUNK_OVERHEAD)

# define CHUNK_USABLE_SIZE(s)	(size_t)(s - CHUNK_OVERHEAD)
# define GET_CHUNK_SIZE(p)	*((size_t *)(p))
# define SET_CHUNK_SIZE(p, s)	*((size_t *)p) = s
# define GET_NEXT_CHUNK(p)	*((void **)(p + GET_CHUNK_SIZE(p) - CHUNK_NEXT_SLOT))
# define SET_NEXT_CHUNK(p, n)	(GET_NEXT_CHUNK(p) = n)

# define ALIGNMENT		8
# define SIZE_ALIGN(s)		(((s) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

# define TINY_SPACE_MIN		(ALIGNMENT + HEADER_SIZE)
# define SMALL_SPACE_MIN	(SIZE_ALIGN(TINY_ALLOC + 1) + HEADER_SIZE)
# define LARGE_SPACE_MIN	(SIZE_ALIGN(SMALL_ALLOC + 1) + HEADER_SIZE) 

# define GET_HEADER(p)		(t_header *)((void *)p - HEADER_SIZE)	
# define GET_SLOT(p)		(void *)(p + HEADER_SIZE)

# define SLOT_USABLE_SIZE(p) 	(void *)((GET_HEADER(p))->Next) - p
# define SLOT_FULL_SIZE(p)  	(void *)((GET_HEADER(p))->Next) - (void *)GET_HEADER(p)

void	*map_memory(int size);

#endif

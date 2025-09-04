#ifndef MALLOC_H
# define MALLOC_H

# include <unistd.h>

typedef struct 	s_header {
	struct s_header	*Prev;
	struct s_header *Next;
	size_t	Size;
}		t_header;

typedef struct	s_free {
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

# define TINY_ALLOC	64	//sizeof(t_free) 	// + overhead = + 16 = 40, * 100 = 4000
# define SMALL_ALLOC	1024 	//128 		// + overhead = + 16 = 272, * 100 = 27200 
# define MIN_ENTRY	100

# define PAGE_SIZE		getpagesize()
# define CHUNK_ALIGN(c)		(((c) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1)) 	

# define HEADER_SIZE		sizeof(t_header)
# define CHUNK_HEADER     	(sizeof(size_t) + sizeof(void *) + sizeof(void *)) // size of chunk + pointer to first allocatable block
# define CHUNK_FOOTER	    	(sizeof(void *) + sizeof(void *)) // last header pointer + pointer to next chunk
# define CHUNK_OVERHEAD		(CHUNK_HEADER + CHUNK_FOOTER)
# define CHUNK_STARTING_ADDR(p) (p + CHUNK_HEADER)
# define CHUNK_SET_POINTER_TO_FIRST_ALLOC(c, p) (*((void **)c + sizeof(size_t)) = p)
# define CHUNK_GET_POINTER_TO_FIRST_ALLOC(c) ((t_header *)(*((void **)c + sizeof(size_t))))

# define MIN_CHUNK_SIZE(s)	((s + HEADER_SIZE) * MIN_ENTRY + CHUNK_OVERHEAD)

# define TINY_CHUNK		CHUNK_ALIGN(MIN_CHUNK_SIZE(TINY_ALLOC))
# define SMALL_CHUNK		CHUNK_ALIGN(MIN_CHUNK_SIZE(SMALL_ALLOC))
# define LARGE_CHUNK(s)		CHUNK_ALIGN(s + HEADER_SIZE + CHUNK_OVERHEAD)
# define INTERN_CHUNK		CHUNK_ALIGN(sizeof(t_free) * 100 + CHUNK_OVERHEAD)

# define CHUNK_USABLE_SIZE(s)	(size_t)(s - CHUNK_OVERHEAD)
# define GET_CHUNK_SIZE(p)	*((size_t *)(p))
# define SET_CHUNK_SIZE(p, s)	*((size_t *)p) = s
# define GET_NEXT_CHUNK(p)	(*((void **)(p + GET_CHUNK_SIZE(p) - sizeof(void *)))) // go to the end, then backtrack to previous
# define SET_NEXT_CHUNK(p, n)	(GET_NEXT_CHUNK(p) = n)
# define GET_PREV_CHUNK(p)	(*((void **)(p + sizeof(size_t) + sizeof(void *)))) // go to the chunk's third slot
# define SET_PREV_CHUNK(p, n)	(GET_PREV_CHUNK(p) = n)

# define ALIGNMENT		16
# define SIZE_ALIGN(s)		(((s) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

# define TINY_SPACE_MIN		(sizeof(t_free) + HEADER_SIZE)
# define SMALL_SPACE_MIN	(SIZE_ALIGN(TINY_ALLOC + 1) + HEADER_SIZE)
# define LARGE_SPACE_MIN	(SIZE_ALIGN(SMALL_ALLOC + 1) + HEADER_SIZE) 

# define GET_HEADER(p)		((t_header *)((void *)p - HEADER_SIZE))	
# define GET_SLOT(p)		  ((t_free *)((void *)p + HEADER_SIZE))

# define SLOT_USABLE_SIZE(p) 	(((GET_HEADER(p))->Size) - HEADER_SIZE)
# define SLOT_FULL_SIZE(p)  	((GET_HEADER(p))->Size) 

# define GET_FREE_SIZE(p)	(GET_HEADER(p)->Size)
# define SET_FREE_SIZE(p, s)	GET_HEADER(p)->Size = s

# define ALLOC_FLAG		1
# define FLAG(p)		((t_header *)((uint64_t)p | ALLOC_FLAG))
# define UNFLAG(p)		((t_header *)((uint64_t)p & (~ALLOC_FLAG)))
# define IS_FLAGGED(p)		((uint64_t)p & ALLOC_FLAG)

# define IS_LAST_HDR(Hdr)	(*((void **)Hdr) == NULL)
# define GET_LAST_HDR(Chunk)	(((void *)Chunk + GET_CHUNK_SIZE(Chunk)) - CHUNK_FOOTER)

void	*map_memory(int size);

#endif

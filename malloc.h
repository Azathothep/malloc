#ifndef MALLOC_H
# define MALLOC_H

# include "libft.h"

//# include <stdlib.h>
# include <unistd.h>

void	*malloc(size_t size);

typedef struct 	s_header {
	size_t			p_size; //previous size
	size_t			size; //current size
	struct s_header	*prev;
	struct s_header *next;
} 		t_header;

# define ALIGNMENT 16
# define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
# define CHUNK_SIZE getpagesize()
# define CHUNK_ALIGN(size) (((size)+(CHUNK_SIZE-1)) & ~(CHUNK_SIZE-1))

# define OVERHEAD (sizeof(t_header))
# define HDRP(bp) ((t_header *)((char *)bp - OVERHEAD))
# define GET_SIZE(bp) (HDRP(bp)->size)
# define NEXTP(bp) (HDRP(bp)->next + OVERHEAD)
# define PREVP(bp) (HDRP(bp)->prev + OVERHEAD)
# define BP(hdrp)	(char *)((char *)hdrp + OVERHEAD)
# define BLOCK_SIZE(hdrp) (hdrp->size + OVERHEAD)

# define TINY 128
# define SMALL 1280
# define MIN_ENTRY 100

# define TINY_SPACE CHUNK_ALIGN((TINY + OVERHEAD) * MIN_ENTRY + OVERHEAD)
# define SMALL_SPACE CHUNK_ALIGN((SMALL + OVERHEAD) * MIN_ENTRY + OVERHEAD)

# define TINY_ZONE freep
# define SMALL_ZONE (t_header *)((char *)freep + TINY_SPACE)
# define LARGE_ZONE (t_header *)((char *)freep + TINY_SPACE + SMALL_SPACE)

extern t_header *freep;

#endif

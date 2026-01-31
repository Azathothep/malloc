#include "malloc.h"
#include "utils.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_BLUE		"\x1b[34m"
#define ANSI_COLOR_RESET	"\x1b[0m"

void	print_block(t_header *Hdr) {
	char *color = NULL;
	
	if (Hdr->State == FREE)
		color = ANSI_COLOR_GREEN;
	else if (Hdr->State == UNSORTED_FREE)
		color = ANSI_COLOR_BLUE;
	else
		color = ANSI_COLOR_RED;
	
	PRINT(color); PRINT_ADDR(Hdr); PRINT(": ");

	if (Hdr->State == INUSE) {
		PRINT_UINT64(Hdr->RealSize - HEADER_SIZE);
		PRINT(" ");
	}
	
	PRINT("["); PRINT_UINT64(Hdr->RealSize); PRINT("] ");
	PRINT(ANSI_COLOR_RESET); NL();
}

void	show_alloc_zone(t_memzone *Zone) {
	t_memchunk *Chunk = Zone->FirstChunk;

	while (Chunk != NULL) {
		PRINT("CHUNK: "); PRINT_ADDR(Chunk); NL();	

		t_header *Hdr = CHUNK_STARTING_ADDR(Chunk);
	
		print_block(Hdr);
		
		Hdr = Hdr->Next;

		while (Hdr != NULL) {
			print_block(Hdr);
			t_header *Prev = Hdr;
			Hdr = Hdr->Next;
			if (Hdr != NULL && (uint64_t)Hdr < (uint64_t)Prev) {
				PRINT("CIRCULAR HEADER !!!\n");
				break;
			}
		}
		
		Chunk = Chunk->Next;
	}
}

void	show_alloc_mem() {
	PRINT("\n---- full mem ----\n");

	t_memzone *Zone = GET_TINY_ZONE();
	PRINT("TINY ZONE\n");
	show_alloc_zone(Zone);
	NL();

	Zone = GET_SMALL_ZONE();
	PRINT("SMALL ZONE\n");
	show_alloc_zone(Zone);
	NL();

	Zone = GET_LARGE_ZONE();
	PRINT("LARGE ZONE\n");
	show_alloc_zone(Zone);
	NL();

	PRINT("------------------\n");
}

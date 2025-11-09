#include "malloc.h"
#include "utils.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

//TODO(felix): add chunk subdivision in zones

void	print_block(t_header *Hdr) {
	char *color = NULL;
	
	if (Hdr->Free)
		color = ANSI_COLOR_GREEN;
	else
		color = ANSI_COLOR_RED;
	
	PRINT(color); PRINT_ADDR(Hdr); PRINT(": ");

	if (!Hdr->Free) {
		PRINT_UINT64(Hdr->RealSize - HEADER_SIZE);
		PRINT(" ");
	}
	
	PRINT("["); PRINT_UINT64(Hdr->RealSize); PRINT("] ");
	PRINT(ANSI_COLOR_RESET); NL();
}

void	show_alloc_zone(t_memzone *Zone) {
	void *Chunk = Zone->StartingBlockAddr;

	while (Chunk != NULL) {
		PRINT("CHUNK: "); PRINT_ADDR(Chunk); NL();	

		t_header *Hdr = CHUNK_STARTING_ADDR(Chunk);//CHUNK_GET_POINTER_TO_FIRST_ALLOC(Chunk);
	
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
		
		Chunk = GET_NEXT_CHUNK(Chunk);
	}
}

void	show_alloc_mem() {
	PRINT("\n---- full mem ----\n");

	t_memzone *Zone = GET_TINY_ZONE();//&MemoryLayout.TinyZone;
	PRINT("TINY ZONE\n"); // : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_alloc_zone(Zone);
	NL();

	Zone = GET_SMALL_ZONE(); //&MemoryLayout.SmallZone;
	PRINT("SMALL ZONE\n"); // : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_alloc_zone(Zone);
	NL();

	Zone = GET_LARGE_ZONE(); //&MemoryLayout.LargeZone;
	PRINT("LARGE ZONE\n"); // : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_alloc_zone(Zone);
	NL();

	PRINT("------------------\n");
}

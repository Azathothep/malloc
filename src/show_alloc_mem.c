#include "malloc.h"
#include "utils.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

//TODO(felix): add chunk subdivision in zones

void	print_block(t_header *Hdr) {
	char *color = NULL;
	char flagged = IS_FLAGGED(Hdr);
	
	if (flagged == 1)
		color = ANSI_COLOR_RED;
	else
		color = ANSI_COLOR_GREEN;
	
	Hdr = UNFLAG(Hdr);

	PRINT(color); PRINT_ADDR(Hdr); PRINT(": ");

	if (flagged == 1) {
		PRINT_UINT64(Hdr->Size);
		PRINT(" ");
	}
	
	PRINT("["); PRINT_UINT64(Hdr->RealSize); PRINT("] ");
	//PRINT(", Prev: "); PRINT_ADDR(Hdr->Prev);
	//PRINT(", Next:"); PRINT_ADDR(Hdr->Next);
	PRINT(ANSI_COLOR_RESET); NL();
}

void	show_alloc_zone(t_memchunks *Zone) {
	void *Chunk = Zone->StartingBlockAddr;

	while (Chunk != NULL) {
		PRINT("CHUNK: "); PRINT_ADDR(Chunk); NL();	

		t_header *Hdr = CHUNK_STARTING_ADDR(Chunk);//CHUNK_GET_POINTER_TO_FIRST_ALLOC(Chunk);
	
		print_block(Hdr);
		
		Hdr = UNFLAG(Hdr);
		Hdr = Hdr->Next;

		t_header *HdrClean = UNFLAG(Hdr);
		while (HdrClean != NULL) {
			print_block(Hdr);
			t_header *Prev = HdrClean;
			Hdr = HdrClean->Next;
			HdrClean = UNFLAG(Hdr);
			if (HdrClean != NULL && (uint64_t)HdrClean < (uint64_t)Prev) {
				PRINT("CIRCULAR HEADER !!!\n");
				break;
			}
		}
		
		Chunk = GET_NEXT_CHUNK(Chunk);
	}
}

void	show_alloc_mem() {
	PRINT("\n---- full mem ----\n");

	t_memchunks *Zone = GET_TINY_ZONE();//&MemoryLayout.TinyZone;
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

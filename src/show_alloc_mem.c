#include "malloc.h"
#include "lst_free.h"
#include "utils.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

//TODO(felix): add chunk subdivision in zones

void	show_alloc_zone(t_memchunks *Zone) {
	void *Chunk = Zone->StartingBlockAddr;

	while (Chunk) {
		t_header *Hdr = CHUNK_GET_POINTER_TO_FIRST_ALLOC(Chunk);	

		while (UNFLAG(Hdr) != NULL) {
			char *color = NULL;
			if (IS_FLAGGED(Hdr) == 1)
				color = ANSI_COLOR_RED;
			else
				color = ANSI_COLOR_GREEN;
			Hdr = UNFLAG(Hdr);
			PRINT(color); PRINT_ADDR(Hdr); PRINT(": "); PRINT_UINT64(Hdr->Size); PRINT(" bytes"); PRINT(ANSI_COLOR_RESET); NL();
			Hdr = Hdr->Next;
		}
		
		Chunk = GET_NEXT_CHUNK(Chunk);
	}
}

void	show_alloc_mem() {
	PRINT("\n---- full mem ----\n");

	t_memchunks *Zone = &MemoryLayout.TinyZone;
	PRINT("TINY ZONE : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_alloc_zone(Zone);
	NL();

	Zone = &MemoryLayout.SmallZone;
	PRINT("SMALL ZONE : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_alloc_zone(Zone);
	NL();

	Zone = &MemoryLayout.LargeZone;
	PRINT("LARGE ZONE : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_alloc_zone(Zone);
	NL();

	PRINT("------------------\n");
}

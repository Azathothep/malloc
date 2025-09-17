#include "malloc.h"
#include "utils.h"
#include <stdlib.h>
#include "../ft_malloc.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

#define LINE_SIZE	2

//TODO(felix): add chunk subdivision in zones

void	scan_hexdump_single(void *P) {
	void *mem = *((void **)P);
	PRINT_ADDR(mem);
}

void	scan_hexdump(t_header *Hdr) {
	if (Hdr == NULL) {
		PRINT("["); PRINT_ADDR(Hdr); PRINT("]: ");
		return;
	}

	size_t byteSize = Hdr->RealSize;
	if (byteSize > 90)
		byteSize = 90;

	int slotSize = byteSize / 8;
	int lines = slotSize / LINE_SIZE;
	void *P = (void *)Hdr;

	int i = 0;
	while (i < lines) {
		PRINT("["); PRINT_ADDR(P); PRINT("]: ");
		int j = 0;
		while (j < LINE_SIZE) {
			scan_hexdump_single(P);	
			P += 8;
			PRINT("  ");
			j++;
		}
	
		NL();
		i++;
	}

	int remaining = slotSize - (lines * LINE_SIZE);
	if (remaining > 0) {
		PRINT("["); PRINT_ADDR(P); PRINT("]: ");
	}

	i = 0;
	while (i < remaining) {
		scan_hexdump_single(P);
		PRINT(" ");
		P += 8;
		i++;
	}

	if (byteSize > 90)
		PRINT("...");

	NL();
}

void	scan_error(t_header *Hdr, t_header *Prev, char *errmsg) {

	//show_alloc_mem();

	PRINT(ANSI_COLOR_RED); PRINT("["); PRINT_ADDR(Hdr); PRINT("]: CORRUPTED MEMORY - "); PRINT(errmsg); PRINT(ANSI_COLOR_RESET); NL();

	PRINT("\n----------------- HEXDUMP -----------------\n");
	PRINT("\nPrevious header\n");
	scan_hexdump(Prev);
	PRINT("\nCorrupted header\n");
	scan_hexdump(Hdr);
	PRINT("\nNext header\n");
	scan_hexdump(UNFLAG(Hdr->Next));

//	show_tiny_bins();
	exit(1);
}

void	scan_zone_integrity(t_memchunks *Zone) {
	void *Chunk = Zone->StartingBlockAddr;

	while (Chunk != NULL) {
		t_header *Hdr = CHUNK_STARTING_ADDR(Chunk);
	
		// FIRST BLOCK CHECK
		t_header *Prev = NULL;
		while (UNFLAG(Hdr) != NULL) {
			uint64_t flagged = IS_FLAGGED(Hdr);
			Hdr = UNFLAG(Hdr);

			void *AlignedHdr = (void *)SIZE_ALIGN((uint64_t)Hdr);
			if (Hdr != AlignedHdr) {
				scan_error(Hdr, Prev, "UNALIGNED HEADER");
				return;
			}

			if (flagged == 1 && Hdr->Size > Hdr->RealSize) {
				scan_error(Hdr, Prev, "INCONSISTENT SIZE");
				return;
			}

			t_header *Next = UNFLAG(Hdr->Next);
			if (Next != NULL) {
				if (UNFLAG(Next->Prev) != Hdr) {
					scan_error(Hdr, Prev, "INCONSISTENT HEADERS");
					return;
				}
				
				if (Prev != NULL && flagged != IS_FLAGGED(Next->Prev)) {
					scan_error(Hdr, Prev, "INCONSISTENT FLAGS");
					return;
				}
			}			

			t_header *NextCalculated = (t_header *)((void *)Hdr + Hdr->RealSize);
			if (Next != NULL && NextCalculated != Next) {
				scan_error(Hdr, Prev, "HOLE OR OVERLAPPING SLOT");
				return;
			}

			if (Next != NULL && (uint64_t)Next < (uint64_t)Hdr) {
				scan_error(Hdr, Prev, "CIRCULAR HEADER");
				return;
			}

			Prev = Hdr;
			Hdr = Hdr->Next;
		}
		
		Chunk = GET_NEXT_CHUNK(Chunk);
	}
}

void	scan_memory_integrity() {
	scan_zone_integrity(&MemoryLayout.TinyZone);
	scan_zone_integrity(&MemoryLayout.SmallZone);
	scan_zone_integrity(&MemoryLayout.LargeZone);
}

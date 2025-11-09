#include "malloc.h"
#include "utils.h"
#include <stdlib.h>
#include "../ft_malloc.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

#define LINE_SIZE	2

//TODO(felix): add chunk subdivision in zones

void	show_bins(t_memzone *Zone) {
	PRINT("BINS\n");

	t_header **Bins = Zone->Bins;

	int i = 0;
	while (i < Zone->BinsCount) {
		PRINT("["); PRINT_UINT64(i); PRINT("] (");
		if (i < 8)
			PRINT_UINT64((i + 1) * 8);
		else
			PRINT("+");

		PRINT(")"); NL();
		
		t_header *Ptr = Bins[i];
		while (Ptr != NULL) {
			PRINT_ADDR(Ptr); NL();
			Ptr = Ptr->NextFree;
		}
		NL();
		
		i++;
	}
}

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
	scan_hexdump(Hdr->Next);

	exit(1);
}

void	bin_error(t_header *Hdr, char *errmsg, t_memzone *Zone) {
	PRINT(ANSI_COLOR_RED); PRINT("["); PRINT_ADDR(Hdr); PRINT("]: CORRUPTED BINS - "); PRINT(errmsg); PRINT(ANSI_COLOR_RESET); NL();

	PRINT("\n----------------- HEXDUMP -----------------\n");
	PRINT("\nPrev free header\n");
	scan_hexdump(Hdr->PrevFree);
	PRINT("\nCorrupted header\n");
	scan_hexdump(Hdr);
	PRINT("\nNext free header\n");
	scan_hexdump(Hdr->NextFree);

	show_bins(Zone);

	exit(1);
}

void	search_for_double(t_memzone *Zone, t_header *to_check, int CurrentBin) {
	t_header **Bins = Zone->Bins;
	t_header *NextFree = to_check->NextFree;
	while (CurrentBin < Zone->BinsCount) {
		while (NextFree != NULL) {
			if (to_check == NextFree) {
				bin_error(to_check, "Header found twice !", Zone);
			}

			NextFree = NextFree->NextFree;
		}

		CurrentBin++;
		if (CurrentBin < Zone->BinsCount)
			NextFree = Bins[CurrentBin];
	}
}

void	scan_free_integrity(t_memzone *Zone) {
	int currentBin = 0;
	t_header **Bins = Zone->Bins;
	
	while (currentBin < Zone->BinsCount) {
		t_header *Hdr = Bins[currentBin];
		while (Hdr != NULL) {
			if (Hdr->PrevFree != NULL && Hdr->PrevFree->NextFree != Hdr)
				bin_error(Hdr, "Inconsistent free headers", Zone);

			if (Hdr->NextFree != NULL && Hdr->NextFree->PrevFree != Hdr)
				bin_error(Hdr, "Inconsistent free headers", Zone);

			search_for_double(Zone, Hdr, currentBin);
			Hdr = Hdr->NextFree;
		}
		currentBin++;
	}
}

void	scan_zone_integrity(t_memzone *Zone) {
	void *Chunk = Zone->StartingBlockAddr;

	while (Chunk != NULL) {
		t_header *Hdr = CHUNK_STARTING_ADDR(Chunk);
	
		// FIRST BLOCK CHECK
		t_header *Prev = NULL;
		while (Hdr != NULL) {

			void *AlignedHdr = (void *)SIZE_ALIGN((uint64_t)Hdr);
			if (Hdr != AlignedHdr) {
				scan_error(Hdr, Prev, "UNALIGNED HEADER");
				return;
			}

			t_header *Next = Hdr->Next;
			if (Next != NULL) {
				if (Next->Prev != Hdr) {
					scan_error(Hdr, Prev, "INCONSISTENT HEADERS");
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
	t_memzone *Zone = GET_TINY_ZONE();
	scan_zone_integrity(Zone);
	scan_free_integrity(Zone);

	Zone = GET_SMALL_ZONE();
	scan_zone_integrity(Zone);
	scan_free_integrity(Zone);	

	Zone = GET_LARGE_ZONE();
	scan_zone_integrity(Zone);
	scan_free_integrity(Zone);
}

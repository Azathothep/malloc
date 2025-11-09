#include <sys/mman.h>
#include <errno.h>
#include "malloc.h"
#include "utils.h"
#include "../ft_malloc.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

//#define PRINT_FREE
#define SCAN_MEMORY_FREE

// --------- BINS ---------- //

int get_large_bin_index(size_t AlignedSize) {
	int segments[LARGE_BINS_SEGMENTS_COUNT] = LARGE_BINS_SEGMENTS;
	//size_t FullSize = AlignedSize + HEADER_SIZE;

	// GET SEGMENT INDEX
	int segment_index = 0;
	while (segment_index < LARGE_BINS_SEGMENTS_COUNT && (int)AlignedSize > segments[segment_index]) 
		segment_index++;

	if (segment_index == LARGE_BINS_SEGMENTS_COUNT)
		return LARGE_BINS_DUMP;

	// GET BIN INDEX IN SEGMENT
	int SizeInSegment = AlignedSize;
	if (segment_index == 0)
		SizeInSegment -= SMALL_ALLOC_MAX;
	else
		SizeInSegment -= segments[segment_index - 1];
	
	int space_between = GET_LARGE_BINS_SEGMENTS_SPACE_BETWEEN(segment_index);
	int index_in_segment = (SizeInSegment - 1) / space_between;

	// GET SEGMENT STARTING BIN INDEX
	int segment_starting_index = 0;
	int number_of_bins_in_segment = 1 << 5;
	while (segment_index > 0) {
		segment_starting_index += number_of_bins_in_segment;
		number_of_bins_in_segment = number_of_bins_in_segment >> 1;
		segment_index--;
	}

	int result = segment_starting_index + index_in_segment;

	return result;
}

int get_bin_index(size_t AlignedSize, t_zonetype ZoneType) {
	int result = 0;

	if (ZoneType == TINY) {
		if (AlignedSize > TINY_ALLOC_MAX)
			result = TINY_BINS_DUMP;
		else
			result = (AlignedSize / ALIGNMENT) - 1;
	} else if (ZoneType == SMALL) {
		if (AlignedSize > SMALL_ALLOC_MAX)
			result = SMALL_BINS_DUMP;
		else
			result = ((AlignedSize - TINY_ALLOC_MAX) / ALIGNMENT) - 1;
	} else {
		result = get_large_bin_index(AlignedSize);
	}

	return result;
}

void	put_slot_in_bin(t_header *Hdr, t_memzone *Zone) {
	int BinSize = Hdr->RealSize - HEADER_SIZE;

	int Index = get_bin_index(BinSize, Zone->ZoneType);

	t_header **Bins = Zone->Bins;

	if (Zone->ZoneType != LARGE) {
		t_header *NextHdrInBin = Bins[Index];
		
		Bins[Index] = Hdr;
		Hdr->PrevFree = NULL;
		Hdr->NextFree = NextHdrInBin;
		
		if (NextHdrInBin != NULL)
			NextHdrInBin->PrevFree = Hdr;
	} else {
		t_header *HdrIter = Bins[Index];

		t_header *PrevHdrIter = NULL;

		while (HdrIter != NULL && HdrIter->RealSize > Hdr->RealSize) {
			PrevHdrIter = HdrIter;
			HdrIter = HdrIter->NextFree;
		}

		Hdr->NextFree = HdrIter;
		Hdr->PrevFree = PrevHdrIter;

		if (HdrIter != NULL)
			HdrIter->PrevFree = Hdr;

		if (PrevHdrIter != NULL)
			PrevHdrIter->NextFree = Hdr;
		else
			Bins[Index] = Hdr;
	}
}

void	remove_slot_from_bin(t_header *Hdr, t_memzone *Zone) {
	int index = get_bin_index(Hdr->RealSize - HEADER_SIZE, Zone->ZoneType);

	if (Hdr->PrevFree != NULL) {
		Hdr->PrevFree->NextFree = Hdr->NextFree;
	} else {
		Zone->Bins[index] = Hdr->NextFree;
	}

	if (Hdr->NextFree != NULL)
			Hdr->NextFree->PrevFree = Hdr->PrevFree;

	Hdr->PrevFree = NULL;
	Hdr->NextFree = NULL;
}

void	try_coalesce_slot(t_header *Hdr, t_header **NextHdrToCheck, t_memzone *Zone) {
	t_header *NextFree = Hdr->NextFree;

	t_header *Base = Hdr;
	t_header *Prev = Base->Prev;

	//TODO(felix): optimize this part & start coalescing when backtracking
	while (Prev != NULL && Prev->Free) {
		Base = Prev;
		Prev = Base->Prev;
	}
	
	size_t NewSize = Base->RealSize;
	t_header *Current = Base;
	t_header *Next = Current->Next;
	
	while (Next != NULL && Next->Free) {

		while (NextFree != NULL 
			&& (uint64_t)NextFree > (uint64_t)Base
			&& (uint64_t)NextFree <= (uint64_t)Next)
			NextFree = NextFree->NextFree;

		Current = Next;
		NewSize += Current->RealSize;
		remove_slot_from_bin(Current, Zone);
		Next = Current->Next; //UNFLAG(Current->Next);
	}

	if (Base->RealSize != NewSize) { 
		remove_slot_from_bin(Base, Zone);
		Base->RealSize = NewSize;
		Base->Next = Current->Next;

		if (Next != NULL)
			Next->Prev = Base;
	
		put_slot_in_bin(Base, Zone);
	}

	*NextHdrToCheck = NextFree;

#ifdef SCAN_MEMORY_FREE
  	scan_memory_integrity();
#endif
}

void	coalesce_slots(t_memzone *Zone) {
	int i = 0;

	while (i < Zone->BinsCount) {
		t_header *Hdr = Zone->Bins[i];

		while (Hdr != NULL) {
			try_coalesce_slot(Hdr, &Hdr, Zone);
		}

		i++;
	}
}

void	free_slot(t_header *Hdr) {
 	size_t BlockSize = Hdr->RealSize - HEADER_SIZE;
	
	t_memzone *Zone = NULL;
	if (BlockSize > SMALL_ALLOC_MAX) {
		Zone = GET_LARGE_ZONE();
	} else if (BlockSize > TINY_ALLOC_MAX) {
		Zone = GET_SMALL_ZONE();
	} else {
		Zone = GET_TINY_ZONE();
	}

	put_slot_in_bin(Hdr, Zone);
}

void	flush_unsorted_bin() {
	t_header *Hdr = MemoryLayout.UnsortedBin;

	while (Hdr != NULL) {
		t_header *Next = Hdr->NextFree;
		free_slot(Hdr);
		Hdr = Next;
	}

	MemoryLayout.UnsortedBin = NULL;
}

void	add_to_unsorted_bin(t_header *Hdr) {

	Hdr->Free = 1;

	t_header *FirstBinHdr = MemoryLayout.UnsortedBin;

	Hdr->PrevFree = NULL;
	Hdr->NextFree = NULL;

	MemoryLayout.UnsortedBin = Hdr;

	if (FirstBinHdr == NULL)
		return;

	Hdr->NextFree = FirstBinHdr;
	FirstBinHdr->PrevFree = Hdr;
}


// ----------- FREE ------------ //

void	free(void *Ptr) {

	//TODO(felix): verify if block need to be freed beforehand !!!	

	if (Ptr == NULL)
		return;

	t_header *Hdr = GET_HEADER(Ptr);

	if (Hdr->Free)
		return;

#ifdef PRINT_FREE
	PRINT("Freeing address "); PRINT_ADDR(Ptr); PRINT(", Header: "); PRINT_ADDR(Hdr); NL();
#endif

	add_to_unsorted_bin(Hdr);

#ifdef SCAN_MEMORY_FREE
	scan_memory_integrity();
#endif
}

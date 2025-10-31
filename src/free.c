#include <sys/mman.h>
#include <errno.h>
#include "malloc.h"
#include "utils.h"
#include "../ft_malloc.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

//#define PRINT_FREE

void	lst_free_add(t_header **BeginList, t_header *Hdr) {
	
	Hdr->PrevFree = NULL;
	Hdr->NextFree = NULL;

	t_header *List = *BeginList;	

	*BeginList = Hdr;
	Hdr->NextFree = List;
	
	if (List != NULL)
		List->PrevFree = Hdr;
}

void	lst_free_remove(t_header **BeginList, t_header *Hdr) {
	if (*BeginList == Hdr) {
		*BeginList = Hdr->NextFree;
		return;
	}

	t_header *Prev = Hdr->PrevFree;
	t_header *Next = Hdr->NextFree;
	if (Prev != NULL)
		Prev->NextFree = Next;

 	if (Next != NULL)
		Next->PrevFree = Prev;	
}

size_t	coalesce_with_prev(t_header *MiddleHdr) {
	t_header *PrevHdr = UNFLAG(MiddleHdr->Prev);
	t_header *NextHdr = UNFLAG(MiddleHdr->Next);
	
	PrevHdr->Next = MiddleHdr->Next;
	if (NextHdr != NULL)
		NextHdr->Prev = MiddleHdr->Prev;

	PrevHdr->RealSize += MiddleHdr->RealSize;
	return PrevHdr->RealSize;
}

// --------- BINS ---------- //

void	show_bins(t_memchunks *Zone) {
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

int get_bin_index(size_t AlignedSize, t_zonetype ZoneType) {
	int result = 0;

	if (ZoneType == SMALL) {
		if (AlignedSize > SMALL_ALLOC_MAX)
			result = SMALL_BINS_DUMP;
		else
			result = ((AlignedSize - TINY_ALLOC_MAX) / ALIGNMENT) - 1;
	}
	else
	{
		if (AlignedSize > TINY_ALLOC_MAX)
			result = TINY_BINS_DUMP;
		else
			result = (AlignedSize / ALIGNMENT) - 1;
	}

	return result;
}

void	put_slot_in_bin(t_header *Hdr, t_memchunks *Zone) {
	int BinSize = Hdr->RealSize - HEADER_SIZE;

	int Index = get_bin_index(BinSize, Zone->ZoneType);

	t_header **Bins = Zone->Bins;
	t_header *NextHdrInBin = Bins[Index];

	Bins[Index] = Hdr;
	Hdr->PrevFree = NULL;
	Hdr->NextFree = NextHdrInBin;

	if (NextHdrInBin != NULL)
		NextHdrInBin->PrevFree = Hdr;
}

void	remove_slot_from_bin(t_header *Hdr, t_memchunks *Zone) {
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

void	try_coalesce_slot(t_header *Hdr, t_header **NextHdrToCheck, t_memchunks *Zone) {
	t_header *NextFree = Hdr->NextFree;

	t_header *Base = Hdr;
	t_header *Prev = UNFLAG(Base->Prev);

	//TODO(felix): optimize this part & start coalescing when backtracking
	while (Prev != NULL && IS_FLAGGED(Base->Prev) == 0) {
		Base = Prev;
		Prev = UNFLAG(Base->Prev);
	}
	
	size_t NewSize = Base->RealSize;
	t_header *Current = Base;
	t_header *Next = UNFLAG(Current->Next);
	
	while (Next != NULL && IS_FLAGGED(Current->Next) == 0) {
	
		while (NextFree != NULL 
			&& (uint64_t)NextFree > (uint64_t)Base
			&& (uint64_t)NextFree <= (uint64_t)Next)
			NextFree = NextFree->NextFree;

		Current = Next;
		NewSize += Current->RealSize;
		remove_slot_from_bin(Current, Zone);
		Next = UNFLAG(Current->Next);
	}

	if (Base->RealSize != NewSize) { 
		remove_slot_from_bin(Base, Zone);
		Base->RealSize = NewSize;
		Base->Size = NewSize - HEADER_SIZE; 
		Base->Next = Current->Next;

		if (Next != NULL)
			Next->Prev = Base;
	
		put_slot_in_bin(Base, Zone);
	}

	*NextHdrToCheck = NextFree;

  	scan_memory_integrity();
}

void	coalesce_slots(t_memchunks *Zone) {
	int i = 0;

	while (i < Zone->BinsCount) {
		t_header *Hdr = Zone->Bins[i];

		while (Hdr != NULL) {
			try_coalesce_slot(Hdr, &Hdr, Zone);
		}

		i++;
	}
}

void	free_slot(t_header *Hdr, t_zonetype ZoneType) {
	t_header *HdrPrev = UNFLAG(Hdr->Prev);
	t_header *HdrNext = UNFLAG(Hdr->Next);

	if (HdrNext != NULL)
		HdrNext->Prev = Hdr;

	if (HdrPrev != NULL)
		HdrPrev->Next = Hdr;

	t_memchunks *Zone = NULL;
	if (ZoneType == SMALL)
		Zone = GET_SMALL_ZONE();
	else
		Zone = GET_TINY_ZONE();

	put_slot_in_bin(Hdr, Zone);
}

void	free_large_slot(t_header *Hdr) {
	t_memchunks *MemBlock = GET_LARGE_ZONE();

	lst_free_add(&MemBlock->FreeList, Hdr);

	t_header *PrevHdr = UNFLAG(Hdr->Prev);
	t_header *NextHdr = UNFLAG(Hdr->Next);

	//Coalesce
	if (PrevHdr != NULL && IS_FLAGGED(Hdr->Prev) == 0) {
		lst_free_remove(&MemBlock->FreeList, Hdr);
		size_t NewSize = coalesce_with_prev(Hdr);
		(void)NewSize;
		//PRINT("Coalesced with previous slot for total size "); PRINT_UINT64(NewSize); NL();
		Hdr = UNFLAG(Hdr->Prev);
		PrevHdr = UNFLAG(Hdr->Prev);
  	}

	if (NextHdr != NULL && IS_FLAGGED(Hdr->Next) == 0) {
    		t_header *NextSlot = NextHdr;
		lst_free_remove(&MemBlock->FreeList, NextSlot);
    		size_t NewSize = coalesce_with_prev(NextHdr);
		(void)NewSize;
		NextHdr = UNFLAG(Hdr->Next);
		//PRINT("Coalesced with following slot for total size "); PRINT_UINT64(NewSize); NL();
	}
	
	if (NextHdr != NULL)
		NextHdr->Prev = Hdr;

	if (PrevHdr != NULL) {
		PrevHdr->Next = Hdr;
	} else {
    		void *CurrentChunk = ((void *)Hdr - CHUNK_HEADER);	
	
		// Unmap ?
		size_t ChunkSize = GET_CHUNK_SIZE(CurrentChunk);
		if (Hdr->RealSize == CHUNK_USABLE_SIZE(ChunkSize)
			&& CurrentChunk != MemBlock->StartingBlockAddr) {
			
			lst_free_remove(&MemBlock->FreeList, Hdr);
#ifdef PRINT_FREE
      			PRINT(ANSI_COLOR_RED);
 			PRINT("Unmapping chunk at address "); PRINT_ADDR(CurrentChunk); PRINT(" and size "); PRINT_UINT64(ChunkSize); NL();
      			PRINT(ANSI_COLOR_RESET);
#endif
			void *PrevChunk = GET_PREV_CHUNK(CurrentChunk);
      			void *NextChunk = GET_NEXT_CHUNK(CurrentChunk);
      			if (munmap(CurrentChunk, GET_CHUNK_SIZE(CurrentChunk)) < 0)
      			{
        			PRINT_ERROR("Cannot unmap chunk, errno = "); PRINT_UINT64(errno); NL();
				return;
			}

			if (NextChunk != NULL)
				SET_PREV_CHUNK(NextChunk, PrevChunk);

			if (PrevChunk != NULL)	
				SET_NEXT_CHUNK(PrevChunk, NextChunk);		
      		else
				MemBlock->StartingBlockAddr = NextChunk;
		}	
  	}
}

// ----------- FREE ------------ //

void	free(void *Ptr) {

	//TODO(felix): verify if block need to be freed beforehand !!!	

	t_header *Hdr = GET_HEADER(Ptr);
 	size_t BlockSize = Hdr->Size;

#ifdef PRINT_FREE
	PRINT("Freeing address "); PRINT_ADDR(Ptr); PRINT(" (size: "); PRINT_UINT64(BlockSize); PRINT(", Header: "); PRINT_ADDR(GET_HEADER(Ptr)); PRINT(")"); NL();
#endif

	if (BlockSize > SMALL_ALLOC_MAX) {
		free_large_slot(Hdr);
	} else if (BlockSize > TINY_ALLOC_MAX) {
		free_slot(Hdr, SMALL);
	} else {
		free_slot(Hdr, TINY);
	}

	scan_memory_integrity();
}

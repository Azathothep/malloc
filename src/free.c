#include <sys/mman.h>
#include <errno.h>
#include "malloc.h"
#include "utils.h"
#include "../ft_malloc.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

#define PRINT_FREE

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

// --------- TINY BIN ---------- //

void	show_tiny_bins() {
	PRINT("TINY BINS\n");

	t_header **TinyBins = MemoryLayout.TinyBins;

	int i = 0;
	while (i < 9) {
		PRINT("["); PRINT_UINT64(i); PRINT("] (");
		if (i < 8)
			PRINT_UINT64((i + 1) * 8);
		else
			PRINT("+");

		PRINT(")"); NL();
		
		t_header *Ptr = TinyBins[i];
		while (Ptr != NULL) {
			PRINT_ADDR(Ptr); NL();
			Ptr = Ptr->NextFree;
		}
		NL();
		
		i++;
	}
}

int	get_tiny_bin_index(size_t AlignedSize) {
	if (AlignedSize > TINY_ALLOC_MAX)
		return 8;

	return (AlignedSize / 8) - 1;
}

void	put_tiny_slot_in_bin(t_header *Hdr) {
	int BinSize = Hdr->RealSize - HEADER_SIZE;

	int Index = get_tiny_bin_index(BinSize);

	PRINT("put_tiny_slot_in_bin: Hdr = "); PRINT_ADDR(Hdr); NL();

	t_header **TinyBins = MemoryLayout.TinyBins;
	t_header *NextHdrInBin = TinyBins[Index];

	TinyBins[Index] = Hdr;
	Hdr->PrevFree = NULL;
	Hdr->NextFree = NextHdrInBin;
	
	if (NextHdrInBin != NULL)
		NextHdrInBin->PrevFree = Hdr;
}

void	remove_tiny_slot_from_bin(t_header *Hdr) {
	PRINT("remove_tiny_slot_from_bin: "); PRINT_ADDR(Hdr); NL();
	//PRINT("remove_tiny_slot_from_bin: Hdr->PrevFree = "); PRINT_ADDR(Hdr->PrevFree); NL();
	//PRINT("remove_tiny_slot_from_bin: Hdr->NextFree = "); PRINT_ADDR(Hdr->NextFree); NL();

	if (Hdr->PrevFree != NULL)
		Hdr->PrevFree->NextFree = Hdr->NextFree;
	else {
		int index = get_tiny_bin_index(Hdr->RealSize);
		MemoryLayout.TinyBins[index] = Hdr->NextFree;	
	}

	if (Hdr->NextFree != NULL)
		Hdr->NextFree->PrevFree = Hdr->PrevFree;

	//Hdr->PrevFree = NULL;
	//Hdr->NextFree = NULL;
}

t_header	*try_coalesce_tiny_slot(t_header *Hdr) {
	PRINT("TRYING TO COALESCE "); PRINT_ADDR(Hdr); NL();
	t_header *NextFree = Hdr->NextFree;

	t_header *Base = Hdr;
	t_header *Prev = UNFLAG(Base->Prev);
	size_t BaseSize = Base->RealSize;

	while (Prev != NULL && IS_FLAGGED(Base->Prev) == 0) {
		Base = Prev;
		Prev = UNFLAG(Base->Prev);
	}

	PRINT("BACKTRACKED TO "); PRINT_ADDR(Base); PRINT(", Prev = "); PRINT_ADDR(Base->Prev); NL();
	remove_tiny_slot_from_bin(Base);
	
	t_header *Current = Base;
	t_header *Next = UNFLAG(Current->Next);
	
	while (Next != NULL && IS_FLAGGED(Current->Next) == 0) {
		Current = Next;
		PRINT("Merging "); PRINT_ADDR(Base); PRINT(" ["); PRINT_UINT64(Base->RealSize); PRINT("] and "); PRINT_ADDR(Current); PRINT(" ["); PRINT_UINT64(Current->RealSize); PRINT("]"); NL();
		remove_tiny_slot_from_bin(Current);
		Base->RealSize += Current->RealSize;
		Next = UNFLAG(Current->Next);
	}

	while (NextFree != NULL // TODO(felix): can remove NULL check: next check implicitly checks it 
		&& (uint64_t)NextFree > (uint64_t)Base
		&& (uint64_t)NextFree <= (uint64_t)Next)
			NextFree = NextFree->NextFree;

	if (Base->RealSize != BaseSize) {
		Base->Size = Base->RealSize - HEADER_SIZE;
		Base->Next = Current->Next;
		if (Next != NULL)
			Next->Prev = Base;

	}

	put_tiny_slot_in_bin(Base);
  	scan_memory_integrity();

	return NextFree;
}

void	coalesce_tiny_slots() {
	int i = 0;

	//show_alloc_mem();
	//show_tiny_bins();

	while (i < 9) {
		t_header *Hdr = MemoryLayout.TinyBins[i];
    PRINT("Coalescing slot index: "); PRINT_UINT64(i); NL();

		while (Hdr != NULL) {
			Hdr = try_coalesce_tiny_slot(Hdr);
		}	

		i++;
	}
}

void	free_tiny_slot(t_header *Hdr) {
#ifdef PRINT_FREE
	//PRINT("Freeing tiny slot "); PRINT_ADDR(GET_SLOT(Hdr)); NL();
#endif
	t_header *HdrPrev = UNFLAG(Hdr->Prev);
	t_header *HdrNext = UNFLAG(Hdr->Next);

	if (HdrNext != NULL)
		HdrNext->Prev = Hdr;

	if (HdrPrev != NULL)
		HdrPrev->Next = Hdr;	

	put_tiny_slot_in_bin(Hdr);	
#ifdef PRINT_FREE	
//	show_tiny_bins();
#endif
}

// ----------- FREE ------------ //

void	free(void *Ptr) {

	//TODO(felix): verify if block need to be freed beforehand	

	t_header *Hdr = GET_HEADER(Ptr);
 	size_t BlockSize = Hdr->Size;
	
#ifdef PRINT_FREE
	PRINT("Freeing address "); PRINT_ADDR(Ptr); PRINT(" (size: "); PRINT_UINT64(BlockSize); PRINT(", Header: "); PRINT_ADDR(GET_HEADER(Ptr)); PRINT(")"); NL();
#endif

  	t_memchunks *MemBlock = NULL;
	if (BlockSize > SMALL_ALLOC_MAX) {
		MemBlock = &MemoryLayout.LargeZone;
	} else if (BlockSize > TINY_ALLOC_MAX) {
		MemBlock = &MemoryLayout.SmallZone;
 	} else {
  		//MemBlock = &MemoryLayout.TinyZone;
		free_tiny_slot(Hdr);
		scan_memory_integrity();
		return;
	}

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

	scan_memory_integrity();
}

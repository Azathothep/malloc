#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include "utils.h"
#include "malloc.h"

#define PRINT_MALLOC

t_memlayout MemoryLayout;

// TODO(felix): change this to support multithreaded programs
void	*map_memory(int ChunkSize) {
	void *ptrToMappedMemory = mmap(NULL,
					ChunkSize,
					PROT_READ | PROT_WRITE,
					MAP_ANON | MAP_ANONYMOUS | MAP_PRIVATE, 
					-1, //fd
					0); //offset_t

	if (ptrToMappedMemory == MAP_FAILED) {
		PRINT_ERROR("Failed to map memory, errno = "); PRINT_UINT64(errno); NL();
		return NULL;
	}

#ifdef PRINT_MALLOC
	PRINT("Successfully mapped "); PRINT_UINT64(ChunkSize); PRINT(" bytes of memory to addr "); PRINT_ADDR(ptrToMappedMemory); NL();
#endif	

	return ptrToMappedMemory;
}


t_header	*get_free_slot(t_header **begin_lst, size_t size) {
	t_header *lst = *begin_lst;

	// First, check for a perfect fit...	
	while (lst != NULL && lst->RealSize != size) 
        	lst = lst->NextFree;

	if (lst != NULL)
		return lst;

	lst = *begin_lst;
	
	// Then, check for at least a double fit...
	while (lst != NULL && lst->RealSize < size * 2) 
		lst = lst->NextFree;

	if (lst != NULL)
		return lst;

	lst = *begin_lst;

	// Then, check for any fit
	while (lst != NULL && lst->RealSize < size) 
		lst = lst->NextFree;

	return lst;
}

void	*alloc_chunk(t_memchunks *MemZone, size_t ChunkSize) {
	void *NewChunk = map_memory(ChunkSize);
	if (NewChunk == NULL)
		return NULL;

	SET_CHUNK_SIZE(NewChunk, ChunkSize);
	SET_NEXT_CHUNK(NewChunk, NULL);	
	SET_PREV_CHUNK(NewChunk, NULL);

	if (MemZone->StartingBlockAddr == NULL) {
		MemZone->StartingBlockAddr = NewChunk;
		return NewChunk;
	}

	void *LastChunk = MemZone->StartingBlockAddr;
	void *NextChunk = GET_NEXT_CHUNK(LastChunk);
	while (NextChunk != NULL) {
		LastChunk = NextChunk;
		NextChunk = GET_NEXT_CHUNK(NextChunk);
	}

	SET_NEXT_CHUNK(LastChunk, NewChunk);
	SET_PREV_CHUNK(NewChunk, LastChunk);
	return NewChunk;	
}

t_header	*break_tiny_slot(t_header *Hdr, size_t RequestedSize) {
	int FullSize = RequestedSize + HEADER_SIZE;
	size_t NewSize = Hdr->RealSize - FullSize;
	Hdr->RealSize = NewSize;
		
	t_header *PrevHdr = Hdr;

	Hdr = (t_header *)((void *)Hdr + NewSize);

	Hdr->Size = RequestedSize;
	Hdr->RealSize = FullSize;
	Hdr->Prev = PrevHdr;
	Hdr->Next = PrevHdr->Next;

	PrevHdr->Next = FLAG(Hdr);
	
	// PUT BROKEN PART TO NEW BIN
	put_tiny_slot_in_bin(PrevHdr);	

	return Hdr;
} 

t_header	*allocate_and_initialize_chunk(t_memchunks *MemZone, size_t ChunkSize) {
	void *NewChunk = alloc_chunk(MemZone, ChunkSize);
	if (NewChunk == NULL)
		return NULL;

	void *ChunkStartingAddr = CHUNK_STARTING_ADDR(NewChunk);

	t_header *Hdr = (t_header *)ChunkStartingAddr;
	Hdr->Prev = NULL;
	Hdr->Next = NULL;
	Hdr->Size = CHUNK_USABLE_SIZE(ChunkSize);
	Hdr->RealSize = Hdr->Size;
	
	Hdr->PrevFree = NULL;
	Hdr->NextFree = NULL;
	
	return Hdr;
}

t_header	*get_perfect_or_breakable_tiny_slot(size_t AlignedSize) {
	int index = get_tiny_bin_index(AlignedSize);
//	PRINT("Bin index for size "); PRINT_UINT64(AlignedSize); PRINT(": "); PRINT_UINT64(index); NL();

	t_header **TinyBins = MemoryLayout.TinyBins;

	// FOUND PERFECT FIT RIGHT AWAY
	if (TinyBins[index] != NULL) {
		t_header *Hdr = TinyBins[index];
		TinyBins[index] = Hdr->NextFree;
		return Hdr;
	}

	// TRY TO FIND A BIGGER SLOT THAT CAN BE SPLIT IN TWO
	size_t MinSlotSizeToBreak = ALIGNMENT + (HEADER_SIZE + AlignedSize);
	index = get_tiny_bin_index(MinSlotSizeToBreak);
	
	while (index < 8 && TinyBins[index] == NULL) {
		index++;
	}

	t_header *Hdr = NULL;

	if (index < 8) {
		Hdr = TinyBins[index];
		TinyBins[index] = Hdr->NextFree;
	} else {
		Hdr = TinyBins[8];
		MinSlotSizeToBreak = (HEADER_SIZE + AlignedSize) + (HEADER_SIZE + ALIGNMENT);
		while (Hdr != NULL && Hdr->RealSize < MinSlotSizeToBreak)
			Hdr = Hdr->NextFree;

		if (Hdr == NULL)
			return NULL;

		if (Hdr->PrevFree != NULL)
			Hdr->PrevFree->NextFree = Hdr->NextFree;
		else
			TinyBins[8] = NULL;

		if (Hdr->NextFree != NULL)
			Hdr->NextFree->PrevFree = Hdr->PrevFree;
	}

	Hdr = break_tiny_slot(Hdr, AlignedSize);
	return Hdr;
}

t_header	*get_tiny_slot(size_t AlignedSize) {
	
	t_header *Hdr = get_perfect_or_breakable_tiny_slot(AlignedSize);

	if (Hdr != NULL)
		return Hdr;

	// TRY TO COALESCE FREE SLOTS
	//if (AlignedSize > MIN_ALLOC) {
		coalesce_tiny_slots();
		Hdr = get_perfect_or_breakable_tiny_slot(AlignedSize);
				
		if (Hdr != NULL)
			return Hdr;
	//}

	// ELSE, ALLOCATE NEW CHUNK
	Hdr = allocate_and_initialize_chunk(&MemoryLayout.TinyZone, TINY_CHUNK);
	if (Hdr == NULL)
		return NULL;

	Hdr = break_tiny_slot(Hdr, AlignedSize);
	return Hdr;
}

t_header	*get_small_large_slot(size_t AlignedSize) {	
	size_t RequestedSize = AlignedSize + HEADER_SIZE;
		
	t_memchunks *MemZone = NULL;
	int MinSlotSize = 0;
	if (AlignedSize > SMALL_ALLOC_MAX) {
		MemZone = &MemoryLayout.LargeZone;
		MinSlotSize = LARGE_SPACE_MIN;
	} else {
		MemZone = &MemoryLayout.SmallZone;
		MinSlotSize = SMALL_SPACE_MIN;
	}
	
	t_header *Hdr = get_free_slot(&MemZone->FreeList, RequestedSize);
	if (Hdr == NULL) {
		// allocate new chunk
		int ChunkSize = 0;

		if (AlignedSize > SMALL_ALLOC_MAX) {
			ChunkSize = LARGE_CHUNK(AlignedSize);
			if (ChunkSize < LARGE_PREALLOC)
				ChunkSize = LARGE_PREALLOC;
		} else if (AlignedSize > TINY_ALLOC_MAX) {
			ChunkSize = SMALL_CHUNK;
    		} else {
			ChunkSize = TINY_CHUNK;
    		}

		Hdr = allocate_and_initialize_chunk(MemZone, ChunkSize);
		if (Hdr == NULL)
			return NULL;	
	
 		lst_free_add(&MemZone->FreeList, Hdr);
	}

	if (Hdr->RealSize >= (RequestedSize + MinSlotSize)) {
		size_t NewSize = Hdr->RealSize - RequestedSize;
		Hdr->RealSize = NewSize;
		
		t_header *PrevHdr = Hdr;

		Hdr = (t_header *)((void *)Hdr + NewSize);
	
			//Hdr = (t_header *)Addr;
			
		Hdr->Size = AlignedSize;
		Hdr->RealSize = RequestedSize;
		Hdr->Prev = PrevHdr; 
		Hdr->Next = PrevHdr->Next;

		PrevHdr->Next = FLAG(Hdr);
	} else {
		Hdr->Size = AlignedSize;
		lst_free_remove(&MemZone->FreeList, Hdr);
	
		t_header *PrevHdr = UNFLAG(Hdr->Prev);
		if (PrevHdr != NULL) {
			PrevHdr->Next = FLAG(Hdr);
		}	
	}

	return Hdr;
}

void	*malloc_block(size_t size) {
	size_t AlignedSize = SIZE_ALIGN(size);
	t_header *Hdr = NULL;

	if (AlignedSize <= TINY_ALLOC_MAX) {
		if (AlignedSize < MIN_ALLOC)
			AlignedSize = MIN_ALLOC;

		Hdr = get_tiny_slot(AlignedSize);	
	} else {
		Hdr = get_small_large_slot(AlignedSize);
	}
	
	t_header *NextHdr = UNFLAG(Hdr->Next);		
	t_header *PrevHdr = UNFLAG(Hdr->Prev);

	if (PrevHdr != NULL) {
		PrevHdr->Next = FLAG(Hdr);
	} else {
		// How to manage if first of the block ?
	}

	if (NextHdr != NULL)
		NextHdr->Prev = FLAG(Hdr);

	void *AllocatedPtr = GET_SLOT(Hdr);

#ifdef PRINT_MALLOC
	PRINT("Allocated "); PRINT_UINT64(AlignedSize); PRINT(" ["); PRINT_UINT64(AlignedSize + HEADER_SIZE); PRINT("] bytes at address "); PRINT_ADDR(AllocatedPtr); NL();
#endif

	return AllocatedPtr;
}

void	*malloc(size_t size) {
	//PRINT("Malloc request of size "); PRINT_UINT64(size); NL();

	void *Allocation = malloc_block(size);
	scan_memory_integrity();
	return Allocation;
}

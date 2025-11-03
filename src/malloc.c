#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include "utils.h"
#include "malloc.h"

//#define PRINT_MALLOC

t_memlayout MemoryLayout = {
	TINY, NULL, NULL, TINY_BINS_COUNT, { },

	SMALL, NULL, NULL, SMALL_BINS_COUNT, { },

	LARGE, NULL, NULL, LARGE_BINS_COUNT, { }
};

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

t_header	*allocate_and_initialize_chunk(t_memchunks *MemZone, size_t ChunkSize) {
	void *NewChunk = alloc_chunk(MemZone, ChunkSize);
	if (NewChunk == NULL)
		return NULL;

	void *ChunkStartingAddr = CHUNK_STARTING_ADDR(NewChunk);

	t_header *Hdr = (t_header *)ChunkStartingAddr;
	Hdr->Prev = NULL;
	Hdr->Next = NULL;
	Hdr->RealSize = CHUNK_USABLE_SIZE(ChunkSize) - HEADER_SIZE;
	
	Hdr->PrevFree = NULL;
	Hdr->NextFree = NULL;
	
	return Hdr;
}

t_header	*break_slot(t_header *Hdr, size_t AllocatedSize) {
	int FullSize = AllocatedSize + HEADER_SIZE;
	size_t NewSize = Hdr->RealSize - FullSize;
	Hdr->RealSize = NewSize;
		
	t_header *PrevHdr = Hdr;
	t_header *NextHdr = UNFLAG(Hdr->Next);

	Hdr = (t_header *)((void *)Hdr + NewSize);

	Hdr->RealSize = FullSize;

	Hdr->Prev = PrevHdr;
	Hdr->Next = PrevHdr->Next;

	PrevHdr->Next = FLAG(Hdr);
	
	if (NextHdr != NULL)
		NextHdr->Prev = FLAG(Hdr);
	
	return Hdr;
}

t_header *get_slot_of_size_in_large_bin(size_t AlignedSize, t_memchunks *Zone, int BinIndex) {
	size_t RequestedSize = AlignedSize + HEADER_SIZE;

	if (Zone->ZoneType != LARGE)
		return Zone->Bins[BinIndex];

	t_header *Hdr = Zone->Bins[BinIndex];
	if (Hdr == NULL || Hdr->RealSize < RequestedSize) // If first one is already too small
		return NULL;

	while (Hdr != NULL) {										// If looping, it means Hdr is larger or equal too the RequestSize
			if (Hdr->NextFree == NULL							// If is last one
				|| Hdr->NextFree->RealSize < RequestedSize) 	// or next one is too small
			break;

		Hdr = Hdr->NextFree;
	}

	return Hdr;
}

t_header *get_perfect_slot(size_t AlignedSize, t_memchunks *Zone) {
	int index = get_bin_index(AlignedSize, Zone->ZoneType);

	t_header *Hdr = NULL;

	int bin_dump = 0;
	if (Zone->ZoneType == TINY)
		bin_dump = TINY_BINS_DUMP;
	else if (Zone->ZoneType == SMALL)
		bin_dump = SMALL_BINS_DUMP;
	else
		bin_dump = LARGE_BINS_DUMP;
	
	if (index >= bin_dump)
		return NULL;

	if (Zone->ZoneType == LARGE) {
		Hdr = get_slot_of_size_in_large_bin(AlignedSize, Zone, index);
	} else {
		Hdr = Zone->Bins[index];
	}

	if (Hdr == NULL)
		return NULL;

	remove_slot_from_bin(Hdr, Zone);

	return Hdr;
}

t_header	*get_breakable_slot(size_t AlignedSize, t_memchunks *Zone) {
	int min_alloc = 0;
	int bin_dumps = 0;

	if (Zone->ZoneType == TINY) {
		min_alloc = MIN_TINY_ALLOC;
		bin_dumps = TINY_BINS_DUMP;
	}
	else if (Zone->ZoneType == SMALL) {
		min_alloc = MIN_SMALL_ALLOC;
		bin_dumps = SMALL_BINS_DUMP;
	} else {
		min_alloc = MIN_LARGE_ALLOC;
		bin_dumps = LARGE_BINS_DUMP;
	}

	size_t MinSlotSizeToBreak = min_alloc + (HEADER_SIZE + AlignedSize);
	int index = get_bin_index(MinSlotSizeToBreak, Zone->ZoneType);

	t_header *Hdr = NULL;
	
	while (index < bin_dumps) {
		if (Zone->ZoneType == LARGE) {
			Hdr = get_slot_of_size_in_large_bin(MinSlotSizeToBreak, Zone, index);			
		} else {
			Hdr = Zone->Bins[index];
		}
		
		if (Hdr != NULL) {
			break;
		}

		index++;
	}

	if (index >= bin_dumps) {
		Hdr = Zone->Bins[bin_dumps];
		
		while (Hdr != NULL
			&& Hdr->RealSize < (HEADER_SIZE + MinSlotSizeToBreak)) {
			
			Hdr = Hdr->NextFree;
		}

		if (Hdr == NULL)
			return NULL;
	}

	remove_slot_from_bin(Hdr, Zone);
	return Hdr;
}

t_header	*get_perfect_or_break_slot(size_t AlignedSize, t_memchunks *Zone) {
	t_header *Hdr = get_perfect_slot(AlignedSize, Zone);

	if (Hdr != NULL)
		return Hdr;

	Hdr = get_breakable_slot(AlignedSize, Zone);

	if (Hdr == NULL)
		return NULL;

	t_header *ToBreak = Hdr;
	Hdr = break_slot(ToBreak, AlignedSize);
	put_slot_in_bin(ToBreak, Zone);
	return Hdr;
}

t_header	*get_slot(size_t AlignedSize, t_zonetype ZoneType) {
	t_memchunks *Zone = NULL;
	size_t ChunkSize = 0;

	if (ZoneType == TINY) {
		Zone = GET_TINY_ZONE();
		ChunkSize = TINY_CHUNK;
	}
	else if (ZoneType == SMALL) {
		Zone = GET_SMALL_ZONE();
		ChunkSize = SMALL_CHUNK;
	} else {
		Zone = GET_LARGE_ZONE();
		ChunkSize = LARGE_CHUNK(AlignedSize);
	
		if (ChunkSize < (size_t)LARGE_PREALLOC)
			ChunkSize = LARGE_PREALLOC;
	}

	t_header *Hdr = get_perfect_or_break_slot(AlignedSize, Zone);

	if (Hdr != NULL)
		return Hdr;

	coalesce_slots(Zone);
	Hdr = get_perfect_or_break_slot(AlignedSize, Zone);

	if (Hdr != NULL)
		return Hdr;

	Hdr = allocate_and_initialize_chunk(Zone, ChunkSize);
	
	if (Hdr == NULL)
		return NULL;

	t_header *ToBreak = Hdr;
	Hdr = break_slot(ToBreak, AlignedSize);
	put_slot_in_bin(ToBreak, Zone);

	return Hdr;
}

void	*malloc_block(size_t size) {
	size_t AlignedSize = SIZE_ALIGN(size);

	t_header *Hdr = NULL;
	
	if (AlignedSize <= TINY_ALLOC_MAX) {
		
		if (AlignedSize < MIN_TINY_ALLOC)
			AlignedSize = MIN_TINY_ALLOC;

		Hdr = get_slot(AlignedSize, TINY);		
	} else if (AlignedSize <= SMALL_ALLOC_MAX) {
		Hdr = get_slot(AlignedSize, SMALL);
	} else {
		Hdr = get_slot(AlignedSize, LARGE);
	}
	
	t_header *NextHdr = UNFLAG(Hdr->Next);		
	t_header *PrevHdr = UNFLAG(Hdr->Prev);

	if (PrevHdr != NULL) {
		PrevHdr->Next = FLAG(Hdr);
	} else {
		// TODO: how to flag block as occupied if it is the first ?
		// can't just rely on the next one: can be the only one in its chunk
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

	if (size == 0)
		return NULL;

	void *Allocation = malloc_block(size);
	
	scan_memory_integrity();
	
	return Allocation;
}

#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include "utils.h"
#include "malloc.h"

t_memlayout MemoryLayout = {
	TINY, NULL, NULL, { },  TINY_BINS_COUNT, { },

	SMALL, NULL, NULL, { }, SMALL_BINS_COUNT, { },

	LARGE, NULL, NULL, { }, LARGE_BINS_COUNT, { },

	NULL
};

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

	return ptrToMappedMemory;
}

void	*alloc_chunk(t_memzone *Zone, size_t ChunkSize) {
	t_memchunk *NewChunk = (t_memchunk *)map_memory(ChunkSize);
	if (NewChunk == NULL)
		return NULL;

	NewChunk->FullSize = ChunkSize;
	NewChunk->Next = NULL;
	NewChunk->Prev = NULL;

	if (Zone->FirstChunk == NULL) {
		Zone->FirstChunk = NewChunk;
		return NewChunk;
	}

	t_memchunk *LastChunk = Zone->FirstChunk;
	t_memchunk *NextChunk = LastChunk->Next; 

	// Get the current last chunk in zone
	while (NextChunk != NULL) {
		LastChunk = NextChunk;
		NextChunk = NextChunk->Next;
	}

	// Append the new chunk after the last
	LastChunk->Next = NewChunk;
	NewChunk->Prev = LastChunk;

	return NewChunk;
}

// Allocate a new chunk for the provided zone and initialize it
t_header	*allocate_and_initialize_chunk(t_memzone *Zone, size_t ChunkSize) {
	void *NewChunk = alloc_chunk(Zone, ChunkSize);
	if (NewChunk == NULL)
		return NULL;

	void *ChunkStartingAddr = CHUNK_STARTING_ADDR(NewChunk);

	t_header *Hdr = (t_header *)ChunkStartingAddr;
	Hdr->Prev = NULL;
	Hdr->Next = NULL;
	Hdr->State = FREE;
	Hdr->SlotSize = CHUNK_USABLE_SIZE(ChunkSize);
	
	Hdr->PrevFree = NULL;
	Hdr->NextFree = NULL;

	Zone->MemStatus.TotalMappedMemSize += Hdr->SlotSize;
	Zone->MemStatus.TotalFreedMemSize += Hdr->SlotSize;

	return Hdr;
}

// Break the provided slot in two parts, the last one being of the required size
t_header	*break_slot(t_header *Hdr, size_t RequestedSize) {
	int RequiredSlotSize = RequestedSize + HEADER_SIZE;
	size_t LeftoverSize = Hdr->SlotSize - RequiredSlotSize;
	Hdr->SlotSize = LeftoverSize;
		
	t_header *PrevHdr = Hdr;
	t_header *NextHdr = Hdr->Next; 

	Hdr = (t_header *)((void *)Hdr + LeftoverSize);

	Hdr->SlotSize = RequiredSlotSize;

	Hdr->Prev = PrevHdr;
	Hdr->Next = PrevHdr->Next;

	PrevHdr->Next = Hdr;

	if (NextHdr != NULL)
		NextHdr->Prev = Hdr;

	return Hdr;
}

t_header *get_slot_of_size_in_large_bin(size_t RequestedSize, t_memzone *Zone, int BinIndex) {
	size_t RequiredSlotSize = RequestedSize + HEADER_SIZE;

	t_header *Hdr = Zone->Bins[BinIndex];
	if (Hdr == NULL || Hdr->SlotSize < RequiredSlotSize) // Large bins are sorted from biggest to smalles. If first one is already too small, the rest won't do.
		return NULL;

	while (Hdr != NULL) {	// If looping, it means Hdr is larger or equal too the RequestSize
			if (Hdr->NextFree == NULL	// If is last one
				|| Hdr->NextFree->SlotSize < RequiredSlotSize) 	// Or next one is too small
			break;

		Hdr = Hdr->NextFree;
	}

	return Hdr;
}

t_header *get_perfect_slot(size_t RequestedSize, t_memzone *Zone) {
	int BinIndex = get_bin_index(RequestedSize, Zone->ZoneType);

	t_header *Hdr = NULL;

	int ZoneBinDumpIndex = 0;
	if (Zone->ZoneType == TINY)
		ZoneBinDumpIndex = TINY_BINS_DUMP;
	else if (Zone->ZoneType == SMALL)
		ZoneBinDumpIndex = SMALL_BINS_DUMP;
	else
		ZoneBinDumpIndex = LARGE_BINS_DUMP;
	
	if (BinIndex >= ZoneBinDumpIndex)
		return NULL;

	if (Zone->ZoneType == LARGE) {
		Hdr = get_slot_of_size_in_large_bin(RequestedSize, Zone, BinIndex);
	} else {
		Hdr = Zone->Bins[BinIndex];
	}

	if (Hdr == NULL)
		return NULL;

	remove_slot_from_bin(Hdr, Zone);

	return Hdr;
}

t_header	*get_breakable_slot(size_t RequestedSize, t_memzone *Zone) {
	int ZoneMinAllocSize = 0;
	int ZoneBinDumpIndex = 0;

	if (Zone->ZoneType == TINY) {
		ZoneMinAllocSize = MIN_TINY_ALLOC;
		ZoneBinDumpIndex = TINY_BINS_DUMP;
	}
	else if (Zone->ZoneType == SMALL) {
		ZoneMinAllocSize = MIN_SMALL_ALLOC;
		ZoneBinDumpIndex = SMALL_BINS_DUMP;
	} else {
		ZoneMinAllocSize = MIN_LARGE_ALLOC;
		ZoneBinDumpIndex = LARGE_BINS_DUMP;
	}

	// The minimum slot size to find
	size_t MinSlotSizeToBreak = ZoneMinAllocSize + (HEADER_SIZE + RequestedSize);
	int BinIndex = get_bin_index(MinSlotSizeToBreak, Zone->ZoneType);

	t_header *Hdr = NULL;
	
	while (BinIndex < ZoneBinDumpIndex) {
		if (Zone->ZoneType == LARGE) {
			Hdr = get_slot_of_size_in_large_bin(MinSlotSizeToBreak, Zone, BinIndex);			
		} else {
			Hdr = Zone->Bins[BinIndex];
		}
		
		if (Hdr != NULL) {
			break;
		}

		BinIndex++;
	}

	// Couldn't find a slot in bin, try to get one from the bin dump
	if (BinIndex >= ZoneBinDumpIndex) {
		Hdr = Zone->Bins[ZoneBinDumpIndex];
		
		while (Hdr != NULL
			&& Hdr->SlotSize < (HEADER_SIZE + MinSlotSizeToBreak)) {
			
			Hdr = Hdr->NextFree;
		}

		if (Hdr == NULL)
			return NULL;
	}

	remove_slot_from_bin(Hdr, Zone);
	return Hdr;
}

t_header	*get_perfect_or_break_slot(size_t RequestedSize, t_memzone *Zone) {
	t_header *Hdr = get_perfect_slot(RequestedSize, Zone);

	if (Hdr != NULL)
		return Hdr;

	// If perfect slot not found, get a slot large enough to be broken
	Hdr = get_breakable_slot(RequestedSize, Zone);

	if (Hdr == NULL)
		return NULL;

	t_header *ToBreak = Hdr;
	Hdr = break_slot(ToBreak, RequestedSize);
	// ToBreak has been broken in two parts, with the last being the one we're interested in.
	// ToBreak points to the leftover
	Hdr->State = FREE;
	put_slot_in_bin(ToBreak, Zone);
	return Hdr;
}

t_header	*get_slot_from_zone(size_t RequestedSize, t_memzone *Zone) {	
	t_header *Hdr = get_perfect_or_break_slot(RequestedSize, Zone);

	if (Hdr != NULL)
		Hdr->State = INUSE;

	// Regardless of the previous result, always flush the unsorted bin after the first allocation request not found in unsorted bin
	flush_unsorted_bin();

	if (Hdr != NULL)
		return Hdr;

	coalesce_slots(Zone);

	// After coalescion, retry to get a slot from freed memory
	Hdr = get_perfect_or_break_slot(RequestedSize, Zone);

	if (Hdr != NULL)
		return Hdr;

	// Else, create a new chunk, allocate it and break a new slot of the required side
	size_t ChunkSize = 0;

	if (Zone->ZoneType == TINY) {
		ChunkSize = TINY_CHUNK;
	}
	else if (Zone->ZoneType == SMALL) {
		ChunkSize = SMALL_CHUNK;
	} else {
		ChunkSize = LARGE_CHUNK(RequestedSize);
	
		if (ChunkSize < (size_t)LARGE_PREALLOC)
			ChunkSize = LARGE_PREALLOC;
	}

	Hdr = allocate_and_initialize_chunk(Zone, ChunkSize);
	
	if (Hdr == NULL)
		return NULL;

	t_header *ToBreak = Hdr;
	Hdr = break_slot(ToBreak, RequestedSize);
	put_slot_in_bin(ToBreak, Zone);

	return Hdr;
}

// Get associated zone and try get slot in this zone
t_header	*get_zone_slot(size_t RequestedSize) {
	t_memzone *Zone = NULL;

	if (RequestedSize <= TINY_ALLOC_MAX) {
		
		if (RequestedSize < MIN_TINY_ALLOC)
			RequestedSize = MIN_TINY_ALLOC;

		Zone = GET_TINY_ZONE();
	} else if (RequestedSize <= SMALL_ALLOC_MAX) {
		Zone = GET_SMALL_ZONE();
	} else {
		Zone = GET_LARGE_ZONE();
	}

	t_header *Hdr = get_slot_from_zone(RequestedSize, Zone);

	if (Hdr == NULL)
		return NULL;

	Zone->MemStatus.TotalFreedMemSize -= Hdr->SlotSize;

	return Hdr;
}

t_header	*find_in_unsorted_bin(size_t RequestedSize) {
	t_header *Hdr = MemoryLayout.UnsortedBin;

	size_t RequiredSlotSize = RequestedSize + HEADER_SIZE;
	
	while (Hdr != NULL) {
		
		if (Hdr->SlotSize != RequiredSlotSize) {
			Hdr = Hdr->NextFree;
			continue;
		}

		if (Hdr->PrevFree != NULL)
			Hdr->PrevFree->NextFree = Hdr->NextFree;
		else
			MemoryLayout.UnsortedBin = Hdr->NextFree;

		if (Hdr->NextFree != NULL)
			Hdr->NextFree->PrevFree = Hdr->PrevFree;
	
		break;
	}

	return Hdr;
}

void	*get_slot_for_size(size_t Size) {
	size_t RequestedSize = SIZE_ALIGN(Size);

	t_header *Hdr = find_in_unsorted_bin(RequestedSize);

	// If no slot of the provided size find in unsorted bin, allocate a slot
	if (Hdr == NULL)
		Hdr = get_zone_slot(RequestedSize);

	// If coudn't allocate, it means no memory is available
	if (Hdr == NULL)
		return NULL;

	Hdr->State = INUSE;

	void *AllocatedPtr = GET_SLOT(Hdr);

	return AllocatedPtr;
}

void	*malloc(size_t Size) {

	if (Size == 0)
		return NULL;

	void *AllocatedPtr = get_slot_for_size(Size);

	return AllocatedPtr;
}

#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include "libft.h"
#include "utils.h"
#include "malloc.h"
#include "lst_free.h"

t_memlayout MemoryLayout;

// TODO(felix): change this to support multithreaded programs
void	*map_memory(int ZoneSize) {
	void *ptrToMappedMemory = mmap(NULL,
					ZoneSize,
					PROT_READ | PROT_WRITE,
					MAP_ANON | MAP_ANONYMOUS | MAP_PRIVATE, 
					-1, //fd
					0); //offset_t

	if (ptrToMappedMemory == MAP_FAILED) {
		PRINT_ERROR("Failed to map memory, errno = "); PRINT_UINT64(errno); NL();
		return NULL;
	}

	PRINT("Successfully mapped "); PRINT_UINT64(ZoneSize); PRINT(" bytes of memory to addr ");
	PRINT_ADDR(ptrToMappedMemory); NL();
	return ptrToMappedMemory;
}

t_free	*get_free_slot(t_free **begin_lst, size_t size) {
	t_free *lst = *begin_lst;
	if (lst == NULL)
		return NULL;

	while (lst != NULL && lst->Size < size) {
        lst = lst->Next;
	}

	return lst;
}

void	*alloc_chunk(t_memchunks *MemZone, size_t ChunkSize) {
	void *NewChunk = map_memory(ChunkSize);
	if (NewChunk == NULL)
		return NULL;

	SET_CHUNK_SIZE(NewChunk, ChunkSize);
	SET_NEXT_CHUNK(NewChunk, NULL);	

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
	return NewChunk;	
}

void	*malloc_block(size_t size) {
	size_t AlignedSize = SIZE_ALIGN(size);
	size_t RequestedSize = AlignedSize + HEADER_SIZE;

	t_memchunks *MemZone = NULL;
	int MinSlotSize = 0;
	if (size > SMALL_ALLOC) {
		MemZone = &MemoryLayout.LargeZone;
		MinSlotSize = LARGE_SPACE_MIN;
	} else if (size > TINY_ALLOC) {
		MemZone = &MemoryLayout.SmallZone;
		MinSlotSize = SMALL_SPACE_MIN;
	} else {
		MemZone = &MemoryLayout.TinyZone;
		MinSlotSize = TINY_SPACE_MIN;
	}

	t_free *Slot = get_free_slot(&MemZone->FreeList, RequestedSize);
	if (Slot == NULL) {
		// allocated new
		int ChunkSize = 0;
	   	if (size > SMALL_ALLOC)
			ChunkSize = LARGE_CHUNK(size);
		else if (size > TINY_ALLOC)
			ChunkSize = SMALL_CHUNK;
    		else {
			ChunkSize = TINY_CHUNK;
    		}

		void *NewChunk = alloc_chunk(MemZone, ChunkSize);
		if (NewChunk == NULL)
			return NULL;

    void *ChunkStartingAddr = CHUNK_STARTING_ADDR(NewChunk);
		void *FirstAddr = ChunkStartingAddr + HEADER_SIZE;
    Slot = lst_free_add(&MemZone->FreeList,
					CHUNK_USABLE_SIZE(ChunkSize),
					FirstAddr);
	}

	void *Addr = get_free_addr(Slot);
  int AllocatedSize = RequestedSize;
	if (Slot->Size >= (RequestedSize + MinSlotSize)) { // if there is enough space to make another slot
		Slot->Size -= RequestedSize;
    Addr += Slot->Size;
	} else {
		AllocatedSize = Slot->Size;
		lst_free_remove(&MemZone->FreeList, Slot);
	}
	
	t_header *hdr = (t_header *)Addr;
	if (get_free_addr(Slot) != Addr)
		hdr->Prev = get_free_addr(Slot);
	else
		hdr->Prev = NULL; // TODO(felix): replace NULL with real previous slot

	hdr->Next = Addr + AllocatedSize;

	PRINT("Allocated "); PRINT_UINT64(AlignedSize); PRINT(" bytes at address "); PRINT_ADDR(GET_SLOT(Addr)); NL();

	return GET_SLOT(Addr);
}

void	*malloc(size_t size) {
	//PRINT("Malloc request of size "); PRINT_UINT64(size); NL();
	
	return malloc_block(size);
}

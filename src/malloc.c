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

	while (lst != NULL && GET_FREE_SIZE(lst) < size) {
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
		RequestedSize = TINY_SPACE_MIN;
	}

	t_free *Slot = get_free_slot(&MemZone->FreeList, RequestedSize);
	if (Slot == NULL) {
		// allocated new
		int ChunkSize = 0;
	   	if (size > SMALL_ALLOC) {
			ChunkSize = LARGE_CHUNK(size);
		} else if (size > TINY_ALLOC) {
			ChunkSize = SMALL_CHUNK;
    		} else {
			ChunkSize = TINY_CHUNK;
    		}

		void *NewChunk = alloc_chunk(MemZone, ChunkSize);
		if (NewChunk == NULL)
			return NULL;

    void *ChunkStartingAddr = CHUNK_STARTING_ADDR(NewChunk);
    CHUNK_SET_POINTER_TO_FIRST_ALLOC(NewChunk, ChunkStartingAddr); 

		t_header *Hdr = (t_header *)ChunkStartingAddr;
		Hdr->Prev = NULL;
		Hdr->Next = NULL;
		Hdr->Size = CHUNK_USABLE_SIZE(ChunkSize);

		void *FirstAddr = ChunkStartingAddr + HEADER_SIZE;  

  	Slot = lst_free_add(&MemZone->FreeList, FirstAddr);
	}

	void *Addr = get_free_addr(Slot);	
	if (GET_FREE_SIZE(Slot) >= (RequestedSize + MinSlotSize)) { // if there is enough space to make another slot
		size_t NewSize = GET_FREE_SIZE(Slot) - RequestedSize;
		SET_FREE_SIZE(Slot, NewSize);
		
		t_header *PrevHdr = (t_header *)Addr;
		t_header *NextHdr = (UNFLAG(PrevHdr))->Next;		

		Addr += NewSize;
		t_header *Hdr = (t_header *)Addr;
		
		(UNFLAG(Hdr))->Size = RequestedSize;
		(UNFLAG(Hdr))->Prev = PrevHdr;
		(UNFLAG(PrevHdr))->Next = FLAG(Hdr);
		//PRINT("FLAGGING: "); PRINT_ADDR(Hdr); PRINT(" becomes "); PRINT_ADDR(FLAG(Hdr)); NL();	
	
		(UNFLAG(Hdr))->Next = NextHdr;
		
		if (UNFLAG(NextHdr) != NULL) {
			(UNFLAG(NextHdr))->Prev = FLAG(Hdr);
		}

		// flag previous header block's next
		// flag next header block's previous
	} else {
		t_header *Hdr = (t_header *)Addr;
		Hdr->Size = RequestedSize; //GET_FREE_SIZE(Slot);
		lst_free_remove(&MemZone->FreeList, Slot);

		if ((UNFLAG(Hdr->Prev)) != NULL) {
			(UNFLAG(Hdr->Prev))->Next = FLAG(Hdr);
		} else {
        void *ChunkPtr = MemZone->StartingBlockAddr;
        while (ChunkPtr != NULL && CHUNK_STARTING_ADDR(ChunkPtr) != Hdr) {
          ChunkPtr = GET_NEXT_CHUNK(ChunkPtr);
        }

        if (ChunkPtr != NULL)
          CHUNK_SET_POINTER_TO_FIRST_ALLOC(ChunkPtr, FLAG(Hdr));
    }

		if ((UNFLAG(Hdr->Next)) != NULL) {
			(UNFLAG(Hdr->Next))->Prev = FLAG(Hdr);
		}
	}

  void *AllocatedPtr = GET_SLOT(Addr);

	PRINT("Allocated "); PRINT_UINT64(AlignedSize); PRINT(" ["); PRINT_UINT64(RequestedSize); PRINT("] bytes at address "); PRINT_ADDR(AllocatedPtr); NL();

	return AllocatedPtr;
}

void	*malloc(size_t size) {
	//PRINT("Malloc request of size "); PRINT_UINT64(size); NL();
	
	return malloc_block(size);
}

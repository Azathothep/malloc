#include <sys/mman.h>
#include <errno.h>
#include "malloc.h"
#include "utils.h"
#include "lst_free.h"

void	free(void *Ptr) {
	//PRINT("Freeing address "); PRINT_ADDR(Ptr); NL();

	//TODO(felix): verify if block need to be freed beforehand
	
	//t_header *Header = GET_HEADER(Ptr);

 	size_t BlockSize = SLOT_USABLE_SIZE(Ptr);
	size_t BlockWithOverheadSize = SLOT_FULL_SIZE(Ptr);
	//PRINT("Slot size is "); PRINT_UINT64(BlockWithOverheadSize); NL();
	
	t_memchunks *MemBlock = NULL;
	if (BlockSize > SMALL_ALLOC) {
		MemBlock = &MemoryLayout.LargeZone;
	} else if (BlockSize > TINY_ALLOC) {
		MemBlock = &MemoryLayout.SmallZone;
 	} else {
  		MemBlock = &MemoryLayout.TinyZone;
	}

	t_free *Slot = lst_free_add(&MemBlock->FreeList, BlockWithOverheadSize, (void *)Ptr);

	t_free *Prev = Slot->Prev;

	//Coalesce
	if (Prev != NULL
	&& get_free_addr(Prev) + Prev->Size == get_free_addr(Slot)) {
		PRINT("Coalescing with previous slot for total size "); PRINT_UINT64(Prev->Size + Slot->Size); NL();
		Prev->Size += Slot->Size;
		lst_free_remove(&MemBlock->FreeList, Slot);
		Slot = Prev;
	}

	t_free *Next = Slot->Next;
	
	if (Next != NULL
	&& get_free_addr(Slot) + Slot->Size == get_free_addr(Next)) {
		PRINT("Coalescing with following slot for total size "); PRINT_UINT64(Slot->Size + Slot->Next->Size); NL();
		Slot->Size = Slot->Size + Slot->Next->Size;
		lst_free_remove(&MemBlock->FreeList, Slot->Next);
	}

	//Unmap
  	void *PrevChunk = NULL;
	void *CurrentChunk = MemBlock->StartingBlockAddr;
	void *CurrentChunkStartingAddr = CHUNK_STARTING_ADDR(CurrentChunk);
	while (CurrentChunk != NULL)
	{
		if (get_free_addr(Slot) == CurrentChunkStartingAddr
		&& Slot->Size == CHUNK_USABLE_SIZE(GET_CHUNK_SIZE(CurrentChunk))) {
			lst_free_remove(&MemBlock->FreeList, Slot);

      PRINT("Unmapping chunk at address "); PRINT_ADDR(CurrentChunk); PRINT(" and size "); PRINT_UINT64(GET_CHUNK_SIZE(CurrentChunk)); NL();
      
      			void *NextChunk = GET_NEXT_CHUNK(CurrentChunk);
      			if (munmap(CurrentChunk, GET_CHUNK_SIZE(CurrentChunk)) < 0)
      			{
        			PRINT_ERROR("Cannot unmap chunk, errno = "); PRINT_UINT64(errno); NL();
				return;
			}

			if (PrevChunk == NULL)	
				MemBlock->StartingBlockAddr = NextChunk;
      			else
				SET_NEXT_CHUNK(PrevChunk, NextChunk);
			
			break;
		} else {
      PrevChunk = CurrentChunk;
			CurrentChunk = GET_NEXT_CHUNK(CurrentChunk);
		}
	}
}

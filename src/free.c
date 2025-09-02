#include <sys/mman.h>
#include <errno.h>
#include "malloc.h"
#include "utils.h"
#include "lst_free.h"
#include "ft_malloc.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

size_t	coalesce_with_prev(t_header *MiddleHdr) {
	t_header *PrevHdr = UNFLAG(MiddleHdr->Prev);
	t_header *NextHdr = UNFLAG(MiddleHdr->Next);
	
	PrevHdr->Next = MiddleHdr->Next;
	if (!IS_LAST_HDR(NextHdr))
		NextHdr->Prev = MiddleHdr->Prev;

	size_t NewSize = (void *)NextHdr - (void *)PrevHdr;
	PrevHdr->Size = NewSize;
	return NewSize;
}

void	free(void *Ptr) {

	//TODO(felix): verify if block need to be freed beforehand	

 	size_t BlockSize = SLOT_USABLE_SIZE(Ptr);
	
	PRINT("Freeing address "); PRINT_ADDR(Ptr); PRINT(" (size: "); PRINT_UINT64(BlockSize); PRINT(", Header: "); PRINT_ADDR(GET_HEADER(Ptr)); PRINT(")"); NL();
	
  	t_memchunks *MemBlock = NULL;
	if (BlockSize > SMALL_ALLOC) {
		MemBlock = &MemoryLayout.LargeZone;
	} else if (BlockSize > TINY_ALLOC) {
		MemBlock = &MemoryLayout.SmallZone;
 	} else {
  		MemBlock = &MemoryLayout.TinyZone;
	}

	t_free *Slot = lst_free_add(&MemBlock->FreeList, (void *)Ptr);

	t_header *Hdr = GET_HEADER(Slot);
	t_header *PrevHdr = UNFLAG(Hdr->Prev);
	t_header *NextHdr = UNFLAG(Hdr->Next);

	//Coalesce
	if (PrevHdr != NULL && IS_FLAGGED(Hdr->Prev) == 0) {
    		lst_free_remove(&MemBlock->FreeList, GET_SLOT(Hdr));
		size_t NewSize = coalesce_with_prev(Hdr);
		(void)NewSize;
		//PRINT("Coalesced with previous slot for total size "); PRINT_UINT64(NewSize); NL();
		Hdr = UNFLAG(Hdr->Prev);
		PrevHdr = UNFLAG(Hdr->Prev);
  	}

	if (!IS_LAST_HDR(NextHdr) && IS_FLAGGED(Hdr->Next) == 0) {
		t_free *NextSlot = GET_SLOT(NextHdr);
    		lst_free_remove(&MemBlock->FreeList, NextSlot);
    		size_t NewSize = coalesce_with_prev(NextHdr);
		(void)NewSize;
		NextHdr = UNFLAG(Hdr->Next);
		//PRINT("Coalesced with following slot for total size "); PRINT_UINT64(NewSize); NL();
	}

	if (PrevHdr != NULL) {
		PrevHdr->Next = Hdr;
  	} else {
		// Sets the First Allocation Pointer
    		void *HdrPtr = (void *)Hdr - sizeof(void *); 
    		*(void **)HdrPtr = Hdr;
  	}

	if (!IS_LAST_HDR(NextHdr))
		NextHdr->Prev = Hdr;

	//Unmap
  	void *PrevChunk = NULL;
	void *CurrentChunk = MemBlock->StartingBlockAddr;
	void *CurrentChunkStartingAddr = CHUNK_STARTING_ADDR(CurrentChunk);
	
  	while (CurrentChunk != NULL)
	{
		size_t ChunkSize = GET_CHUNK_SIZE(CurrentChunk);		
		size_t ChunkUsableSize = CHUNK_USABLE_SIZE(ChunkSize);

		if (Hdr == CurrentChunkStartingAddr && Hdr->Size == ChunkUsableSize) {
			lst_free_remove(&MemBlock->FreeList, GET_SLOT(Hdr));

      			PRINT(ANSI_COLOR_RED);
 			PRINT("Unmapping chunk at address "); PRINT_ADDR(CurrentChunk); PRINT(" and size "); PRINT_UINT64(GET_CHUNK_SIZE(CurrentChunk)); NL();
      			PRINT(ANSI_COLOR_RESET);

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

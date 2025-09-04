#include <sys/mman.h>
#include <errno.h>
#include "malloc.h"
#include "utils.h"
#include "lst_free.h"
#include "ft_malloc.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

#define PRINT_FREE

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
	
#ifdef PRINT_FREE
	PRINT("Freeing address "); PRINT_ADDR(Ptr); PRINT(" (size: "); PRINT_UINT64(BlockSize); PRINT(", Header: "); PRINT_ADDR(GET_HEADER(Ptr)); PRINT(")"); NL();
#endif

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
	
	if (!IS_LAST_HDR(NextHdr))
		NextHdr->Prev = Hdr;

	if (PrevHdr != NULL) {
		PrevHdr->Next = Hdr;
	} else {
    		void *CurrentChunk = ((void *)Hdr - CHUNK_HEADER);	
		
		// Unmap ?
		size_t ChunkSize = GET_CHUNK_SIZE(CurrentChunk);
		if (Hdr->Size != CHUNK_USABLE_SIZE(ChunkSize)) {
			CHUNK_SET_POINTER_TO_FIRST_ALLOC(CurrentChunk, Hdr);
		} else {
			lst_free_remove(&MemBlock->FreeList, GET_SLOT(Hdr));
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

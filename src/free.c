#include <sys/mman.h>
#include <errno.h>
#include "malloc.h"
#include "utils.h"
#include "lst_free.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

size_t	coalesce_with_prev(t_header *MiddleHdr) {
	t_header *PrevHdr = (UNFLAG(MiddleHdr))->Prev;
	t_header *NextHdr = (UNFLAG(MiddleHdr))->Next;

	(UNFLAG(PrevHdr))->Next = NextHdr;
	if (UNFLAG(NextHdr) != NULL)
		(UNFLAG(NextHdr))->Prev = PrevHdr;

  size_t MiddleHdrSize = MiddleHdr->Size;
  size_t PrevHdrSize = ((void *)(UNFLAG(MiddleHdr))) - ((void *)(UNFLAG(PrevHdr)));
	size_t NewSize = MiddleHdrSize + PrevHdrSize;//(UNFLAG(PrevHdr))->Size + (UNFLAG(MiddleHdr))->Size;
	(UNFLAG(PrevHdr))->Size = NewSize;
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
	//(void)Slot;

  //PRINT("Created new Slot at "); PRINT_ADDR(Slot); PRINT(", prev: "); PRINT_ADDR(Slot->Prev); PRINT(", next: "); PRINT_ADDR(Slot->Next); NL();

	t_header *Hdr = GET_HEADER(Slot);

	//t_free *Prev = Slot->Prev;

	//Coalesce
  PRINT("Header: "); PRINT_ADDR(Hdr); NL();
	PRINT("Previous slot : "); PRINT_ADDR(UNFLAG(Hdr->Prev)); NL();
	if (UNFLAG(Hdr->Prev) != NULL && IS_FLAGGED(Hdr->Prev) == 0) {
    PRINT("Coalescing "); PRINT_ADDR(Hdr); PRINT(", slot "); PRINT_ADDR(GET_SLOT(Hdr)); PRINT(" with previous slot...\n");
    lst_free_remove(&MemBlock->FreeList, GET_SLOT(Hdr));
		size_t NewSize = coalesce_with_prev(Hdr);
		PRINT("Coalesced with previous slot for total size "); PRINT_UINT64(NewSize); NL();
		//Slot = Prev;
		Hdr = UNFLAG(Hdr->Prev);
  }
  // else {
	//	PRINT("Previous slot is NULL or still allocated: "); PRINT_ADDR(UNFLAG(Hdr->Prev)); NL();
	//}

	//t_free *Next = Slot->Next;	

	//PRINT("Following slot : "); PRINT_ADDR(UNFLAG(Hdr->Next)); NL();
	if (UNFLAG(Hdr->Next) != NULL && IS_FLAGGED(Hdr->Next) == 0) {
		t_free *NextSlot = GET_SLOT(UNFLAG(Hdr->Next));
		//PRINT("SLOT to remove = "); PRINT_ADDR(NextSlot); NL();
		lst_free_remove(&MemBlock->FreeList, NextSlot);
    PRINT("Coalescing to follow: "); PRINT_ADDR(UNFLAG(Hdr->Next)); NL();
    size_t NewSize = coalesce_with_prev(UNFLAG(Hdr->Next));
		PRINT("Coalesced with following slot for total size "); PRINT_UINT64(NewSize); NL();
	}
  //else {
	//	PRINT("Next slot is NULL or still allocated: "); PRINT_ADDR(UNFLAG(Hdr->Next)); NL();
	//}

	if (UNFLAG(Hdr->Prev) != NULL) {
		(UNFLAG(Hdr->Prev))->Next = UNFLAG(Hdr);
  } else {
    void **HdrPtr = (void **)(((void *)(UNFLAG(Hdr))) - sizeof(void *)); 
    *HdrPtr = UNFLAG(Hdr);
  }

	if (UNFLAG(Hdr->Next) != NULL)
		(UNFLAG(Hdr->Next))->Prev = UNFLAG(Hdr);

  Hdr = UNFLAG(Hdr);

	//Unmap
  void *PrevChunk = NULL;
	void *CurrentChunk = MemBlock->StartingBlockAddr;
	void *CurrentChunkStartingAddr = CHUNK_STARTING_ADDR(CurrentChunk);
	
  PRINT("UNMAP - CURRENT CHUNK "); PRINT_ADDR(CurrentChunk); NL();
  while (CurrentChunk != NULL)
	{
		size_t ChunkSize = GET_CHUNK_SIZE(CurrentChunk);		
		size_t ChunkUsableSize = CHUNK_USABLE_SIZE(ChunkSize);

    PRINT("UNMAP -- COMPARING "); PRINT_ADDR(Hdr); PRINT(" WITH "); PRINT_ADDR(CurrentChunkStartingAddr); NL();
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

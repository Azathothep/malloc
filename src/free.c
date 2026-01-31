#include <sys/mman.h>
#include <errno.h>
#include "malloc.h"
#include "utils.h"

#define ANSI_COLOR_RED		"\x1b[31m"
#define	ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_RESET	"\x1b[0m"

// --------- BINS ---------- //

// Get the index of the large bin that can store the provided AlignedSize
int get_large_bin_index(size_t AlignedSize) {
	// Getting large bin index is a bit complicated, because these bins are not spaced regularly.
	// Large bins are divided in segment. Every bin in the same segment is regularly spaced.
	// We must first find the associated segment, then get the bin index inside that segment.

	// This holds the max size of each segment
	int max_segment_sizes[LARGE_BINS_SEGMENTS_COUNT] = LARGE_BINS_SEGMENTS;

	// GET SEGMENT INDEX
	int segment_index = 0;
	while (segment_index < LARGE_BINS_SEGMENTS_COUNT && (int)AlignedSize > max_segment_sizes[segment_index]) 
		segment_index++;

	// If the size is higher than what the last segment can store, then return the bin dump's index
	if (segment_index == LARGE_BINS_SEGMENTS_COUNT)
		return LARGE_BINS_DUMP;

	// GET BIN INDEX IN SEGMENT
	// To get the bin index, we must first calculated the size relative to that segment
	// (for example, if AlignedSize = 1000 and the segments are 500 and 2000, it will be stored in the second segment.
	// Then, we substract the previous segment max size to the the size relative to that segment, which would be 1000 - 500 = 500
	int SizeInSegment = AlignedSize;
	if (segment_index == 0)
		SizeInSegment -= SMALL_ALLOC_MAX;
	else
		SizeInSegment -= max_segment_sizes[segment_index - 1];

	// This is the space between each bin of the target segment
	int space_between = GET_LARGE_BINS_SEGMENTS_SPACE_BETWEEN(segment_index);
	
	// Then, we calculate the bin index relative to that segment
	int index_in_segment = (SizeInSegment - 1) / space_between;

	// GET SEGMENT STARTING BIN INDEX
	// We must still get the starting index of the target segment in the whole array
	// We loop through each segment and add the number of bins each one hold, until we reach the target one
	int segment_starting_index = 0;
	int number_of_bins_in_segment = 1 << 5;
	while (segment_index > 0) {
		segment_starting_index += number_of_bins_in_segment;
		number_of_bins_in_segment = number_of_bins_in_segment >> 1;
		segment_index--;
	}

	// Finally, we only need to add the segment's starting index to the index in that segment
	int result = segment_starting_index + index_in_segment;

	return result;
}

int get_bin_index(size_t AlignedSize, t_zonetype ZoneType) {
	int result = 0;

	if (ZoneType == TINY) {
		if (AlignedSize > TINY_ALLOC_MAX)
			result = TINY_BINS_DUMP;
		else
			result = (AlignedSize / ALIGNMENT) - 1;
	} else if (ZoneType == SMALL) {
		if (AlignedSize > SMALL_ALLOC_MAX)
			result = SMALL_BINS_DUMP;
		else
			result = ((AlignedSize - TINY_ALLOC_MAX) / ALIGNMENT) - 1;
	} else {
		result = get_large_bin_index(AlignedSize);
	}

	return result;
}

void	put_slot_in_bin(t_header *Hdr, t_memzone *Zone) {
	int BinSize = Hdr->RealSize - HEADER_SIZE;

	int Index = get_bin_index(BinSize, Zone->ZoneType);

	t_header **Bins = Zone->Bins;

	if (Zone->ZoneType != LARGE) {
		t_header *NextHdrInBin = Bins[Index];
		
		Bins[Index] = Hdr;
		Hdr->PrevFree = NULL;
		Hdr->NextFree = NextHdrInBin;
		
		if (NextHdrInBin != NULL)
			NextHdrInBin->PrevFree = Hdr;
	} else {
		t_header *HdrIter = Bins[Index];

		t_header *PrevHdrIter = NULL;

		while (HdrIter != NULL && HdrIter->RealSize > Hdr->RealSize) {
			PrevHdrIter = HdrIter;
			HdrIter = HdrIter->NextFree;
		}

		Hdr->NextFree = HdrIter;
		Hdr->PrevFree = PrevHdrIter;

		if (HdrIter != NULL)
			HdrIter->PrevFree = Hdr;

		if (PrevHdrIter != NULL)
			PrevHdrIter->NextFree = Hdr;
		else
			Bins[Index] = Hdr;
	}
}

void	remove_slot_from_bin(t_header *Hdr, t_memzone *Zone) {
	int index = get_bin_index(Hdr->RealSize - HEADER_SIZE, Zone->ZoneType);

	if (Hdr->PrevFree != NULL) {
		Hdr->PrevFree->NextFree = Hdr->NextFree;
	} else {
		Zone->Bins[index] = Hdr->NextFree;
	}

	if (Hdr->NextFree != NULL)
			Hdr->NextFree->PrevFree = Hdr->PrevFree;

	Hdr->PrevFree = NULL;
	Hdr->NextFree = NULL;
}

void	try_coalesce_slot(t_header *Hdr, t_header **NextHdrToCheck, t_memzone *Zone) {
	t_header *NextFree = Hdr->NextFree;

	t_header *Base = Hdr;
	t_header *Prev = Base->Prev;

	//TODO(felix): optimize this part & start coalescing when backtracking
	while (Prev != NULL && Prev->State == FREE) {
		Base = Prev;
		Prev = Base->Prev;
	}
	
	size_t NewSize = Base->RealSize;
	t_header *Current = Base;
	t_header *Next = Current->Next;
	
	while (Next != NULL && Next->State == FREE) {

		while (NextFree != NULL 
			&& (uint64_t)NextFree > (uint64_t)Base
			&& (uint64_t)NextFree <= (uint64_t)Next)
			NextFree = NextFree->NextFree;

		Current = Next;
		NewSize += Current->RealSize;
		remove_slot_from_bin(Current, Zone);
		Next = Current->Next; 
	}

	if (Base->RealSize != NewSize) { 
		remove_slot_from_bin(Base, Zone);
		Base->RealSize = NewSize;
		Base->Next = Current->Next;

		if (Next != NULL)
			Next->Prev = Base;
	
		put_slot_in_bin(Base, Zone);
	}

	*NextHdrToCheck = NextFree;
}

void	coalesce_slots(t_memzone *Zone) {
	int i = 0;

	while (i < Zone->BinsCount) {
		t_header *Hdr = Zone->Bins[i];

		while (Hdr != NULL) {
			try_coalesce_slot(Hdr, &Hdr, Zone);
		}

		i++;
	}
}

void	unmap_memory(t_memchunk *Chunk) {
	int ret = munmap((void *)Chunk, Chunk->FullSize);

	if (ret < 0) {
		PRINT_ERROR("Failed to unmap memory, errno = "); PRINT_UINT64(errno); NL();
	}
}

void	relink_chunk_and_unmap(t_memchunk *Chunk, t_memzone *Zone) {
	t_header *FirstHdr = (t_header *)CHUNK_STARTING_ADDR(Chunk);
	remove_slot_from_bin(FirstHdr, Zone);

	Zone->MemStatus.TotalMappedMemSize -= FirstHdr->RealSize;
	Zone->MemStatus.TotalFreedMemSize -= FirstHdr->RealSize;

	if (Chunk->Prev != NULL) {
		Chunk->Prev->Next = Chunk->Next;
	} else {
		Zone->FirstChunk = Chunk->Next;
	}

	if (Chunk->Next != NULL) {
		Chunk->Next->Prev = Chunk->Prev;
	}
	
	unmap_memory(Chunk);
}

typedef struct	s_unmapchunkargs {
	t_memzone 	*Zone;
	size_t 		MinChunkMemBeforeUnmap;
	size_t 		FreeableChunkSize;
}				t_unmapchunkargs;

int		unmap_chunk_recurse(t_memchunk *Chunk, t_unmapchunkargs *UnmapChunkArgs) {
	size_t ChunkUsableSize = CHUNK_USABLE_SIZE(Chunk->FullSize);
	t_header *FirstHdr = (t_header *)CHUNK_STARTING_ADDR(Chunk);
		
	int unmappable = FirstHdr->State == FREE && FirstHdr->RealSize == ChunkUsableSize;

	if (unmappable) {
		UnmapChunkArgs->FreeableChunkSize += Chunk->FullSize;
	}

	if (Chunk->Next == NULL) {
		if (UnmapChunkArgs->FreeableChunkSize < UnmapChunkArgs->MinChunkMemBeforeUnmap)
			return 0;
				
		if (unmappable)
			relink_chunk_and_unmap(Chunk, UnmapChunkArgs->Zone);
	
		return 1;
	}

	if (unmap_chunk_recurse(Chunk->Next, UnmapChunkArgs) == 0) {
		return 0;
	}

	if (unmappable)
		relink_chunk_and_unmap(Chunk, UnmapChunkArgs->Zone);
	
	return 1;
}

typedef struct	s_memparams {
	size_t	MinMemToKeep;
	size_t	MinFreedMemBeforeCoalesce;
	size_t	MinChunkMemBeforeUnmap;
}				t_memparams;

void	unmap_free_zone_chunks(t_memzone *Zone) {
	t_memparams MemParams;

	if (Zone->ZoneType == TINY) {
		MemParams = (t_memparams){ 	MIN_MEM_TO_KEEP_TINY,
									MIN_FREED_MEM_BEFORE_COALESCE_TINY,
									MIN_CHUNK_MEM_BEFORE_UNMAP_TINY };
	} else if (Zone->ZoneType == SMALL) {
		MemParams = (t_memparams){ 	MIN_MEM_TO_KEEP_SMALL,
									MIN_FREED_MEM_BEFORE_COALESCE_SMALL,
									MIN_CHUNK_MEM_BEFORE_UNMAP_SMALL };
	} else {
		MemParams = (t_memparams){ 	MIN_MEM_TO_KEEP_LARGE,
									MIN_FREED_MEM_BEFORE_COALESCE_LARGE,
									MIN_CHUNK_MEM_BEFORE_UNMAP_LARGE };
	}

	if (Zone->MemStatus.TotalMappedMemSize < MemParams.MinMemToKeep)
		return;

	if (Zone->MemStatus.FreedMemSinceLastCoalescion > MemParams.MinFreedMemBeforeCoalesce) {
		coalesce_slots(Zone);
		t_unmapchunkargs UnmapChunkArgs = { Zone, 0, MemParams.MinChunkMemBeforeUnmap };
		unmap_chunk_recurse(Zone->FirstChunk, &UnmapChunkArgs);
		Zone->MemStatus.FreedMemSinceLastCoalescion = 0;
	}
}

void	free_slot(t_header *Hdr) {
	size_t RealSize = Hdr->RealSize;
 	size_t BlockSize = RealSize - HEADER_SIZE;
	
	t_memzone 	*Zone = NULL;

	if (BlockSize > SMALL_ALLOC_MAX) {
		Zone = GET_LARGE_ZONE();
	} else if (BlockSize > TINY_ALLOC_MAX) {
		Zone = GET_SMALL_ZONE();
	} else {
		Zone = GET_TINY_ZONE();
	}

	Hdr->State = FREE;
	put_slot_in_bin(Hdr, Zone);

	Zone->MemStatus.TotalFreedMemSize += RealSize;
	Zone->MemStatus.FreedMemSinceLastCoalescion += RealSize;
}

void	flush_unsorted_bin() {
	t_header *Hdr = MemoryLayout.UnsortedBin;

	while (Hdr != NULL) {
		t_header *Next = Hdr->NextFree;
		free_slot(Hdr);
		Hdr = Next;
	}

	MemoryLayout.UnsortedBin = NULL;
	
	unmap_free_zone_chunks(GET_LARGE_ZONE());
	unmap_free_zone_chunks(GET_SMALL_ZONE());
	unmap_free_zone_chunks(GET_TINY_ZONE());
}

void	add_to_unsorted_bin(t_header *Hdr) {

	Hdr->State = UNSORTED_FREE;

	t_header *FirstBinHdr = MemoryLayout.UnsortedBin;

	Hdr->PrevFree = NULL;
	Hdr->NextFree = NULL;

	MemoryLayout.UnsortedBin = Hdr;

	if (FirstBinHdr == NULL)
		return;

	Hdr->NextFree = FirstBinHdr;
	FirstBinHdr->PrevFree = Hdr;
}


// ----------- FREE ------------ //

void	free(void *Ptr) {

	if (Ptr == NULL)
		return;

	t_header *Hdr = GET_HEADER(Ptr);

	if (Hdr->State != INUSE)
		return;

	add_to_unsorted_bin(Hdr);
}

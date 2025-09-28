#include "malloc.h"
#include "utils.h"

//TODO(felix): add chunk subdivision in zones

void	show_free_zone(t_memchunks *Zone) {
	//t_free *lst = Zone->FreeList;	
	t_header *lst = Zone->FreeList;	

	while (lst) {
		PRINT_ADDR(lst); PRINT(": ");
		//PRINT_UINT64(GET_FREE_SIZE(lst));
		PRINT_UINT64(lst->RealSize);
		PRINT(" bytes\n");
		lst = lst->Next;
	}
}

void	show_free_mem() {
	PRINT("\n---- free mem ----\n");

	t_memchunks *Zone = GET_TINY_ZONE(); //&MemoryLayout.TinyZone;
	PRINT("TINY ZONE : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_free_zone(Zone);
	NL();

	Zone = GET_SMALL_ZONE(); //&MemoryLayout.SmallZone;
	PRINT("SMALL ZONE : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_free_zone(Zone);
	NL();

	Zone = GET_LARGE_ZONE(); //&MemoryLayout.LargeZone;
	PRINT("LARGE ZONE : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_free_zone(Zone);
	NL();

	PRINT("------------------\n");
}

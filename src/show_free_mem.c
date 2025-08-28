#include "malloc.h"
#include "lst_free.h"
#include "utils.h"

//TODO(felix): add chunk subdivision in zones

void	show_free_zone(t_memchunks *Zone) {
	t_free *lst = Zone->FreeList;	
	
	while (lst) {
		PRINT_ADDR(get_free_addr(lst)); PRINT(": "); PRINT_UINT64(lst->Size); PRINT(" bytes\n");
		lst = lst->Next;
	}
}

void	show_free_mem() {
	PRINT("\n---- free mem ----\n");

	t_memchunks *Zone = &MemoryLayout.TinyZone;
	PRINT("TINY ZONE : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_free_zone(Zone);
	NL();

	Zone = &MemoryLayout.SmallZone;
	PRINT("SMALL ZONE : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_free_zone(Zone);
	NL();

	Zone = &MemoryLayout.LargeZone;
	PRINT("LARGE ZONE : "); PRINT_ADDR(Zone->StartingBlockAddr); NL();
	show_free_zone(Zone);
	NL();

	PRINT("------------------\n");
}

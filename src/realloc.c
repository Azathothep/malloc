# include "malloc.h"
# include "utils.h"

void	*realloc(void *ptr, size_t size) {
	(void)ptr;
	(void)size;
	PRINT("Realloc called on "); PRINT_ADDR(ptr); PRINT(" for "); PRINT_UINT64(size); NL();
	return NULL;
}

//#include <stddef.h>
#include <unistd.h>

void	*malloc(size_t size) {
	(void)size;
	write(1, "Hello\n", 6);
	return NULL;
}

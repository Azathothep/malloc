#include <stddef.h>
#include <stdio.h>
#include "malloc.h"

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	void *p = malloc(5);
	(void)p;
	return 0;
}

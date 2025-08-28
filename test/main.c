#include <stddef.h>
#include <stdio.h>
#include "ft_malloc.h"
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	show_free_mem();
	void *a[150];
	int i = 0;
	while (i < 150){
		//size_t size = (i % 5 * 70) + 1;
    a[i] = malloc(24);
    i++;
	}

  	i = 0;
	while (i < 150) {
    		free(a[i]);
    		i++;
  	}

	void *p = malloc(5000);
	free(p);

	show_free_mem();

	/*
	void *p = malloc(7);
	(void)p;
	void *p2 = malloc(7);
	(void)p2;
	void *p3 = malloc(7);
	(void)p3;

	free(p3);	
	free(p);
	free(p2);
	*/

	return 0;
}

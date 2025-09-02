#include <stddef.h>
#include <stdio.h>
#include "ft_malloc.h"
#include <string.h>
#include <unistd.h>

# define INDEX	150

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	show_free_mem();

	void *a[INDEX];
	int i = 0;
	while (i < INDEX){
		size_t size =(i % 5 * 70) + 1;
    		a[i] = malloc(size);
    		i++;
	}

	show_alloc_mem();

	show_free_mem();

	i = 0;
	while (i < INDEX / 2) {
    		free(a[i]);
    		i++;
  	}

	show_alloc_mem();
  show_free_mem();
  
	while (i < INDEX) {
		free(a[i]);
		i++;
	}

	void *p = malloc(5000);
	free(p);

  show_alloc_mem();

	show_free_mem();
  

  /*
  void *p = malloc(7);
	(void)p;
	void *p2 = malloc(7);
	(void)p2;
	void *p3 = malloc(7);
	(void)p3;

  show_alloc_mem();

  show_free_mem();

	free(p3);
  show_free_mem();
	free(p);
	free(p2);
	*/

	return 0;
}

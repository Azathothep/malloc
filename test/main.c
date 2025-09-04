#include <stddef.h>
#include <stdio.h>
#include "ft_malloc.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

# define INDEX	300

void process_allocation(void *a[], int index) {	
	int i = 0;
	while (i < index - 1){
		size_t size = (i % 20 * 64) + 1;
    		a[i] = malloc(size);
    		i++;
	}

	a[index - 1] = malloc(5000);
}

void process_free(void *a[], int index) {
	int i = 0;
	while (i < index) {
		free(a[i]);
		i++;
	}
}

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;	

	int n = 1;
	if (argc > 1)
		n = atoi(argv[1]);

 	//char *buffer = NULL;
	//read(0, &buffer, 1);

	struct timeval t0, t1;
	float mallocTotalTime = 0;
	float freeTotalTime = 0;

	void *a[INDEX];
	(void)a;

	int i = 0;
	while (i < n) {
		gettimeofday(&t0, NULL);
		process_allocation(a, INDEX);
		gettimeofday(&t1, NULL);
	
		float deltaTime = t1.tv_sec - t0.tv_sec + 1E-6 * (t1.tv_usec - t0.tv_usec);
		mallocTotalTime += deltaTime;		

		//show_alloc_mem();

		gettimeofday(&t0, NULL);
		process_free(a, INDEX);
		gettimeofday(&t1, NULL);

		deltaTime = t1.tv_sec - t0.tv_sec + 1E-6 * (t1.tv_usec - t0.tv_usec);
		freeTotalTime += deltaTime;

		i++;
	}

	float mallocAverageTime = mallocTotalTime / i;
	float freeAverageTime = freeTotalTime / i;

	printf("MALLOC - Total time: %.2g seconds (%f)\n", mallocTotalTime, mallocAverageTime);
	printf("FREE - Total time: %.2g seconds (%f)\n", freeTotalTime, freeAverageTime);

	show_alloc_mem();

	show_free_mem();
  
	return 0;
}

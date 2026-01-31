#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "libft.h"

#ifndef STDLIB
 #include "ft_malloc.h"
#endif

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

void	lst_alloc(t_list **begin_lst, size_t size) {
	char *alloc = malloc(size);	
	size_t i = 0;
	while (i < size) {
		alloc[i] = 0xFF;
		i++;
	}

	t_list *list = ft_lstnew(alloc);
	ft_lstadd_front(begin_lst, list);
}

void	lst_free(t_list **begin_lst, int index) {
	t_list *lst = *begin_lst;

	if (index == 0)
	{
		*begin_lst = lst->next;
		ft_lstdelone(lst, &free);
		return;
	}

	int i = 0;
	while (lst != NULL && i < (index - 1)) {
		lst = lst->next;
		i++;
	}

	if (lst == NULL)
		return;

	t_list *to_free = lst->next;
	if (to_free == NULL) {
		return;
	}

	lst->next = to_free->next;
	ft_lstdelone(to_free, &free);
}

void random_allocations(int maxAlloc, size_t maxAllocSize) {
	t_list *begin_lst = NULL;

	int i = 0;
	int third = maxAlloc / 3;
	int twoThird = maxAlloc - third;

	// allocating maxAlloc
	while (i < maxAlloc) {
		size_t size = (rand() % maxAllocSize) + 1;
		lst_alloc(&begin_lst, size);
		i++;
	}

	// freeing two-third of allocations, goes to 1-third
	i = 0;
	while (i < twoThird) {
		int index = (rand() % (maxAlloc - i));
		lst_free(&begin_lst, index);
		i++;
	}

	// re-allocating one-third, goes to 2-third
	i = 0;
while (i < third) {
		size_t size = (rand() % maxAllocSize) + 1;
		lst_alloc(&begin_lst, size);
		i++;
	}

	// re-freeing one-third, goes to 1-third
	i = 0;
	while (i < third) {
		int index = (rand() % (twoThird - i));
		lst_free(&begin_lst, index);
		i++;
	}

	// re-allocating to max, goes to maxAlloc
	i = ft_lstsize(begin_lst);
	while (i < maxAlloc) {
		size_t size = (rand() % maxAllocSize) + 1;
		lst_alloc(&begin_lst, size);
		i++;
	}

	// freeing everything
	ft_lstclear(&begin_lst, &free);
}

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	unsigned int seed = time(NULL);
	int 	n = 1;
	int		maxAlloc = 1000; //100;
	size_t 	maxAllocSize = 20480;

	if (argc > 1)
		seed = atoi(argv[1]);

	if (argc > 2)
		n = atoi(argv[2]);

	if (argc > 3)
		maxAlloc = atoi(argv[3]);

	if (argc > 4)
		maxAllocSize = atoi(argv[4]);

	char seedchar[32];
	ft_bzero(seedchar, 32);
	sprintf(seedchar, "%u", seed);
	srand(seed);

	struct timeval t0, t1;
	float mallocTotalTime = 0;

	int i = 0;
	while (i < n) {
		gettimeofday(&t0, NULL);
		random_allocations(maxAlloc, maxAllocSize);
		gettimeofday(&t1, NULL);
		
		float deltaTime = t1.tv_sec - t0.tv_sec + 1E-6 * (t1.tv_usec - t0.tv_usec);
		mallocTotalTime += deltaTime;		
		
		i++;
	}

	float mallocAverageTime = mallocTotalTime / i;

	printf("MALLOC - Total time: %.2g seconds (%f)\n", mallocTotalTime, mallocAverageTime);

	printf("SEED: %s\n", seedchar);
  
	return 0;
}

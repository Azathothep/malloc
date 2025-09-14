#include <stddef.h>
#include <stdio.h>
#include "ft_malloc.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "libft.h"

# define INDEX	1000i

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
	void *alloc = malloc(size);	
	t_list *list = ft_lstnew(alloc);
	ft_lstadd_front(begin_lst, list);
}

void	lst_free(t_list **begin_lst, int index) {
	t_list *lst = *begin_lst;

	if (index == 0)
	{
		*begin_lst = lst->next;
//		printf("Freeing %p\n", lst->content);
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

//	printf("Freeing %p\n", lst->next->content);
	lst->next = to_free->next;
	ft_lstdelone(to_free, &free);
//	show_alloc_mem();
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

	printf("MAX ALLOCATION...\n");
	show_alloc_mem();


	// freeing two-third of allocations, goes to 1-third
	i = 0;
	while (i < twoThird) {
		int index = (rand() % (maxAlloc - i));
		lst_free(&begin_lst, index);
		i++;
	}

	printf("FREED 2/3...\n");
	show_alloc_mem();

	// re-allocating one-third, goes to 2-third
	i = 0;
	while (i < third) {
		size_t size = (rand() % maxAllocSize) + 1;
		lst_alloc(&begin_lst, size);
		i++;
	}

	printf("RE-ALLOCATED 1/3, GOES TO 2/3...\n");
	show_alloc_mem();

	// re-freeing one-third, goes to 1-third
	i = 0;
	while (i < third) {
		int index = (rand() % (twoThird - i));
		lst_free(&begin_lst, index);
		i++;
	}

	printf("RE-FREED 1/3, GOES TO 1/3...\n");
	show_alloc_mem();

	// re-allocating to max, goes to maxAlloc
	i = ft_lstsize(begin_lst);
	while (i < maxAlloc) {
		size_t size = (rand() % maxAllocSize) + 1;
		lst_alloc(&begin_lst, size);
	//	show_alloc_mem();
		i++;
	}

	printf("RE-ALLOCATED TO MAX...\n");
//	show_alloc_mem();

	// freeing everything
	ft_lstclear(&begin_lst, &free);

	printf("FREED ALL\n");
	show_alloc_mem();
}

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;	

	int 	n = 1;
	int	maxAlloc = 100; // 10
	size_t 	maxAllocSize = 64; // 4096

	if (argc > 1)
		n = atoi(argv[1]);

	if (argc > 2)
		maxAlloc = atoi(argv[2]);

	if (argc > 3)
		maxAllocSize = atoi(argv[3]);

 	//char *buffer = NULL;
	//read(0, &buffer, 1);

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

/*	void *a[INDEX];
	(void)a;

	float freeTotalTime = 0;
	
	while (i < n) {
		gettimeofday(&t0, NULL);
		process_allocation(a, INDEX);
		gettimeofday(&t1, NULL);
	
		float deltaTime = t1.tv_sec - t0.tv_sec + 1E-6 * (t1.tv_usec - t0.tv_usec);
		mallocTotalTime += deltaTime;		

//		show_alloc_mem();

		gettimeofday(&t0, NULL);
		process_free(a, INDEX);
		gettimeofday(&t1, NULL);

		deltaTime = t1.tv_sec - t0.tv_sec + 1E-6 * (t1.tv_usec - t0.tv_usec);
		freeTotalTime += deltaTime;

		i++;
	}


//	float freeAverageTime = freeTotalTime / i;

//	printf("FREE - Total time: %.2g seconds (%f)\n", freeTotalTime, freeAverageTime);
*/
	float mallocAverageTime = mallocTotalTime / i;

	printf("MALLOC - Total time: %.2g seconds (%f)\n", mallocTotalTime, mallocAverageTime);

//	show_alloc_mem();

//	show_free_mem();
  
	return 0;
}

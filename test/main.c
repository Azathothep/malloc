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

void	lst_add_allocation(t_list **BeginLst, size_t AllocationSize) {
	char *Allocation = malloc(AllocationSize);	
	size_t i = 0;
	while (i < AllocationSize) {
		Allocation[i] = 0xFF;
		i++;
	}

	t_list *List = ft_lstnew(Allocation);
	ft_lstadd_front(BeginLst, List);
}

void	lst_free_allocation(t_list **BeginLst, int LstIndex) {
	t_list *Lst = *BeginLst;

	if (LstIndex == 0)
	{
		*BeginLst = Lst->next;
		ft_lstdelone(Lst, &free);
		return;
	}

	int i = 0;
	while (Lst != NULL && i < (LstIndex - 1)) {
		Lst = Lst->next;
		i++;
	}

	if (Lst == NULL)
		return;

	t_list *ToFree = Lst->next;
	if (ToFree == NULL) {
		return;
	}

	Lst->next = ToFree->next;
	ft_lstdelone(ToFree, &free);
}

// The test is divided into multiple steps:
// 1) Allocate MaxNumberOfAllocations allocations, each one of a random size between 0 and MaxAllocationSize
// 2) Randomly free 1/3rd of the MaxNumberOfAllocations
// 3) Allocate again, up to 2/3rd of the MaxNumberOfAllocations
// 4) Free again randomly 2/3rd of the MaxNumberOfAllocations
// 5) Allocate again, up to the MaxNumberOfAllocations
// 6) Finally, free everything.
// This process is designed to stress test the defragmentation process, by freeing only some part of memory and re-allocating it just after
void test_malloc_and_free(int MaxNumberOfAllocations, size_t MaxAllocationSize) {
	t_list *BeginLst = NULL;

	int i = 0;
	int OneThirdAllocationsCount = MaxNumberOfAllocations / 3;
	int TwoThirdAllocationsCount = MaxNumberOfAllocations - OneThirdAllocationsCount;

// 1) Allocate MaxNumberOfAllocations allocations, each one of a random size between 0 and MaxAllocationSize
	while (i < MaxNumberOfAllocations) {
		size_t size = (rand() % MaxAllocationSize) + 1;
		lst_add_allocation(&BeginLst, size);
		i++;
	}

// 2) Randomly free 1/3rd of the MaxNumberOfAllocations
	i = 0;
	while (i < TwoThirdAllocationsCount) {
		int index = (rand() % (MaxNumberOfAllocations - i));
		lst_free_allocation(&BeginLst, index);
		i++;
	}

// 3) Allocate 1/3rd, up to 2/3rd of the MaxNumberOfAllocations
	i = 0;
	while (i < OneThirdAllocationsCount) {
		size_t size = (rand() % MaxAllocationSize) + 1;
		lst_add_allocation(&BeginLst, size);
		i++;
	}

// 4) Free again randomly 2/3rd of the MaxNumberOfAllocations
	i = 0;
	while (i < OneThirdAllocationsCount) {
		int index = (rand() % (TwoThirdAllocationsCount - i));
		lst_free_allocation(&BeginLst, index);
		i++;
	}

// 5) Allocate 2/rd, up to the MaxNumberOfAllocations
	i = ft_lstsize(BeginLst);
	while (i < TwoThirdAllocationsCount) {
		size_t size = (rand() % MaxAllocationSize) + 1;
		lst_add_allocation(&BeginLst, size);
		i++;
	}

// 6) Finally, free everything.
	ft_lstclear(&BeginLst, &free);
}

// Arguments: [Max number of allocations] [Max allocation size] [Loops] [Seed]
int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	int		MaxNumberOfAllocations = 10000; //100;
	size_t 	MaxAllocationSize = 2048;
	int 	Loops = 10;
	unsigned int Seed = time(NULL);

	if (argc > 1)
		MaxNumberOfAllocations = atoi(argv[1]);

	if (argc > 2)
		MaxAllocationSize = atoi(argv[2]);

	if (argc > 3)
		Loops = atoi(argv[3]);

	if (argc > 4)
		Seed = atoi(argv[4]);

	char seedchar[32];
	ft_bzero(seedchar, 32);
	sprintf(seedchar, "%u", Seed);
	srand(Seed);

	printf("SEED: %s\n", seedchar);
	
	struct timeval t0, t1;
	float MallocTotalTime = 0;

	int i = 0;
	while (i < Loops) {
		printf("Loop %d / %d...\r", i, Loops);
		fflush(stdout);
		gettimeofday(&t0, NULL);
		test_malloc_and_free(MaxNumberOfAllocations, MaxAllocationSize);
		gettimeofday(&t1, NULL);
		
		float deltaTime = t1.tv_sec - t0.tv_sec + 1E-6 * (t1.tv_usec - t0.tv_usec);
		MallocTotalTime += deltaTime;		
		
		i++;
	}

	float MallocAverageTime = MallocTotalTime / i;

	printf("Total time: %d loops in %.2g seconds (%fs / loop)\n", Loops, MallocTotalTime, MallocAverageTime);
	printf("Max number of allocations: %d\n", MaxNumberOfAllocations);
	printf("Max allocation size: %lu\n", MaxAllocationSize);
  
	return 0;
}

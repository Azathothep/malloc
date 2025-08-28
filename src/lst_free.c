#include "malloc.h"
#include "utils.h"

void	*get_free_addr(t_free *Slot) {
	return (void *)GET_HEADER(Slot);
}

void	*lst_free_add(t_free **BeginList, int Size, void *Addr) {
//	it_free *slot = lst_free_new(size, addr);

	t_free *Slot = (t_free *)Addr;
	Slot->Size = Size;
	Slot->Prev = NULL;
	Slot->Next = NULL;

	if (*BeginList == NULL) {
		*BeginList = Slot;
		return Slot;
	}

	t_free *List = *BeginList;
	while (List->Next != NULL && List->Next < Slot) {
		List = List->Next;
	}

	if (List->Next != NULL) {
		Slot->Next = List->Next;
		List->Next->Prev = Slot;
	}
	
	Slot->Prev = List;
	List->Next = Slot;
	
	return Slot;
}

void	lst_free_remove(t_free **BeginList, t_free *Slot) {
	if (*BeginList == Slot) {
		*BeginList = Slot->Next;
		return;
	}

	t_free *Prev = Slot->Prev;
	t_free *Next = Slot->Next;
	if (Prev != NULL)
		Prev->Next = Next;
	if (Next != NULL)
		Next->Prev = Prev;	
	
	return;
}

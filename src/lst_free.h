#ifndef LST_FREE_H
# define LST_FREE_H

void	*lst_free_add(t_free **BeginList, int Size, void *Addr);
void	lst_free_remove(t_free **BeginList, t_free *Slot);
void	*get_free_addr(t_free *Slot);

#endif
